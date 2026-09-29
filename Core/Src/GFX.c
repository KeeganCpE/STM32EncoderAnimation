/**
 * @file    GFX.c
 * @brief   Core graphics library for ILI9341 on STM32 Nucleo-L152RE via HAL.
 *          Translated and adapted from Adafruit_GFX (C++ / Arduino) to plain C.
 *
 * Original Adafruit_GFX library:
 *   Copyright (c) 2013 Adafruit Industries. All rights reserved.  BSD license.
 *
 * glcdfont.c must be present in the same source directory.
 * gfxfont.h  must be present in the same include directory.
 */

#include "GFX.h"
#include "glcdfont.c"   /* static const unsigned char font[] */
#include <stdlib.h>     /* abs() */

#define DMA_CHUNK_PIXELS 256
extern SPI_HandleTypeDef hspi1;

/* On Cortex-M all flash is directly readable. */
#ifndef pgm_read_byte
#define pgm_read_byte(addr)  (*(const uint8_t *)(addr))
#endif

uint16_t dma_buf[DMA_CHUNK_PIXELS];


/* ======================================================================== */
/*  GPIO helpers                                                             */
/* ======================================================================== */

void ILI9341_CS_Assert(void)
{
    HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_RESET);
}

void ILI9341_CS_Deassert(void)
{
    HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_SET);
}

void ILI9341_DC_Command(void)
{
    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_RESET);
}

void ILI9341_DC_Data(void)
{
    HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_SET);
}

/* ======================================================================== */
/*  SPI transfers                                                            */
/* ======================================================================== */

static void SPI_Write8(uint8_t byte)
{
    HAL_SPI_Transmit(&ILI9341_SPI_HANDLE, &byte, 1U, ILI9341_SPI_TIMEOUT);
}

static void SPI_Write16(uint16_t word)
{
    SPI_HandleTypeDef *h = &ILI9341_SPI_HANDLE;

    if (h->Init.DataSize == SPI_DATASIZE_16BIT) {
        uint16_t w = word;               // ensure 16-bit aligned variable
        HAL_SPI_Transmit(h, (uint8_t *)&w, 1U, ILI9341_SPI_TIMEOUT);
    } else {
        uint8_t buf[2];
        buf[0] = (uint8_t)(word >> 8U);
        buf[1] = (uint8_t)(word & 0xFFU);
        HAL_SPI_Transmit(h, buf, 2U, ILI9341_SPI_TIMEOUT);
    }
}

/* ======================================================================== */
/*  ILI9341 command / data wrappers                                          */
/* ======================================================================== */

void ILI9341_WriteCommand(uint8_t cmd)
{
    ILI9341_DC_Command();
    SPI_Write8(cmd);
}

void ILI9341_WriteData8(uint8_t data)
{
    ILI9341_DC_Data();
    SPI_Write8(data);
}

void ILI9341_WriteData16(uint16_t data)
{
    ILI9341_DC_Data();
    SPI_Write16(data);
}

/* ======================================================================== */
/*  Address window                                                           */
/* ======================================================================== */

void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0,
                              uint16_t x1, uint16_t y1)
{
    ILI9341_WriteCommand(ILI9341_CASET);
    ILI9341_WriteData16(x0);
    ILI9341_WriteData16(x1);

    ILI9341_WriteCommand(ILI9341_PASET);
    ILI9341_WriteData16(y0);
    ILI9341_WriteData16(y1);

    ILI9341_WriteCommand(ILI9341_RAMWR);
}

/* ======================================================================== */
/*  Hardware + software reset                                                */
/* ======================================================================== */

static void ILI9341_HardwareReset(void)
{
    HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(5U);
    HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(20U);
    HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(150U);
}

/* ======================================================================== */
/*  Register init sequence (matches Adafruit_ILI9341 library byte-for-byte) */
/* ======================================================================== */

