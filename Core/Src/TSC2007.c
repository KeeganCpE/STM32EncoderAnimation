/**
 * @file    TSC2007.c
 * @brief   STM32 HAL driver for the TSC2007 resistive touch controller.
 *          Translated from Adafruit_TSC2007 (C++ / Arduino) to plain C.
 *
 * Original Adafruit library:
 *   Copyright (c) Adafruit Industries. All rights reserved. BSD license.
 *
 * Translation notes
 * ─────────────────
 *  • Adafruit_I2CDevice::write() + read() → HAL_I2C_Master_Transmit() +
 *    HAL_I2C_Master_Receive().  The HAL address parameter is the 7-bit
 *    address shifted left by 1 (0x48 << 1 = 0x90).
 *  • delayMicroseconds(500) → a simple busy-wait loop using DWT or a
 *    manual cycle count.  We use a loop based on the 32 MHz system clock
 *    so no DWT setup is needed.
 *  • The C++ class is replaced by a module-level static state struct.
 *  • The TIRQ interrupt replaces Arduino's IRQ pin polling.
 */

#include "TSC2007.h"
#include <stdlib.h>   /* abs() */

/* ======================================================================== */
/*  Minimum Z1 pressure to accept a touch as valid.                          */
/*  When the finger is physically up, Z1 reads near 0 (no voltage divider). */
/*  When touching, Z1 is typically 100–800 depending on pressure.           */
/*  A threshold of 20 safely rejects pen-up noise without dropping light     */
/*  touches.  Raise it if you see phantom touches; lower if light taps are  */
/*  missed.                                                                  */
/* ======================================================================== */
#define TSC2007_MIN_PRESSURE  1U

/* ======================================================================== */
/*  Module-level state                                                       */
/* ======================================================================== */

/** Set by TSC2007_EXTI_Callback(), cleared by GetPoint / ClearFlag. */
static volatile bool s_touch_flag = false;

/** Optional user callback invoked from ISR. */
static volatile TSC2007_TouchCallback_t s_user_callback = NULL;

/* ======================================================================== */
/*  Internal helpers                                                         */
/* ======================================================================== */

/**
 * @brief  Busy-wait approximately N microseconds.
 *         Based on a 32 MHz SYSCLK (the Nucleo-L152RE PLL configuration).
 *         Each iteration is ~3 cycles; 32 MHz / 3 ≈ 10.7 iterations/µs.
 *         Adjust the multiplier if you change the system clock.
 */
static void delay_us(uint32_t us)
{
    /* 32 MHz → ~10 iterations per µs at 3 cycles/iteration */
    volatile uint32_t count = us * 10U;
    while (count--) {}
}

/**
 * @brief  Write one byte to the TSC2007 over I2C.
 * @return true on success.
 */
static bool I2C_WriteByte(uint8_t byte)
{
    return HAL_I2C_Master_Transmit(&TSC2007_I2C_HANDLE,
                                   TSC2007_I2C_ADDR,
                                   &byte, 1U,
                                   TSC2007_I2C_TIMEOUT) == HAL_OK;
}

/**
 * @brief  Read N bytes from the TSC2007 over I2C.
 * @return true on success.
 */
static bool I2C_ReadBytes(uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Master_Receive(&TSC2007_I2C_HANDLE,
                                  TSC2007_I2C_ADDR,
                                  buf, len,
                                  TSC2007_I2C_TIMEOUT) == HAL_OK;
}

/* ======================================================================== */
/*  TSC2007_EXTI_Callback                                                    */
/*  Called from EXTI15_10_IRQHandler in stm32l1xx_it.c (ISR context).      */
/* ======================================================================== */

void TSC2007_EXTI_Callback(void)
{
    /* Only act on the TIRQ pin — guard against other EXTI 10-15 sources */
    if (__HAL_GPIO_EXTI_GET_IT(TSC2007_TIRQ_PIN))
    {
        s_touch_flag = true;

        if (s_user_callback != NULL)
        {
            s_user_callback();
        }
    }
}

/* ======================================================================== */
/*  TSC2007_Init                                                             */
/* ======================================================================== */

bool TSC2007_Init(void)
{
    s_touch_flag    = false;
    s_user_callback = NULL;

    /*
     * Send the initial POWERDOWN_IRQON command.
     * This arms the TIRQ line so it will assert LOW on the first touch.
     * Mirrors: command(MEASURE_TEMP0, POWERDOWN_IRQON, ADC_12BIT) in begin()
     */
    uint16_t result = TSC2007_Command(TSC2007_MEASURE_TEMP0,
                                      TSC2007_POWERDOWN_IRQON,
                                      TSC2007_ADC_12BIT);

    /* Any I2C error from Command() returns 0; a real TEMP0 read might
     * also be 0, so we instead check the bus state after the call.
     * A simpler check: just try a dummy write and verify HAL_OK. */
    (void)result;

    /* Verify device is present by attempting a zero-length transmit */
    return HAL_I2C_Master_Transmit(&TSC2007_I2C_HANDLE,
                                   TSC2007_I2C_ADDR,
                                   NULL, 0U,
                                   TSC2007_I2C_TIMEOUT) == HAL_OK;
}

/* ======================================================================== */
/*  TSC2007_RegisterCallback                                                 */
/* ======================================================================== */

void TSC2007_RegisterCallback(TSC2007_TouchCallback_t cb)
{
    s_user_callback = cb;
}

