/**
 * @file    TSC2007.h
 * @brief   STM32 HAL driver for the TSC2007 resistive touch controller.
 *          Translated from Adafruit_TSC2007 (C++ / Arduino) to plain C.
 *
 * Original Adafruit library:
 *   Copyright (c) Adafruit Industries. All rights reserved. BSD license.
 *
 * Hardware target:
 *   Board   : STM32 Nucleo-L152RE
 *   Display : Adafruit 2.8" TFT Touch Shield v2 (TSC2007 + ILI9341)
 *   Bus     : I2C1 (PB8=SCL, PB9=SDA) via STM32 HAL
 *
 * Pin mapping:
 *   TSC2007 TIRQ → Arduino D2 → Nucleo-L152RE PA10
 *                  (EXTI line 10, falling-edge interrupt, internal pull-up)
 *
 * ── How the interrupt works ──────────────────────────────────────────────
 *  The TSC2007 TIRQ line is open-drain, active-LOW.  It goes LOW the moment
 *  a finger touches the panel and stays LOW until the host re-arms it by
 *  sending a POWERDOWN_IRQON command over I2C.
 *
 *  This driver uses a falling-edge EXTI interrupt on PA10.  When the edge
 *  is detected, TSC2007_EXTI_Callback() sets a volatile flag and optionally
 *  calls a user-registered callback (ISR context — keep it short).
 *  The main loop then calls TSC2007_IsTouched() / TSC2007_GetPoint().
 *
 * ── Two wiring steps required in your project ────────────────────────────
 *
 *  STEP 1 — Add PA10 GPIO init inside MX_GPIO_Init() in main.c
 *           (paste inside the USER CODE sections so CubeMX won't erase it):
 *
 *    // TSC2007 TIRQ — D2 — PA10 — falling edge, internal pull-up
 *    GPIO_InitStruct.Pin  = GPIO_PIN_10;
 *    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
 *    GPIO_InitStruct.Pull = GPIO_PULLUP;
 *    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
 *    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);  // priority 1, sub 0
 *    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
 *
 *  STEP 2 — Wire TSC2007_EXTI_Callback() into stm32l1xx_it.c:
 *
 *    At the top of stm32l1xx_it.c, inside the USER CODE Includes section:
 *      #include "TSC2007.h"
 *
 *    Then inside EXTI15_10_IRQHandler, before HAL_GPIO_EXTI_IRQHandler():
 *      void EXTI15_10_IRQHandler(void) {
 *          // USER CODE BEGIN EXTI15_10_IRQn 0
 *          TSC2007_EXTI_Callback();
 *          // USER CODE END EXTI15_10_IRQn 0
 *          HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
 *      }
 *
 *    Note: if EXTI15_10_IRQHandler does not already exist in stm32l1xx_it.c,
 *    add the whole function shown above.
 * ─────────────────────────────────────────────────────────────────────────
 */

#ifndef TSC2007_H
#define TSC2007_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ======================================================================== */
/*  I2C configuration                                                        */
/* ======================================================================== */

extern I2C_HandleTypeDef hi2c1;
#define TSC2007_I2C_HANDLE    hi2c1
/** 7-bit address 0x48, shifted left 1 for STM32 HAL (which expects 8-bit) */
#define TSC2007_I2C_ADDR      (0x48U << 1U)
#define TSC2007_I2C_TIMEOUT   10U             /**< HAL I2C timeout ms       */

/* ======================================================================== */
/*  TIRQ GPIO pin                                                            */
/* ======================================================================== */

#define TSC2007_TIRQ_PORT     GPIOA
#define TSC2007_TIRQ_PIN      GPIO_PIN_10    /**< Arduino D2 → PA10         */

/* ======================================================================== */
/*  Saturation value — raw ADC reading when no touch present                */
/* ======================================================================== */

#define TSC2007_NO_TOUCH      4095U

/* ======================================================================== */
/*  Command field enumerations                                               */
/*                                                                           */
/*  Command byte layout:                                                     */
/*    [7:4] = function   [3:2] = power mode   [1] = resolution   [0] = 0   */
/* ======================================================================== */

typedef enum {
    TSC2007_MEASURE_TEMP0    = 0,
    TSC2007_MEASURE_AUX      = 2,
    TSC2007_MEASURE_TEMP1    = 4,
    TSC2007_ACTIVATE_X       = 8,
    TSC2007_ACTIVATE_Y       = 9,
    TSC2007_ACTIVATE_YPLUS_X = 10,
    TSC2007_SETUP_CMD        = 11,
    TSC2007_MEASURE_X        = 12,
    TSC2007_MEASURE_Y        = 13,
    TSC2007_MEASURE_Z1       = 14,
    TSC2007_MEASURE_Z2       = 15,
} TSC2007_Function_t;