static void ILI9341_InitRegisters(void)
{
    ILI9341_WriteCommand(ILI9341_SWRESET);
    HAL_Delay(150U);

    ILI9341_WriteCommand(0xEF);
    ILI9341_WriteData8(0x03);
    ILI9341_WriteData8(0x80);
    ILI9341_WriteData8(0x02);

    ILI9341_WriteCommand(0xCF);
    ILI9341_WriteData8(0x00);
    ILI9341_WriteData8(0xC1);
    ILI9341_WriteData8(0x30);

    ILI9341_WriteCommand(0xED);
    ILI9341_WriteData8(0x64);
    ILI9341_WriteData8(0x03);
    ILI9341_WriteData8(0x12);
    ILI9341_WriteData8(0x81);

    ILI9341_WriteCommand(0xE8);
    ILI9341_WriteData8(0x85);
    ILI9341_WriteData8(0x00);
    ILI9341_WriteData8(0x78);

    ILI9341_WriteCommand(0xCB);
    ILI9341_WriteData8(0x39);
    ILI9341_WriteData8(0x2C);
    ILI9341_WriteData8(0x00);
    ILI9341_WriteData8(0x34);
    ILI9341_WriteData8(0x02);

    ILI9341_WriteCommand(0xF7);
    ILI9341_WriteData8(0x20);

    ILI9341_WriteCommand(0xEA);
    ILI9341_WriteData8(0x00);
    ILI9341_WriteData8(0x00);

    ILI9341_WriteCommand(ILI9341_PWCTR1);
    ILI9341_WriteData8(0x23);

    ILI9341_WriteCommand(ILI9341_PWCTR2);
    ILI9341_WriteData8(0x10);

    ILI9341_WriteCommand(ILI9341_VMCTR1);
    ILI9341_WriteData8(0x3E);
    ILI9341_WriteData8(0x28);

    ILI9341_WriteCommand(ILI9341_VMCTR2);
    ILI9341_WriteData8(0x86);

    ILI9341_WriteCommand(ILI9341_MADCTL);
    ILI9341_WriteData8(MADCTL_MX | MADCTL_BGR);

    ILI9341_WriteCommand(ILI9341_VSCRSADD);
    ILI9341_WriteData8(0x00);

    ILI9341_WriteCommand(ILI9341_PIXFMT);
    ILI9341_WriteData8(0x55);

    ILI9341_WriteCommand(ILI9341_FRMCTR1);
    ILI9341_WriteData8(0x00);
    ILI9341_WriteData8(0x18);

    ILI9341_WriteCommand(ILI9341_DFUNCTR);
    ILI9341_WriteData8(0x08);
    ILI9341_WriteData8(0x82);
    ILI9341_WriteData8(0x27);

    ILI9341_WriteCommand(0xF2);
    ILI9341_WriteData8(0x00);

    ILI9341_WriteCommand(ILI9341_GAMMASET);
    ILI9341_WriteData8(0x01);

    ILI9341_WriteCommand(ILI9341_GMCTRP1);
    ILI9341_WriteData8(0x0F); ILI9341_WriteData8(0x31);
    ILI9341_WriteData8(0x2B); ILI9341_WriteData8(0x0C);
    ILI9341_WriteData8(0x0E); ILI9341_WriteData8(0x08);
    ILI9341_WriteData8(0x4E); ILI9341_WriteData8(0xF1);
    ILI9341_WriteData8(0x37); ILI9341_WriteData8(0x07);
    ILI9341_WriteData8(0x10); ILI9341_WriteData8(0x03);
    ILI9341_WriteData8(0x0E); ILI9341_WriteData8(0x09);
    ILI9341_WriteData8(0x00);

    ILI9341_WriteCommand(ILI9341_GMCTRN1);
    ILI9341_WriteData8(0x00); ILI9341_WriteData8(0x0E);
    ILI9341_WriteData8(0x14); ILI9341_WriteData8(0x03);
    ILI9341_WriteData8(0x11); ILI9341_WriteData8(0x07);
    ILI9341_WriteData8(0x31); ILI9341_WriteData8(0xC1);
    ILI9341_WriteData8(0x48); ILI9341_WriteData8(0x08);
    ILI9341_WriteData8(0x0F); ILI9341_WriteData8(0x0C);
    ILI9341_WriteData8(0x31); ILI9341_WriteData8(0x36);
    ILI9341_WriteData8(0x0F);

    ILI9341_WriteCommand(ILI9341_SLPOUT);
    HAL_Delay(120U);

    ILI9341_WriteCommand(ILI9341_DISPON);
    HAL_Delay(20U);
}

/* ======================================================================== */
/*  GFX_Init                                                                 */
/* ======================================================================== */