/* ======================================================================== */
/*  TSC2007_IsTouched / TSC2007_ClearFlag                                   */
/* ======================================================================== */

bool TSC2007_IsTouched(void)
{
    return s_touch_flag;
}

void TSC2007_ClearFlag(void)
{
    s_touch_flag = false;
}

/* ======================================================================== */
/*  TSC2007_Command                                                          */
/*                                                                           */
/*  Translated from Adafruit_TSC2007::command().                            */
/*                                                                           */
/*  Command byte layout:                                                     */
/*    bits [7:4] = function (4 bits)                                         */
/*    bits [3:2] = power mode (2 bits)                                       */
/*    bit  [1]   = resolution (1 bit)                                        */
/*    bit  [0]   = 0 (unused)                                                */
/*                                                                           */
/*  Response: 2 bytes.                                                       */
/*    12-bit result = (byte0 << 4) | (byte1 >> 4)                            */
/* ======================================================================== */

uint16_t TSC2007_Command(TSC2007_Function_t   func,
                         TSC2007_Power_t      pwr,
                         TSC2007_Resolution_t res)
{
    uint8_t cmd = (uint8_t)((uint8_t)func << 4U)
                | (uint8_t)((uint8_t)pwr  << 2U)
                | (uint8_t)((uint8_t)res  << 1U);

    if (!I2C_WriteByte(cmd))
    {
        return 0U;
    }

    /* Wait ≥ 500 µs for ADC conversion (datasheet requirement) */
    delay_us(500U);

    uint8_t reply[2] = {0, 0};
    if (!I2C_ReadBytes(reply, 2U))
    {
        return 0U;
    }

    /* Reconstruct 12-bit value: high byte provides upper 8 bits [11:4],
     * low byte provides lower 4 bits [3:0] in its upper nibble [7:4]. */
    return ((uint16_t)reply[0] << 4U) | ((uint16_t)reply[1] >> 4U);
}

/* ======================================================================== */
/*  TSC2007_ReadTouch                                                        */
/*                                                                           */
/*  Translated from Adafruit_TSC2007::read_touch().                         */
/*                                                                           */
/*  Reads Z1 and Z2 (pressure), then double-samples X and Y to reject       */
/*  noisy readings caused by finger lifting ("pen up flicker").              */
/*  If the two X samples differ by more than 100 counts, or the two Y        */
/*  samples differ by more than 100 counts, the read is rejected.            */
/*  Re-arms the TIRQ interrupt at the end via POWERDOWN_IRQON.              */
/* ======================================================================== */

bool TSC2007_ReadTouch(uint16_t *x, uint16_t *y,
                       uint16_t *z1, uint16_t *z2)
{
    /* Pressure measurements */
    *z1 = TSC2007_Command(TSC2007_MEASURE_Z1, TSC2007_ADON_IRQOFF, TSC2007_ADC_12BIT);

    if (*z1 < TSC2007_MIN_PRESSURE)
	{
		/* Pen is up — re-arm TIRQ and return immediately */
		*z2 = 0U;
		*x  = TSC2007_NO_TOUCH;
		*y  = TSC2007_NO_TOUCH;
		TSC2007_Command(TSC2007_MEASURE_TEMP0, TSC2007_POWERDOWN_IRQON, TSC2007_ADC_12BIT);
		return false;
	}


    *z2 = TSC2007_Command(TSC2007_MEASURE_Z2, TSC2007_ADON_IRQOFF, TSC2007_ADC_12BIT);

    /* Double-sample X and Y to filter pen-up flicker */
    uint16_t x1 = TSC2007_Command(TSC2007_MEASURE_X, TSC2007_ADON_IRQOFF, TSC2007_ADC_12BIT);
    uint16_t y1 = TSC2007_Command(TSC2007_MEASURE_Y, TSC2007_ADON_IRQOFF, TSC2007_ADC_12BIT);
    uint16_t x2 = TSC2007_Command(TSC2007_MEASURE_X, TSC2007_ADON_IRQOFF, TSC2007_ADC_12BIT);
    uint16_t y2 = TSC2007_Command(TSC2007_MEASURE_Y, TSC2007_ADON_IRQOFF, TSC2007_ADC_12BIT);

    /* Re-arm TIRQ so the next touch fires another interrupt */
    TSC2007_Command(TSC2007_MEASURE_TEMP0, TSC2007_POWERDOWN_IRQON, TSC2007_ADC_12BIT);

    /* Reject if the two samples are too far apart (finger moving/lifting) */
    if (abs((int32_t)x1 - (int32_t)x2) > 100) { return false; }
    if (abs((int32_t)y1 - (int32_t)y2) > 100) { return false; }

    *x = x1;
    *y = y1;

    /* 4095 on both axes = no touch (ADC rail) */
    return (*x != TSC2007_NO_TOUCH) && (*y != TSC2007_NO_TOUCH);
}

/* ======================================================================== */
/*  TSC2007_GetPoint                                                         */
/* ======================================================================== */

TSC2007_Point_t TSC2007_GetPoint(void)
{
    TSC2007_Point_t pt = {0, 0, 0};

    /* Clear the flag regardless of whether the read succeeds */
    s_touch_flag = false;

    uint16_t x = 0U, y = 0U, z1 = 0U, z2 = 0U;
    if (TSC2007_ReadTouch(&x, &y, &z1, &z2))
    {
        pt.x = (int16_t)x;
        pt.y = (int16_t)y;
        pt.z = (int16_t)z1;
    }

    return pt;
}
