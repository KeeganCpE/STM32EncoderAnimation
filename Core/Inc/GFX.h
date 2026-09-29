/**
 * @file    GFX.h
 * @brief   Core graphics library for ILI9341 on STM32 Nucleo-L152RE via HAL.
 *          Translated and adapted from Adafruit_GFX (C++ / Arduino) to plain C.
 *
 * Original Adafruit_GFX library:
 *   Copyright (c) 2013 Adafruit Industries. All rights reserved.
 *   BSD license — see source for full text.
 *
 * Hardware target:
 *   Board   : STM32 Nucleo-L152RE
 *   Display : Adafruit 2.8" TFT Touch Shield v2 (ILI9341 + TSC2007)
 *   Bus     : SPI1 via STM32 HAL
 *
 * Pin mapping (Arduino shield header → Nucleo-L152RE):
 *   D13 SCK  → PA5  (SPI1_SCK,  AF5)
 *   D12 MISO → PA6  (SPI1_MISO, AF5)
 *   D11 MOSI → PA7  (SPI1_MOSI, AF5)
 *   D10 CS   → PB6  (GPIO out, active-LOW)
 *   D9  DC   → PC7  (GPIO out, LOW=cmd HIGH=data)
 *   D8  RST  → PA9  (GPIO out, active-LOW)
 *   D3  LED  → PB3  (GPIO out, HIGH=backlight on)
 */

#ifndef GFX_H
#define GFX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l1xx_hal.h"
#include "gfxfont.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ======================================================================== */
/*  SPI / GPIO pin configuration                                             */
/* ======================================================================== */

extern SPI_HandleTypeDef hspi1;
#define ILI9341_SPI_HANDLE    hspi1
#define ILI9341_SPI_TIMEOUT   100U

#define ILI9341_CS_PORT       GPIOB
#define ILI9341_CS_PIN        GPIO_PIN_6   /* D10 */

#define ILI9341_DC_PORT       GPIOC
#define ILI9341_DC_PIN        GPIO_PIN_7   /* D9  */

#define ILI9341_RST_PORT      GPIOA
#define ILI9341_RST_PIN       GPIO_PIN_9   /* D8  */

#define ILI9341_LED_PORT      GPIOB
#define ILI9341_LED_PIN       GPIO_PIN_3   /* D3  */

/* ======================================================================== */
/*  ILI9341 geometry                                                         */
/* ======================================================================== */

#define ILI9341_TFTWIDTH    240U
#define ILI9341_TFTHEIGHT   320U

/* ======================================================================== */
/*  ILI9341 command codes                                                    */
/* ======================================================================== */

#define ILI9341_NOP         0x00
#define ILI9341_SWRESET     0x01
#define ILI9341_RDDID       0x04
#define ILI9341_RDDST       0x09
#define ILI9341_SLPIN       0x10
#define ILI9341_SLPOUT      0x11
#define ILI9341_PTLON       0x12
#define ILI9341_NORON       0x13
#define ILI9341_RDMODE      0x0A
#define ILI9341_RDMADCTL    0x0B
#define ILI9341_RDPIXFMT    0x0C
#define ILI9341_RDIMGFMT    0x0D
#define ILI9341_RDSELFDIAG  0x0F
#define ILI9341_INVOFF      0x20
#define ILI9341_INVON       0x21
#define ILI9341_GAMMASET    0x26
#define ILI9341_DISPOFF     0x28
#define ILI9341_DISPON      0x29
#define ILI9341_CASET       0x2A
#define ILI9341_PASET       0x2B
#define ILI9341_RAMWR       0x2C
#define ILI9341_RAMRD       0x2E
#define ILI9341_PTLAR       0x30
#define ILI9341_MADCTL      0x36
#define ILI9341_VSCRSADD    0x37
#define ILI9341_PIXFMT      0x3A
#define ILI9341_FRMCTR1     0xB1
#define ILI9341_FRMCTR2     0xB2
#define ILI9341_FRMCTR3     0xB3
#define ILI9341_INVCTR      0xB4
#define ILI9341_DFUNCTR     0xB6
#define ILI9341_PWCTR1      0xC0
#define ILI9341_PWCTR2      0xC1
#define ILI9341_PWCTR3      0xC2
#define ILI9341_PWCTR4      0xC3
#define ILI9341_PWCTR5      0xC4
#define ILI9341_VMCTR1      0xC5
#define ILI9341_VMCTR2      0xC7
#define ILI9341_RDID1       0xDA
#define ILI9341_RDID2       0xDB
#define ILI9341_RDID3       0xDC
#define ILI9341_RDID4       0xDD
#define ILI9341_GMCTRP1     0xE0
#define ILI9341_GMCTRN1     0xE1