void GFX_Init(GFX_t *gfx)
{
    gfx->WIDTH       = (int16_t)ILI9341_TFTWIDTH;
    gfx->HEIGHT      = (int16_t)ILI9341_TFTHEIGHT;
    gfx->_width      = gfx->WIDTH;
    gfx->_height     = gfx->HEIGHT;
    gfx->rotation    = 0U;

    gfx->cursor_x    = 0;
    gfx->cursor_y    = 0;
    gfx->textcolor   = GFX_WHITE;
    gfx->textbgcolor = GFX_WHITE;
    gfx->textsize    = 1U;
    gfx->wrap        = 1U;

    ILI9341_CS_Deassert();
    HAL_GPIO_WritePin(ILI9341_LED_PORT, ILI9341_LED_PIN, GPIO_PIN_SET);
    ILI9341_HardwareReset();
    ILI9341_CS_Assert();
    ILI9341_InitRegisters();
    ILI9341_CS_Deassert();
}

/* ======================================================================== */
/*  GFX_SetRotation                                                          */
/* ======================================================================== */

void GFX_SetRotation(GFX_t *gfx, uint8_t r)
{
    gfx->rotation = r & 0x03U;

    ILI9341_CS_Assert();
    ILI9341_WriteCommand(ILI9341_MADCTL);

    switch (gfx->rotation)
    {
        case 0:
            ILI9341_WriteData8(MADCTL_MX | MADCTL_BGR);
            gfx->_width  = gfx->WIDTH;
            gfx->_height = gfx->HEIGHT;
            break;
        case 1:
            ILI9341_WriteData8(MADCTL_MV | MADCTL_BGR);
            gfx->_width  = gfx->HEIGHT;
            gfx->_height = gfx->WIDTH;
            break;
        case 2:
            ILI9341_WriteData8(MADCTL_MY | MADCTL_BGR);
            gfx->_width  = gfx->WIDTH;
            gfx->_height = gfx->HEIGHT;
            break;
        case 3:
            ILI9341_WriteData8(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
            gfx->_width  = gfx->HEIGHT;
            gfx->_height = gfx->WIDTH;
            break;
        default: break;
    }

    ILI9341_CS_Deassert();
}

/* ======================================================================== */
/*  GFX_DrawPixel                                                            */
/* ======================================================================== */

void GFX_DrawPixel(GFX_t *gfx, int16_t x, int16_t y, uint16_t color)
{
    if ((x < 0) || (x >= gfx->_width) || (y < 0) || (y >= gfx->_height))
        return;

    ILI9341_CS_Assert();
    ILI9341_SetAddressWindow((uint16_t)x, (uint16_t)y,
                             (uint16_t)x, (uint16_t)y);
    ILI9341_WriteData16(color);
    ILI9341_CS_Deassert();
}

/* ======================================================================== */
/*  GFX_WriteLine — Bresenham line algorithm                                 */
/*                                                                           */
/*  Draws a straight line between any two arbitrary points.                  */
/*  Works for any angle — horizontal, vertical, or diagonal.                */
/* ======================================================================== */

void GFX_WriteLine(GFX_t *gfx,
                   int16_t x0, int16_t y0,
                   int16_t x1, int16_t y1,
                   uint16_t color)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);

    if (steep) {
        int16_t t;
        t = x0; x0 = y0; y0 = t;
        t = x1; x1 = y1; y1 = t;
    }

    if (x0 > x1) {
        int16_t t;
        t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }

    int16_t dx    = x1 - x0;
    int16_t dy    = abs(y1 - y0);
    int16_t err   = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
        if (steep) {
            GFX_DrawPixel(gfx, y0, x0, color);
        } else {
            GFX_DrawPixel(gfx, x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0  += ystep;
            err += dx;
        }
    }
}

/* ======================================================================== */
/*  GFX_WriteRect — arbitrary four-sided closed shape                        */
/*                                                                           */
/*  Connects four corner points in order with GFX_WriteLine.                */
/*  Corners do NOT need to be axis-aligned, so you can draw parallelograms, */
/*  diamonds, or any quadrilateral.                                          */
/* ======================================================================== */

void GFX_WriteRect(GFX_t *gfx,
                   int16_t x0, int16_t y0,
                   int16_t x1, int16_t y1,
                   int16_t x2, int16_t y2,
                   int16_t x3, int16_t y3,
                   uint16_t color)
{
    GFX_WriteLine(gfx, x0, y0, x1, y1, color);
    GFX_WriteLine(gfx, x1, y1, x2, y2, color);
    GFX_WriteLine(gfx, x2, y2, x3, y3, color);
    GFX_WriteLine(gfx, x3, y3, x0, y0, color);
}