typedef enum {
    TSC2007_POWERDOWN_IRQON = 0,  /**< ADC off, TIRQ line re-armed          */
    TSC2007_ADON_IRQOFF     = 1,  /**< ADC on,  TIRQ disabled during conv   */
    TSC2007_ADOFF_IRQON     = 2,  /**< ADC off, TIRQ armed                  */
} TSC2007_Power_t;

typedef enum {
    TSC2007_ADC_12BIT = 0,
    TSC2007_ADC_8BIT  = 1,
} TSC2007_Resolution_t;

/* ======================================================================== */
/*  Touch point                                                              */
/* ======================================================================== */

typedef struct {
    int16_t x;   /**< Raw X ADC value  0–4095  (0 = left  side of panel)   */
    int16_t y;   /**< Raw Y ADC value  0–4095  (0 = top   side of panel)   */
    int16_t z;   /**< Pressure (z1 value; 0 = no touch, larger = harder)   */
} TSC2007_Point_t;

/* ======================================================================== */
/*  User callback type                                                       */
/*  Called from ISR context — must be very short (set a flag, etc.)         */
/* ======================================================================== */
typedef void (*TSC2007_TouchCallback_t)(void);

/* ======================================================================== */
/*  Public API                                                               */
/* ======================================================================== */

/**
 * @brief  Initialise the TSC2007.
 *         Sends the initial POWERDOWN_IRQON command over I2C so the TIRQ
 *         line is armed before the first touch.
 *         Call after all MX_xxx_Init() functions in main.c.
 * @return true  if the I2C transaction succeeded.
 * @return false if the device did not ACK (check wiring / pull-ups).
 */
bool TSC2007_Init(void);

/**
 * @brief  Register an optional callback invoked from the EXTI ISR.
 *         The callback fires every time TIRQ goes LOW (new touch detected).
 *         Pass NULL to unregister.
 *
 * @note   Runs in INTERRUPT context.  Do NOT call TSC2007_ReadTouch() or
 *         any blocking HAL function from inside this callback.
 *         Recommended use: set a volatile flag that the main loop checks.
 */
void TSC2007_RegisterCallback(TSC2007_TouchCallback_t cb);

/**
 * @brief  Returns true if a touch interrupt has fired since the last
 *         TSC2007_ClearFlag() or TSC2007_GetPoint() call.
 *         Non-blocking; safe to call from the main loop at any rate.
 */
bool TSC2007_IsTouched(void);

/**
 * @brief  Discard the pending touch flag without reading I2C data.
 *         Use this to ignore a touch event intentionally.
 */
void TSC2007_ClearFlag(void);

/**
 * @brief  Send one command to the TSC2007 and return the 12-bit result.
 *         Waits 500 µs for the ADC conversion (per datasheet).
 * @return 12-bit ADC value, or 0 on I2C error.
 */
uint16_t TSC2007_Command(TSC2007_Function_t   func,
                         TSC2007_Power_t      pwr,
                         TSC2007_Resolution_t res);

/**
 * @brief  Full touch read: X, Y, Z1, Z2.
 *         Performs double X/Y sampling and rejects unstable readings
 *         (delta > 100 counts between the two samples = finger lifting).
 *         Re-arms the TIRQ interrupt when done.
 *
 * @param  x   Raw X value (0–4095).
 * @param  y   Raw Y value (0–4095).
 * @param  z1  Z1 pressure measurement.
 * @param  z2  Z2 pressure measurement.
 * @return true  on a valid, stable touch.
 * @return false if no touch, unstable, or I2C error.
 */
bool TSC2007_ReadTouch(uint16_t *x, uint16_t *y,
                       uint16_t *z1, uint16_t *z2);

/**
 * @brief  Convenience wrapper — reads touch data, clears the flag, and
 *         returns a TSC2007_Point_t.  All fields are 0 if read failed.
 *
 * Typical main-loop usage:
 * @code
 *   if (TSC2007_IsTouched()) {
 *       TSC2007_Point_t p = TSC2007_GetPoint();
 *       if (p.z > 0) {
 *           // valid touch at raw (p.x, p.y)
 *       }
 *   }
 * @endcode
 */
TSC2007_Point_t TSC2007_GetPoint(void);

/**
 * @brief  MUST be called from EXTI15_10_IRQHandler in stm32l1xx_it.c.
 *         Sets the internal touch flag and invokes the user callback.
 *         See the file header for the exact two-step wiring instructions.
 */
void TSC2007_EXTI_Callback(void);

#ifdef __cplusplus
}
#endif

#endif /* TSC2007_H */