#define MADCTL_MY    0x80
#define MADCTL_MX    0x40
#define MADCTL_MV    0x20
#define MADCTL_ML    0x10
#define MADCTL_RGB   0x00
#define MADCTL_BGR   0x08
#define MADCTL_MH    0x04

/* ======================================================================== */
/*  Classic font geometry (5×7 px glyph, glcdfont.c)                       */
/* ======================================================================== */

#define GFX_FONT_WIDTH       5U
#define GFX_FONT_HEIGHT      7U
#define GFX_CHAR_ADVANCE_X   6U   /* 5 px glyph + 1 px gap */
#define GFX_CHAR_ADVANCE_Y   8U   /* 7 px glyph + 1 px gap */

/* ======================================================================== */
/*  RGB565 colour constants                                                  */
/* ======================================================================== */

#define GFX_BLACK        ((uint16_t)0x0000)
#define GFX_NAVY         ((uint16_t)0x000F)
#define GFX_DARKGREEN    ((uint16_t)0x03E0)
#define GFX_DARKCYAN     ((uint16_t)0x03EF)
#define GFX_MAROON       ((uint16_t)0x7800)
#define GFX_PURPLE       ((uint16_t)0x780F)
#define GFX_OLIVE        ((uint16_t)0x7BE0)
#define GFX_LIGHTGREY    ((uint16_t)0xC618)
#define GFX_DARKGREY     ((uint16_t)0x7BEF)
#define GFX_BLUE         ((uint16_t)0x001F)
#define GFX_GREEN        ((uint16_t)0x07E0)
#define GFX_CYAN         ((uint16_t)0x07FF)
#define GFX_RED          ((uint16_t)0xF800)
#define GFX_MAGENTA      ((uint16_t)0xF81F)
#define GFX_YELLOW       ((uint16_t)0xFFE0)
#define GFX_WHITE        ((uint16_t)0xFFFF)
#define GFX_ORANGE       ((uint16_t)0xFD20)
#define GFX_GREENYELLOW  ((uint16_t)0xAFE5)
#define GFX_PINK         ((uint16_t)0xFC18)

#define GFX_COLOR565(r, g, b) \
    ((uint16_t)( (((uint16_t)(r) & 0xF8U) << 8U) | \
                 (((uint16_t)(g) & 0xFCU) << 3U) | \
                 ((uint16_t)(b) >> 3U) ))

/* ======================================================================== */
/*  GFX context                                                              */
/* ======================================================================== */

typedef struct {
    int16_t  WIDTH;       /**< Raw panel width  (fixed at init)              */
    int16_t  HEIGHT;      /**< Raw panel height (fixed at init)              */
    int16_t  _width;      /**< Effective width  after rotation               */
    int16_t  _height;     /**< Effective height after rotation               */
    uint8_t  rotation;    /**< 0–3                                           */

    int16_t  cursor_x;    /**< Text cursor X                                 */
    int16_t  cursor_y;    /**< Text cursor Y                                 */
    uint16_t textcolor;   /**< Foreground colour                             */
    uint16_t textbgcolor; /**< Background colour (== textcolor → transparent)*/
    uint8_t  textsize;    /**< Scale multiplier: 1 = native 5×7             */
    uint8_t  wrap;        /**< Non-zero: auto-wrap at right edge             */
} GFX_t;

/* ======================================================================== */
/*  Public API — display control                                             */
/* ======================================================================== */

/** Initialise display hardware and GFX context. Call after all MX_Init(). */
void GFX_Init(GFX_t *gfx);

/** Set rotation 0–3 (0=portrait, 1=landscape CW, 2=portrait inv, 3=landscape CCW). */
void GFX_SetRotation(GFX_t *gfx, uint8_t r);

static inline int16_t GFX_Width (const GFX_t *gfx) { return gfx->_width;  }
static inline int16_t GFX_Height(const GFX_t *gfx) { return gfx->_height; }

/* ======================================================================== */
/*  Public API — pixel / fill                                                */
/* ======================================================================== */

/** Draw one pixel. */
void GFX_DrawPixel(GFX_t *gfx, int16_t x, int16_t y, uint16_t color);

/** Fill a solid rectangle given top-left corner, width, and height. */
void GFX_FillRect(GFX_t *gfx, int16_t x, int16_t y,
                  int16_t w, int16_t h, uint16_t color);