/* ======================================================================== */
/*  GFX_WritePerfRect — axis-aligned rectangle outline                       */
/*                                                                           */
/*  Draws an unfilled rectangle given two opposite corners (x0,y0)-(x1,y1). */
/*  Use GFX_FillRect for a solid filled rectangle.                          */
/* ======================================================================== */

void GFX_WritePerfRect(GFX_t *gfx,
                       int16_t x0, int16_t y0,
                       int16_t x1, int16_t y1,
                       uint16_t color)
{
//    GFX_WriteLine(gfx, x0, y0, x0, y1, color);   /* left   */
//    GFX_WriteLine(gfx, x0, y1, x1, y1, color);   /* bottom */
//    GFX_WriteLine(gfx, x1, y1, x1, y0, color);   /* right  */
//    GFX_WriteLine(gfx, x1, y0, x0, y0, color);   /* top    */
	GFX_FillRectDMA(gfx, x0, y0, x1-x0, y1 - y0, color);
	GFX_FillRectDMA(gfx, x0 + 3, y0 + 3, x1 - x0 - 6, y1 - y0 - 6, GFX_BLACK);
}

/* ======================================================================== */
/*  GFX_FillRect — solid filled rectangle (efficient burst write)           */
/* ======================================================================== */

void GFX_FillRect(GFX_t *gfx,
                  int16_t x, int16_t y, int16_t w, int16_t h,
                  uint16_t color)
{
    if ((x >= gfx->_width) || (y >= gfx->_height) || (w <= 0) || (h <= 0))
        return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if ((x + w) > gfx->_width)  w = gfx->_width  - x;
    if ((y + h) > gfx->_height) h = gfx->_height - y;

    ILI9341_CS_Assert();
    ILI9341_SetAddressWindow((uint16_t)x,          (uint16_t)y,
                             (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));
    ILI9341_DC_Data();

    uint32_t n = (uint32_t)w * (uint32_t)h;
    while (n--) {
    	SPI_Write16(color);
    }

    ILI9341_CS_Deassert();
}

void GFX_FillRectDMA(GFX_t *gfx, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	uint32_t total_pixels = w * h;

	fill_dma_buffer(color);

	// Set the drawing window
	ILI9341_CS_Assert();
	ILI9341_SetAddressWindow(x, y, x + w - 1, y + h - 1);
	ILI9341_DC_Data();

	while (total_pixels > 0) {

		uint32_t chunk = (total_pixels > DMA_CHUNK_PIXELS)
						 ? DMA_CHUNK_PIXELS
						 : total_pixels;

		// Start DMA transfer
		HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)dma_buf, chunk * 2);

		// Wait for DMA complete
		while (hspi1.State != HAL_SPI_STATE_READY) {}

		total_pixels -= chunk;
	}

	ILI9341_CS_Deassert();
}
/* ======================================================================== */
/*  GFX_FillScreen                                                           */
/* ======================================================================== */

void GFX_FillScreen(GFX_t *gfx, uint16_t color)
{
	uint16_t x = 0;
	uint16_t y = 0;

	uint32_t total_pixels = (uint32_t)gfx->_width * (uint32_t)gfx->_height;

	fill_dma_buffer(color);

	// Set the drawing window
	ILI9341_CS_Assert();
	ILI9341_SetAddressWindow(x, y, x + gfx->_width - 1, y + gfx->_height - 1);
	ILI9341_DC_Data();

	while (total_pixels > 0) {

		uint32_t chunk = (total_pixels > DMA_CHUNK_PIXELS)
						 ? DMA_CHUNK_PIXELS
						 : total_pixels;

		// Start DMA transfer
		HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)dma_buf, chunk * 2);

		// Wait for DMA complete
		while (hspi1.State != HAL_SPI_STATE_READY) {}

		total_pixels -= chunk;
	}

	ILI9341_CS_Deassert();
}

void fill_dma_buffer(uint16_t color) {
	uint8_t hi = (uint8_t)(color >> 8U);
	uint8_t lo = (uint8_t)(color & 0xFFU);
	uint16_t swapped = ((uint16_t)lo << 8U) | (uint16_t)hi;
	for (int i = 0; i < DMA_CHUNK_PIXELS; i++) {
		dma_buf[i] = swapped;
	}
}

