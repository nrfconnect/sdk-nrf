/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/*!
* @file Adafruit_ILI9341.cpp
*
* @mainpage Adafruit ILI9341 TFT Displays
*
* @section intro_sec Introduction
*
* This is the documentation for Adafruit's ILI9341 driver for the
* Arduino platform.
*
* This library works with the Adafruit 2.8" Touch Shield V2 (SPI)
*    http://www.adafruit.com/products/1651
*
* Adafruit 2.4" TFT LCD with Touchscreen Breakout w/MicroSD Socket - ILI9341
*    https://www.adafruit.com/product/2478
*
* 2.8" TFT LCD with Touchscreen Breakout Board w/MicroSD Socket - ILI9341
*    https://www.adafruit.com/product/1770
*
* 2.2" 18-bit color TFT LCD display with microSD card breakout - ILI9340
*    https://www.adafruit.com/product/1770
*
* TFT FeatherWing - 2.4" 320x240 Touchscreen For All Feathers
*    https://www.adafruit.com/product/3315
*
* These displays use SPI to communicate, 4 or 5 pins are required
* to interface (RST is optional).
*
* Adafruit invests time and resources providing this open source code,
* please support Adafruit and open-source hardware by purchasing
* products from Adafruit!
*
* @section dependencies Dependencies
*
* This library depends on <a href="https://github.com/adafruit/Adafruit_GFX">
* Adafruit_GFX</a> being present on your system. Please make sure you have
* installed the latest version before using this library.
*
* @section author Author
*
* Written by Limor "ladyada" Fried for Adafruit Industries.
*
* @section license License
*
* BSD license, all text here must be included in any redistribution.
*
*/

#include <zephyr.h>
#include "Adafruit_ILI9341.h"

#define MADCTL_MY  0x80  /* < Bottom to top              */
#define MADCTL_MX  0x40  /* < Right to left              */
#define MADCTL_MV  0x20  /* < Reverse Mode               */
#define MADCTL_ML  0x10  /* < LCD refresh Bottom to top  */
#define MADCTL_RGB 0x00  /* < Red-Green-Blue pixel order */
#define MADCTL_BGR 0x08  /* < Blue-Green-Red pixel order */
#define MADCTL_MH  0x04  /* < LCD refresh right to left  */

const u8_t Adafruit_ILI9341_initcmd[] = {
/*	ILI9341_SWRESET, 0x40, */              /* Software reset */
	0xEF, 3, 0x03, 0x80, 0x02,
	0xCF, 3, 0x00, 0xC1, 0x30,
	0xED, 4, 0x64, 0x03, 0x12, 0x81,
	0xE8, 3, 0x85, 0x00, 0x78,
	0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
	0xF7, 1, 0x20,
	0xEA, 2, 0x00, 0x00,
	ILI9341_PWCTR1,   1, 0x23,             /* Power control VRH[5:0] */
	ILI9341_PWCTR2,   1, 0x10,             /* Power control SAP[2:0];BT[3:0] */
	ILI9341_VMCTR1,   2, 0x3e, 0x28,       /* VCM control */
	ILI9341_VMCTR2,   1, 0x86,             /* VCM control2 */
/*	ILI9341_MADCTL,   1, 0x48, */          /* Memory Access Control */
	ILI9341_MADCTL,   1, 0x38,             /* Memory Access Control */
	ILI9341_VSCRSADD, 1, 0x00,             /* Vertical scroll zero */
	ILI9341_PIXFMT,   1, 0x55,
	ILI9341_FRMCTR1,  2, 0x00, 0x18,
	ILI9341_DFUNCTR,  3, 0x08, 0x82, 0x27, /* Display Function Control */
	0xF2, 1, 0x00,                         /* 3Gamma Function Disable */
	ILI9341_GAMMASET, 1, 0x01,             /* Gamma curve selected */
	ILI9341_GMCTRP1, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, /* Set Gamma */
		0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
	ILI9341_GMCTRN1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, /* Set Gamma */
		0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
	ILI9341_SLPOUT,  0x80,                 /* Exit Sleep */
/*	ILI9341_DISPON,  0x80, */              /* Display on */
	ILI9341_DISPOFF, 0x00,                 /* Display off */
	ILI9341_NOP                            /* End of list */
};