/** Fill the entire screen with one colour. */
void GFX_FillScreen(GFX_t *gfx, uint16_t color);

/* ======================================================================== */
/*  Public API — lines and shapes                                            */
/* ======================================================================== */

/**
 * @brief  Draw a straight line between any two points.
 *         Uses Bresenham's algorithm — works at any angle.
 * @param  x0, y0  Start point.
 * @param  x1, y1  End point.
 * @param  color   RGB565 colour.
 */
void GFX_WriteLine(GFX_t *gfx,
                   int16_t x0, int16_t y0,
                   int16_t x1, int16_t y1,
                   uint16_t color);

/**
 * @brief  Draw an arbitrary four-sided closed shape (quadrilateral).
 *         Connects four corner points in order: 0→1→2→3→0.
 *         Corners do NOT need to be axis-aligned — you can draw
 *         parallelograms, diamonds, or any four-sided polygon.
 * @param  x0,y0 … x3,y3  The four corners in drawing order.
 * @param  color            RGB565 colour.
 */
void GFX_WriteRect(GFX_t *gfx,
                   int16_t x0, int16_t y0,
                   int16_t x1, int16_t y1,
                   int16_t x2, int16_t y2,
                   int16_t x3, int16_t y3,
                   uint16_t color);

/**
 * @brief  Draw an axis-aligned rectangle outline (unfilled).
 *         Pass two opposite corners.  For a solid rectangle use GFX_FillRect.
 * @param  x0, y0  One corner.
 * @param  x1, y1  Opposite corner.
 * @param  color   RGB565 colour.
 */
void GFX_WritePerfRect(GFX_t *gfx,
                       int16_t x0, int16_t y0,
                       int16_t x1, int16_t y1,
                       uint16_t color);

/* ======================================================================== */
/*  Public API — text                                                        */
/* ======================================================================== */

/** Set text cursor (top-left corner of next character). */
void GFX_SetCursor(GFX_t *gfx, int16_t x, int16_t y);

/** Set foreground text colour; background is transparent. */
void GFX_SetTextColor(GFX_t *gfx, uint16_t color);

/**
 * @brief  Set foreground AND background text colour.
 *         When fg != bg every pixel in the character cell is drawn —
 *         faster and avoids artefacts over coloured backgrounds.
 */
void GFX_SetTextColorBG(GFX_t *gfx, uint16_t color, uint16_t bg);

/**
 * @brief  Set text scale multiplier.
 *         1 = native 5×7 px, 2 = 10×14 px, 3 = 15×21 px.
 */
void GFX_SetTextSize(GFX_t *gfx, uint8_t size);

/** Enable/disable auto-wrap at the right edge (default: enabled). */
void GFX_SetTextWrap(GFX_t *gfx, uint8_t w);

/**
 * @brief  Draw one character at an explicit position.
 *         Does NOT move the cursor.
 */
void GFX_DrawChar(GFX_t *gfx,
                  int16_t x, int16_t y, unsigned char c,
                  uint16_t color, uint16_t bg, uint8_t size);

/**
 * @brief  Print a null-terminated string at the current cursor.
 *         Uses textcolor, textbgcolor, textsize and wrap from the context.
 *         Handles '\\n'; advances cursor after each character.
 */
void GFX_Print(GFX_t *gfx, const char *str);

/**
 * @brief  Convenience one-liner: position + colour + size + print.
 *
 * Example:
 *   GFX_DrawString(&display, 10, 20, "Hello!", GFX_WHITE, GFX_BLACK, 2);
 */
void GFX_DrawString(GFX_t *gfx,
                    int16_t x, int16_t y,
                    const char *str,
                    uint16_t color, uint16_t bg,
                    uint8_t size);

void fill_dma_buffer(uint16_t color);

void GFX_FillRectDMA(GFX_t *gfx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/* ======================================================================== */
/*  Low-level ILI9341 helpers                                               */
/* ======================================================================== */

void ILI9341_CS_Assert(void);
void ILI9341_CS_Deassert(void);
void ILI9341_DC_Command(void);
void ILI9341_DC_Data(void);
void ILI9341_WriteCommand(uint8_t cmd);
void ILI9341_WriteData8(uint8_t data);
void ILI9341_WriteData16(uint16_t data);
void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0,
                              uint16_t x1, uint16_t y1);

#ifdef __cplusplus
}
#endif

#endif /* GFX_H */