/* ======================================================================== */
/*  Text helpers                                                             */
/* ======================================================================== */

void GFX_SetCursor(GFX_t *gfx, int16_t x, int16_t y)
{
    gfx->cursor_x = x;
    gfx->cursor_y = y;
}

void GFX_SetTextColor(GFX_t *gfx, uint16_t color)
{
    gfx->textcolor = gfx->textbgcolor = color;
}

void GFX_SetTextColorBG(GFX_t *gfx, uint16_t color, uint16_t bg)
{
    gfx->textcolor   = color;
    gfx->textbgcolor = bg;
}

void GFX_SetTextSize(GFX_t *gfx, uint8_t size)
{
    gfx->textsize = (size > 0U) ? size : 1U;
}

void GFX_SetTextWrap(GFX_t *gfx, uint8_t w)
{
    gfx->wrap = w;
}

/* ======================================================================== */
/*  GFX_DrawChar                                                             */
/* ======================================================================== */

void GFX_DrawChar(GFX_t *gfx,
                  int16_t x, int16_t y, unsigned char c,
                  uint16_t color, uint16_t bg, uint8_t size)
{
    if ((x >= gfx->_width)  ||
        (y >= gfx->_height) ||
        ((x + (int16_t)GFX_FONT_WIDTH  * (int16_t)size) < 0) ||
        ((y + (int16_t)GFX_FONT_HEIGHT * (int16_t)size) < 0))
    {
        return;
    }

    if (c < 0x20U) c = 0x20U;
    if (c > 0x7EU) c = 0x20U;

    uint8_t sz = (size > 0U) ? size : 1U;

    for (int8_t col = 0; col < 6; col++)
    {
        uint8_t line = (col < 5)
                     ? pgm_read_byte(&font[(uint8_t)(c - 0x20U) * 5U + (uint8_t)col])
                     : 0x00U;

        for (int8_t row = 0; row < 8; row++, line >>= 1U)
        {
            uint16_t pixel_color;
            if (line & 0x01U) {
                pixel_color = color;
            } else if (bg != color) {
                pixel_color = bg;
            } else {
                continue;
            }

            int16_t px = x + (int16_t)col * (int16_t)sz;
            int16_t py = y + (int16_t)row * (int16_t)sz;

            if (sz == 1U) {
                GFX_DrawPixel(gfx, px, py, pixel_color);
            } else {
                GFX_FillRect(gfx, px, py, (int16_t)sz, (int16_t)sz, pixel_color);
            }
        }
    }
}

/* ======================================================================== */
/*  GFX_Print                                                                */
/* ======================================================================== */

void GFX_Print(GFX_t *gfx, const char *str)
{
    if (str == NULL) return;

    while (*str)
    {
        unsigned char c = (unsigned char)*str++;

        if (c == '\n') {
            gfx->cursor_x  = 0;
            gfx->cursor_y += (int16_t)GFX_CHAR_ADVANCE_Y * (int16_t)gfx->textsize;
        } else if (c == '\r') {
            /* ignored */
        } else {
            if (gfx->wrap &&
                (gfx->cursor_x + (int16_t)GFX_CHAR_ADVANCE_X * (int16_t)gfx->textsize)
                    > gfx->_width)
            {
                gfx->cursor_x  = 0;
                gfx->cursor_y += (int16_t)GFX_CHAR_ADVANCE_Y * (int16_t)gfx->textsize;
            }
            GFX_DrawChar(gfx,
                         gfx->cursor_x, gfx->cursor_y,
                         c,
                         gfx->textcolor, gfx->textbgcolor,
                         gfx->textsize);
            gfx->cursor_x += (int16_t)GFX_CHAR_ADVANCE_X * (int16_t)gfx->textsize;
        }
    }
}

/* ======================================================================== */
/*  GFX_DrawString — convenience: set cursor/colour/size, then print        */
/* ======================================================================== */

void GFX_DrawString(GFX_t *gfx,
                    int16_t x, int16_t y,
                    const char *str,
                    uint16_t color, uint16_t bg,
                    uint8_t size)
{
    GFX_SetCursor(gfx, x, y);
    GFX_SetTextColorBG(gfx, color, bg);
    GFX_SetTextSize(gfx, size);
    GFX_Print(gfx, str);
}
