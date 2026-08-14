/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *
 * Private register map and driver-internal API for the Texas Instruments
 * TAC5112 low-power stereo audio codec.
 *
 * Register addresses and field values in this file are transcribed from the
 * TAC5112 datasheet (SLASF24A, December 2023 - Revised January 2025), section
 * 8 "Register Maps". The complete page 0, page 1, and page 3 device
 * configuration register map is declared for reference, even where the driver
 * does not use a register yet. Bitfield definitions are only provided for the
 * registers required to implement the Zephyr audio codec interface (configure,
 * start, stop, volume and mute, PLL control, and fault reporting) for the
 * stereo analog path. The multi-channel PDM/TDM extensions of the wider
 * TAx5x1x family remain out of scope for this driver.
 *
 * The declarations in this header are driver-internal and not part of the
 * public Zephyr audio codec API. They may change without notice.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TAC5112_H_
#define ZEPHYR_DRIVERS_AUDIO_TAC5112_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/audio/codec.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ---------------------------------------------------------------------
 * Register addressing
 * ---------------------------------------------------------------------
 */

/**
 * @brief Paged register address.
 *
 * TAC5112 control registers are organized as 256-register "pages" (datasheet
 * Section 7.5, "Programming"). Register 0 of every page is PAGE_CFG and
 * selects which page addresses 1-255 refer to. All registers used by this
 * driver live on page 0 (device, DAC, and ADC control), page 1 (interrupt and
 * fault status), or page 3 (PLL and clock dividers).
 */
struct tac5112_reg {
	uint8_t page; /**< Register page selected through PAGE_CFG. */
	uint8_t addr; /**< Register address within the page. */
};

/** @brief Build a @ref tac5112_reg from a page number and address. */
#define TAC5112_REG(_page, _addr)                                                                  \
	((struct tac5112_reg){.page = (uint8_t)(_page), .addr = (uint8_t)(_addr)})

/**
 * @brief Sentinel marking the page-select cache as invalid.
 *
 * Forces a PAGE_CFG write before the next register access.
 */
#define TAC5112_PAGE_CACHE_INVALID 0xFFU

/* ---- Page 0: device, ASI, clock, ADC, DAC, and status registers ---- */
#define TAC5112_REG_PAGE_CFG	   TAC5112_REG(0, 0x00)
#define TAC5112_REG_SW_RESET	   TAC5112_REG(0, 0x01)
#define TAC5112_REG_DEV_MISC_CFG   TAC5112_REG(0, 0x02)
#define TAC5112_REG_AVDD_IOVDD_STS TAC5112_REG(0, 0x03)

#define TAC5112_REG_MISC_CFG	     TAC5112_REG(0, 0x04)
#define TAC5112_REG_MISC_CFG1	     TAC5112_REG(0, 0x05)
#define TAC5112_REG_DAC_CFG_A0	     TAC5112_REG(0, 0x06)
#define TAC5112_REG_MISC_CFG0	     TAC5112_REG(0, 0x0A)
#define TAC5112_REG_GPIO2_CFG0	     TAC5112_REG(0, 0x0B)
#define TAC5112_REG_GPO1_CFG0	     TAC5112_REG(0, 0x0C)
#define TAC5112_REG_GPI_CFG	     TAC5112_REG(0, 0x0D)
#define TAC5112_REG_GPO_GPI_VAL	     TAC5112_REG(0, 0x0E)
#define TAC5112_REG_INTF_CFG0	     TAC5112_REG(0, 0x0F)
#define TAC5112_REG_INTF_CFG1	     TAC5112_REG(0, 0x10)
#define TAC5112_REG_INTF_CFG2	     TAC5112_REG(0, 0x11)
#define TAC5112_REG_INTF_CFG3	     TAC5112_REG(0, 0x12)
#define TAC5112_REG_INTF_CFG4	     TAC5112_REG(0, 0x13)
#define TAC5112_REG_INTF_CFG5	     TAC5112_REG(0, 0x14)
#define TAC5112_REG_INTF_CFG6	     TAC5112_REG(0, 0x15)
#define TAC5112_REG_ASI_CFG0	     TAC5112_REG(0, 0x18)
#define TAC5112_REG_ASI_CFG1	     TAC5112_REG(0, 0x19)
#define TAC5112_REG_PASI_CFG0	     TAC5112_REG(0, 0x1A)
#define TAC5112_REG_PASI_TX_CFG0     TAC5112_REG(0, 0x1B)
#define TAC5112_REG_PASI_TX_CFG1     TAC5112_REG(0, 0x1C)
#define TAC5112_REG_PASI_TX_CFG2     TAC5112_REG(0, 0x1D)
#define TAC5112_REG_PASI_TX_CH1_CFG  TAC5112_REG(0, 0x1E)
#define TAC5112_REG_PASI_TX_CH2_CFG  TAC5112_REG(0, 0x1F)
#define TAC5112_REG_PASI_TX_CH3_CFG  TAC5112_REG(0, 0x20)
#define TAC5112_REG_PASI_TX_CH4_CFG  TAC5112_REG(0, 0x21)
#define TAC5112_REG_PASI_TX_CH5_CFG  TAC5112_REG(0, 0x22)
#define TAC5112_REG_PASI_TX_CH6_CFG  TAC5112_REG(0, 0x23)
#define TAC5112_REG_PASI_TX_CH7_CFG  TAC5112_REG(0, 0x24)
#define TAC5112_REG_PASI_TX_CH8_CFG  TAC5112_REG(0, 0x25)
#define TAC5112_REG_PASI_RX_CFG0     TAC5112_REG(0, 0x26)
#define TAC5112_REG_PASI_RX_CFG1     TAC5112_REG(0, 0x27)
#define TAC5112_REG_PASI_RX_CH1_CFG  TAC5112_REG(0, 0x28)
#define TAC5112_REG_PASI_RX_CH2_CFG  TAC5112_REG(0, 0x29)
#define TAC5112_REG_PASI_RX_CH3_CFG  TAC5112_REG(0, 0x2A)
#define TAC5112_REG_PASI_RX_CH4_CFG  TAC5112_REG(0, 0x2B)
#define TAC5112_REG_PASI_RX_CH5_CFG  TAC5112_REG(0, 0x2C)
#define TAC5112_REG_PASI_RX_CH6_CFG  TAC5112_REG(0, 0x2D)
#define TAC5112_REG_PASI_RX_CH7_CFG  TAC5112_REG(0, 0x2E)
#define TAC5112_REG_PASI_RX_CH8_CFG  TAC5112_REG(0, 0x2F)
#define TAC5112_REG_CLK_CFG0	     TAC5112_REG(0, 0x32)
#define TAC5112_REG_CLK_CFG1	     TAC5112_REG(0, 0x33)
#define TAC5112_REG_CLK_CFG2	     TAC5112_REG(0, 0x34)
#define TAC5112_REG_CNT_CLK_CFG0     TAC5112_REG(0, 0x35)
#define TAC5112_REG_CNT_CLK_CFG1     TAC5112_REG(0, 0x36)
#define TAC5112_REG_CNT_CLK_CFG2     TAC5112_REG(0, 0x37)
#define TAC5112_REG_CNT_CLK_CFG3     TAC5112_REG(0, 0x38)
#define TAC5112_REG_CNT_CLK_CFG4     TAC5112_REG(0, 0x39)
#define TAC5112_REG_CNT_CLK_CFG5     TAC5112_REG(0, 0x3A)
#define TAC5112_REG_CNT_CLK_CFG6     TAC5112_REG(0, 0x3B)
#define TAC5112_REG_CLK_ERR_STS0     TAC5112_REG(0, 0x3C)
#define TAC5112_REG_CLK_ERR_STS1     TAC5112_REG(0, 0x3D)
#define TAC5112_REG_CLK_DET_STS0     TAC5112_REG(0, 0x3E)
#define TAC5112_REG_CLK_DET_STS1     TAC5112_REG(0, 0x3F)
#define TAC5112_REG_CLK_DET_STS2     TAC5112_REG(0, 0x40)
#define TAC5112_REG_CLK_DET_STS3     TAC5112_REG(0, 0x41)
#define TAC5112_REG_INT_CFG	     TAC5112_REG(0, 0x42)
#define TAC5112_REG_DAC_FLT_CFG	     TAC5112_REG(0, 0x43)
#define TAC5112_REG_ADC_DAC_MISC_CFG TAC5112_REG(0, 0x4B)
#define TAC5112_REG_IADC_CFG	     TAC5112_REG(0, 0x4C)
#define TAC5112_REG_VREF_MICBIAS_CFG TAC5112_REG(0, 0x4D)
#define TAC5112_REG_PWR_TUNE_CFG0    TAC5112_REG(0, 0x4E)
#define TAC5112_REG_PWR_TUNE_CFG1    TAC5112_REG(0, 0x4F)
#define TAC5112_REG_ADC_CH1_CFG0     TAC5112_REG(0, 0x50)
#define TAC5112_REG_IADC_CH_CFG	     TAC5112_REG(0, 0x51)
#define TAC5112_REG_ADC_CH1_CFG2     TAC5112_REG(0, 0x52)
#define TAC5112_REG_ADC_CH1_CFG3     TAC5112_REG(0, 0x53)
#define TAC5112_REG_ADC_CH1_CFG4     TAC5112_REG(0, 0x54)
#define TAC5112_REG_ADC_CH2_CFG0     TAC5112_REG(0, 0x55)
#define TAC5112_REG_ADC_CH2_CFG2     TAC5112_REG(0, 0x57)
#define TAC5112_REG_ADC_CH2_CFG3     TAC5112_REG(0, 0x58)
#define TAC5112_REG_ADC_CH2_CFG4     TAC5112_REG(0, 0x59)
#define TAC5112_REG_ADC_CH3_CFG0     TAC5112_REG(0, 0x5A)
#define TAC5112_REG_ADC_CH3_CFG2     TAC5112_REG(0, 0x5B)
#define TAC5112_REG_ADC_CH3_CFG3     TAC5112_REG(0, 0x5C)
#define TAC5112_REG_ADC_CH3_CFG4     TAC5112_REG(0, 0x5D)
#define TAC5112_REG_ADC_CH4_CFG0     TAC5112_REG(0, 0x5E)
#define TAC5112_REG_ADC_CH4_CFG2     TAC5112_REG(0, 0x5F)
#define TAC5112_REG_ADC_CH4_CFG3     TAC5112_REG(0, 0x60)
#define TAC5112_REG_ADC_CH4_CFG4     TAC5112_REG(0, 0x61)
#define TAC5112_REG_ADC_CFG1	     TAC5112_REG(0, 0x62)
#define TAC5112_REG_DAC_CH1A_CFG0    TAC5112_REG(0, 0x67)
#define TAC5112_REG_DAC_CH1A_CFG1    TAC5112_REG(0, 0x68)
#define TAC5112_REG_DAC_CH1B_CFG0    TAC5112_REG(0, 0x69)
#define TAC5112_REG_DAC_CH1B_CFG1    TAC5112_REG(0, 0x6A)
#define TAC5112_REG_DAC_CH2A_CFG0    TAC5112_REG(0, 0x6E)
#define TAC5112_REG_DAC_CH2A_CFG1    TAC5112_REG(0, 0x6F)
#define TAC5112_REG_DAC_CH2B_CFG0    TAC5112_REG(0, 0x70)
#define TAC5112_REG_DAC_CH2B_CFG1    TAC5112_REG(0, 0x71)
#define TAC5112_REG_DSP_CFG0	     TAC5112_REG(0, 0x72)
#define TAC5112_REG_DSP_CFG1	     TAC5112_REG(0, 0x73)
#define TAC5112_REG_CH_EN	     TAC5112_REG(0, 0x76)
#define TAC5112_REG_DYN_PUPD_CFG     TAC5112_REG(0, 0x77)
#define TAC5112_REG_PWR_CFG	     TAC5112_REG(0, 0x78)
#define TAC5112_REG_DEV_STS0	     TAC5112_REG(0, 0x79)
#define TAC5112_REG_DEV_STS1	     TAC5112_REG(0, 0x7A)
#define TAC5112_REG_I2C_CKSUM	     TAC5112_REG(0, 0x7E)

/* ---- Page 1: DSP, limiter/AGC, interrupts, and diagnostics registers ---- */
/* Register 0 of every page is PAGE_CFG (the page-select). */
#define TAC5112_REG_DSP_CFG0_P1		    TAC5112_REG(1, 0x03)
#define TAC5112_REG_CLK_CFG0_P1		    TAC5112_REG(1, 0x0D)
#define TAC5112_REG_CHANNEL_CFG1	    TAC5112_REG(1, 0x0E)
#define TAC5112_REG_CHANNEL_CFG2	    TAC5112_REG(1, 0x0F)
#define TAC5112_REG_SRC_CFG0		    TAC5112_REG(1, 0x17)
#define TAC5112_REG_SRC_CFG1		    TAC5112_REG(1, 0x18)
#define TAC5112_REG_JACK_DET_CFG0	    TAC5112_REG(1, 0x19)
#define TAC5112_REG_JACK_DET_CFG1	    TAC5112_REG(1, 0x1A)
#define TAC5112_REG_JACK_DET_CFG2	    TAC5112_REG(1, 0x1B)
#define TAC5112_REG_JACK_DET_CFG3	    TAC5112_REG(1, 0x1C)
#define TAC5112_REG_LPAD_CFG1		    TAC5112_REG(1, 0x1E)
#define TAC5112_REG_LPSG_CFG1		    TAC5112_REG(1, 0x1F)
#define TAC5112_REG_LPAD_LPSG_CFG1	    TAC5112_REG(1, 0x20)
#define TAC5112_REG_LIMITER_CFG		    TAC5112_REG(1, 0x23)
#define TAC5112_REG_AGC_DRC_CFG		    TAC5112_REG(1, 0x24)
#define TAC5112_REG_PLIM_CFG0		    TAC5112_REG(1, 0x2B)
#define TAC5112_REG_MIXER_CFG0		    TAC5112_REG(1, 0x2C)
#define TAC5112_REG_MISC_CFG0_P1	    TAC5112_REG(1, 0x2D)
#define TAC5112_REG_BRWNOUT		    TAC5112_REG(1, 0x2E)
#define TAC5112_REG_INT_MASK0		    TAC5112_REG(1, 0x2F)
#define TAC5112_REG_INT_MASK4		    TAC5112_REG(1, 0x32)
#define TAC5112_REG_INT_MASK5		    TAC5112_REG(1, 0x33)
#define TAC5112_REG_INT_LTCH0		    TAC5112_REG(1, 0x34)
#define TAC5112_REG_OUT_CH1_LTCH	    TAC5112_REG(1, 0x38)
#define TAC5112_REG_OUT_CH2_LTCH	    TAC5112_REG(1, 0x39)
#define TAC5112_REG_INT_LTCH1		    TAC5112_REG(1, 0x3A)
#define TAC5112_REG_INT_LTCH2		    TAC5112_REG(1, 0x3B)
#define TAC5112_REG_INT_LIVE0		    TAC5112_REG(1, 0x3C)
#define TAC5112_REG_OUT_CH1_LIVE	    TAC5112_REG(1, 0x40)
#define TAC5112_REG_OUT_CH2_LIVE	    TAC5112_REG(1, 0x41)
#define TAC5112_REG_INT_LIVE1		    TAC5112_REG(1, 0x42)
#define TAC5112_REG_INT_LIVE2		    TAC5112_REG(1, 0x43)
#define TAC5112_REG_DIAG_CFG8		    TAC5112_REG(1, 0x4E)
#define TAC5112_REG_DIAG_CFG9		    TAC5112_REG(1, 0x4F)
#define TAC5112_REG_DIAG_CFG13		    TAC5112_REG(1, 0x53)
#define TAC5112_REG_DIAG_CFG14		    TAC5112_REG(1, 0x54)
#define TAC5112_REG_DIAGDATA_CFG	    TAC5112_REG(1, 0x55)
#define TAC5112_REG_DIAG_MON_MSB_MBIAS	    TAC5112_REG(1, 0x58)
#define TAC5112_REG_DIAG_MON_LSB_MBIAS	    TAC5112_REG(1, 0x59)
#define TAC5112_REG_DIAG_MON_MSB_OUT1P	    TAC5112_REG(1, 0x62)
#define TAC5112_REG_DIAG_MON_LSB_OUT1P	    TAC5112_REG(1, 0x63)
#define TAC5112_REG_DIAG_MON_MSB_OUT1M	    TAC5112_REG(1, 0x64)
#define TAC5112_REG_DIAG_MON_LSB_OUT1M	    TAC5112_REG(1, 0x65)
#define TAC5112_REG_DIAG_MON_MSB_OUT2P	    TAC5112_REG(1, 0x66)
#define TAC5112_REG_DIAG_MON_LSB_OUT2P	    TAC5112_REG(1, 0x67)
#define TAC5112_REG_DIAG_MON_MSB_OUT2M	    TAC5112_REG(1, 0x68)
#define TAC5112_REG_DIAG_MON_LSB_OUT2M	    TAC5112_REG(1, 0x69)
#define TAC5112_REG_DIAG_MON_MSB_TEMP	    TAC5112_REG(1, 0x6A)
#define TAC5112_REG_DIAG_MON_LSB_TEMP	    TAC5112_REG(1, 0x6B)
#define TAC5112_REG_DIAG_MON_MSB_MBIAS_LOAD TAC5112_REG(1, 0x6C)
#define TAC5112_REG_DIAG_MON_LSB_MBIAS_LOAD TAC5112_REG(1, 0x6D)
#define TAC5112_REG_DIAG_MON_MSB_AVDD	    TAC5112_REG(1, 0x6E)
#define TAC5112_REG_DIAG_MON_LSB_AVDD	    TAC5112_REG(1, 0x6F)
#define TAC5112_REG_DIAG_MON_MSB_GPA	    TAC5112_REG(1, 0x70)
#define TAC5112_REG_DIAG_MON_LSB_GPA	    TAC5112_REG(1, 0x71)

/* ---- Page 3: secondary ASI and PLL / clock divider registers ---- */
/* Register 0 of every page is PAGE_CFG (the page-select). */
#define TAC5112_REG_SASI_CFG0	    TAC5112_REG(3, 0x1A)
#define TAC5112_REG_SASI_TX_CFG0    TAC5112_REG(3, 0x1B)
#define TAC5112_REG_SASI_TX_CFG1    TAC5112_REG(3, 0x1C)
#define TAC5112_REG_SASI_TX_CFG2    TAC5112_REG(3, 0x1D)
#define TAC5112_REG_SASI_TX_CH1_CFG TAC5112_REG(3, 0x1E)
#define TAC5112_REG_SASI_TX_CH2_CFG TAC5112_REG(3, 0x1F)
#define TAC5112_REG_SASI_TX_CH3_CFG TAC5112_REG(3, 0x20)
#define TAC5112_REG_SASI_TX_CH4_CFG TAC5112_REG(3, 0x21)
#define TAC5112_REG_SASI_TX_CH5_CFG TAC5112_REG(3, 0x22)
#define TAC5112_REG_SASI_TX_CH6_CFG TAC5112_REG(3, 0x23)
#define TAC5112_REG_SASI_TX_CH7_CFG TAC5112_REG(3, 0x24)
#define TAC5112_REG_SASI_TX_CH8_CFG TAC5112_REG(3, 0x25)
#define TAC5112_REG_SASI_RX_CFG0    TAC5112_REG(3, 0x26)
#define TAC5112_REG_SASI_RX_CFG1    TAC5112_REG(3, 0x27)
#define TAC5112_REG_SASI_RX_CH1_CFG TAC5112_REG(3, 0x28)
#define TAC5112_REG_SASI_RX_CH2_CFG TAC5112_REG(3, 0x29)
#define TAC5112_REG_SASI_RX_CH3_CFG TAC5112_REG(3, 0x2A)
#define TAC5112_REG_SASI_RX_CH4_CFG TAC5112_REG(3, 0x2B)
#define TAC5112_REG_SASI_RX_CH5_CFG TAC5112_REG(3, 0x2C)
#define TAC5112_REG_SASI_RX_CH6_CFG TAC5112_REG(3, 0x2D)
#define TAC5112_REG_SASI_RX_CH7_CFG TAC5112_REG(3, 0x2E)
#define TAC5112_REG_SASI_RX_CH8_CFG TAC5112_REG(3, 0x2F)
#define TAC5112_REG_CLK_CFG12	    TAC5112_REG(3, 0x32)
#define TAC5112_REG_CLK_CFG13	    TAC5112_REG(3, 0x33)
#define TAC5112_REG_CLK_CFG14	    TAC5112_REG(3, 0x34)
#define TAC5112_REG_CLK_CFG15	    TAC5112_REG(3, 0x35)
#define TAC5112_REG_CLK_CFG16	    TAC5112_REG(3, 0x36)
#define TAC5112_REG_CLK_CFG17	    TAC5112_REG(3, 0x37)
#define TAC5112_REG_CLK_CFG18	    TAC5112_REG(3, 0x38)
#define TAC5112_REG_CLK_CFG19	    TAC5112_REG(3, 0x39)
#define TAC5112_REG_CLK_CFG20	    TAC5112_REG(3, 0x3A)
#define TAC5112_REG_CLK_CFG21	    TAC5112_REG(3, 0x3B)
#define TAC5112_REG_CLK_CFG22	    TAC5112_REG(3, 0x3C)
#define TAC5112_REG_CLK_CFG23	    TAC5112_REG(3, 0x3D)
#define TAC5112_REG_CLK_CFG24	    TAC5112_REG(3, 0x3E)
#define TAC5112_REG_CLK_CFG30	    TAC5112_REG(3, 0x44)
#define TAC5112_REG_CLK_CFG31	    TAC5112_REG(3, 0x45)
#define TAC5112_REG_CLKOUT_CFG1	    TAC5112_REG(3, 0x46)
#define TAC5112_REG_CLKOUT_CFG2	    TAC5112_REG(3, 0x47)
#define TAC5112_REG_SARCLK_CFG1	    TAC5112_REG(3, 0x49)
#define TAC5112_REG_ADC_OVRLD_FLAG  TAC5112_REG(3, 0x5B)

/**
 * @brief Highest page number this driver ever accesses.
 *
 * Used by the I2C emulator (test build) to size its backing store and to
 * reject out-of-range page selects.
 */
#define TAC5112_PAGE_MAX 3U

/*
 * ---------------------------------------------------------------------
 * Bitfields
 * ---------------------------------------------------------------------
 */

/* SW_RESET (P0_R1) */
#define TAC5112_SW_RESET_BIT BIT(0)

/* DEV_MISC_CFG (P0_R2) */
#define TAC5112_SLEEP_ENZ_BIT BIT(0) /**< 1 = active mode, 0 = sleep mode. */

/* AVDD_IOVDD_STS (P0_R3) */
#define TAC5112_BRWNOUT_SHDN_STS_BIT BIT(1)

/* MISC_CFG (P0_R4) */
#define TAC5112_I2C_BRDCAST_EN_BIT BIT(1)
#define TAC5112_IGNORE_CLK_ERR_BIT BIT(6)

/* PASI_CFG0 (P0_R26) */
#define TAC5112_PASI_FORMAT_SHIFT  6
#define TAC5112_PASI_FORMAT_MASK   GENMASK(7, 6)
#define TAC5112_PASI_WLEN_SHIFT	   4
#define TAC5112_PASI_WLEN_MASK	   GENMASK(5, 4)
#define TAC5112_PASI_FSYNC_POL_BIT BIT(3)
#define TAC5112_PASI_BCLK_POL_BIT  BIT(2)

#define TAC5112_PASI_FORMAT_TDM 0U
#define TAC5112_PASI_FORMAT_I2S 1U
#define TAC5112_PASI_FORMAT_LJ	2U

#define TAC5112_PASI_WLEN_16 0U
#define TAC5112_PASI_WLEN_20 1U
#define TAC5112_PASI_WLEN_24 2U
#define TAC5112_PASI_WLEN_32 3U

/* PASI_TX_CFG0 (P0_R27) */
#define TAC5112_PASI_TX_USE_INT_FSYNC_BIT BIT(2)
#define TAC5112_PASI_TX_USE_INT_BCLK_BIT  BIT(1)

/* CLK_CFG0 (P0_R50) */
#define TAC5112_CUSTOM_CLK_CFG_BIT   BIT(0)
#define TAC5112_PASI_SAMP_RATE_SHIFT 2
#define TAC5112_PASI_SAMP_RATE_MASK  GENMASK(7, 2)
#define TAC5112_PASI_SAMP_RATE_AUTO  0U
#define TAC5112_PASI_SAMP_RATE_MAX   40U

/* CLK_CFG2 (P0_R52) */
#define TAC5112_PLL_DIS_BIT	      BIT(7)
#define TAC5112_AUTO_PLL_FR_ALLOW_BIT BIT(6)
#define TAC5112_CLK_SRC_SEL_SHIFT     1
#define TAC5112_CLK_SRC_SEL_MASK      GENMASK(3, 1)
#define TAC5112_CLK_SRC_SEL_MAX	      5U

/* VREF_MICBIAS_CFG (P0_R77) */
#define TAC5112_MICBIAS_VAL_SHIFT 2
#define TAC5112_MICBIAS_VAL_MASK  GENMASK(3, 2)
#define TAC5112_VREF_FSCALE_SHIFT 0
#define TAC5112_VREF_FSCALE_MASK  GENMASK(1, 0)

/* ADC_CHx_CFG2 / DAC_CHxy_CFG0 digital volume field: full 8-bit register */
#define TAC5112_DVOL_MUTE    0x00U
#define TAC5112_DAC_DVOL_MIN 0x01U /**< DAC digital volume minimum, -100.0 dB. */
#define TAC5112_DAC_DVOL_0DB 0xC9U /**< DAC digital volume 0.0 dB (201 decimal). */
#define TAC5112_DAC_DVOL_MAX 0xFFU /**< DAC digital volume maximum, +27.0 dB. */
#define TAC5112_ADC_DVOL_MIN 0x01U /**< ADC digital volume minimum, -80.0 dB. */
#define TAC5112_ADC_DVOL_0DB 0xA1U /**< ADC digital volume 0.0 dB (161 decimal). */
#define TAC5112_ADC_DVOL_MAX 0xFFU /**< ADC digital volume maximum, +47.0 dB. */

/*
 * AUDIO_PROPERTY_{OUTPUT,INPUT}_VOLUME are expressed by this driver in units
 * of 0.5 dB (audio_property_value_t.vol == dB * 2), matching the codec's
 * native digital-volume register step size. The following macros give the
 * inclusive valid ranges for tac5112_dvol_from_half_db().
 */
#define TAC5112_DAC_VOL_MIN_HALF_DB ((int32_t)TAC5112_DAC_DVOL_MIN - (int32_t)TAC5112_DAC_DVOL_0DB)
#define TAC5112_DAC_VOL_MAX_HALF_DB ((int32_t)TAC5112_DAC_DVOL_MAX - (int32_t)TAC5112_DAC_DVOL_0DB)
#define TAC5112_ADC_VOL_MIN_HALF_DB ((int32_t)TAC5112_ADC_DVOL_MIN - (int32_t)TAC5112_ADC_DVOL_0DB)
#define TAC5112_ADC_VOL_MAX_HALF_DB ((int32_t)TAC5112_ADC_DVOL_MAX - (int32_t)TAC5112_ADC_DVOL_0DB)

/* CH_EN (P0_R118) */
#define TAC5112_CH_EN_IN_CH1_BIT  BIT(7)
#define TAC5112_CH_EN_IN_CH2_BIT  BIT(6)
#define TAC5112_CH_EN_OUT_CH1_BIT BIT(3)
#define TAC5112_CH_EN_OUT_CH2_BIT BIT(2)

/* PWR_CFG (P0_R120) */
#define TAC5112_PWR_CFG_ADC_PDZ_BIT	BIT(7)
#define TAC5112_PWR_CFG_DAC_PDZ_BIT	BIT(6)
#define TAC5112_PWR_CFG_MICBIAS_PDZ_BIT BIT(5)

/* DEV_STS1 (P0_R122) */
#define TAC5112_DEV_STS1_PLL_STS_BIT BIT(4)

/* OUT_CHn_LTCH / OUT_CHn_LIVE (P1) fault bits, common layout */
#define TAC5112_OUT_CH_SC_OUTP_BIT    BIT(7) /**< Output short-circuit, OUTP. */
#define TAC5112_OUT_CH_SC_OUTM_BIT    BIT(6) /**< Output short-circuit, OUTM. */
#define TAC5112_OUT_CH_VG_FAULT_P_BIT BIT(5) /**< Virtual-ground fault, positive. */
#define TAC5112_OUT_CH_VG_FAULT_M_BIT BIT(4) /**< Virtual-ground fault, negative. */

/* INT_LTCH0 / INT_LIVE0 (P1) */
#define TAC5112_INT_CLK_ERROR_BIT BIT(7) /**< Clock-error interrupt. */
#define TAC5112_INT_PLL_LOCK_BIT  BIT(6) /**< PLL-lock interrupt. */

/*
 * INT_CFG (P0_R66). Reset 0x00 = active-low IRQ, assert on unmasked latched
 * events -- which is exactly the configuration this driver wants, written
 * explicitly rather than relied upon implicitly.
 */
#define TAC5112_INT_POL_ACTIVE_HIGH_BIT BIT(7) /**< 0 = active low (IRQZ). */
#define TAC5112_INT_EVENT_SHIFT		5
#define TAC5112_INT_EVENT_MASK		GENMASK(6, 5)
#define TAC5112_INT_EVENT_LATCHED	0U    /**< Assert on unmasked latched events. */
#define TAC5112_INT_CFG_FAULT_IRQ	0x00U /**< Active-low, latched-event IRQ. */

/* Interrupt mask registers (P1): bit set = masked, bit clear = enabled. */
/* INT_MASK0 (P1_R47) */
#define TAC5112_INT_MASK0_CLK_ERR_BIT  BIT(7)
#define TAC5112_INT_MASK0_PLL_LOCK_BIT BIT(6)
/* INT_MASK4 (P1_R50) */
#define TAC5112_INT_MASK4_OUT_SC_BIT   BIT(5) /**< Output short-circuit fault. */
#define TAC5112_INT_MASK4_DRVR_VG_BIT  BIT(4) /**< Driver virtual-ground fault. */

/* CLK_CFG16 (P3) */
#define TAC5112_PLL_JMUL_MSB_BIT  BIT(7)
#define TAC5112_PLL_DMUL_MSB_MASK GENMASK(5, 0)

/* CLK_CFG19 (P3) */
#define TAC5112_NDIV_SHIFT    5
#define TAC5112_NDIV_MASK     GENMASK(7, 5)
#define TAC5112_PDM_DIV_SHIFT 2
#define TAC5112_PDM_DIV_MASK  GENMASK(4, 2)

/* CLK_CFG20 (P3) */
#define TAC5112_MDIV_SHIFT	    2
#define TAC5112_MDIV_MASK	    GENMASK(7, 2)
#define TAC5112_ADC_MODCLK_DIV_MASK GENMASK(1, 0)

/* Raw field limits (used for bounds checking caller-supplied values) */
#define TAC5112_PLL_JMUL_MIN	       1U
#define TAC5112_PLL_JMUL_MAX	       511U
#define TAC5112_PLL_DMUL_MAX	       9999U
#define TAC5112_PLL_NDIV_MAX	       7U
#define TAC5112_PLL_MDIV_MAX	       63U
#define TAC5112_PLL_PDM_DIV_SEL_MAX    4U
#define TAC5112_ADC_MODCLK_DIV_SEL_MAX 2U

/* Timing constants from datasheet Section 7.4 "Power Modes" */
#define TAC5112_T_SLEEP_EXIT_MIN_MS  2U	 /**< Wait after SLEEP_ENZ=1 before further I2C. */
#define TAC5112_T_SLEEP_ENTER_MIN_MS 10U /**< Wait after entering sleep before I2C. */
#define TAC5112_T_RESET_SETTLE_MS    2U	 /**< Conservative settle time after SW_RESET. */

/* Supported audio sample rate limits, datasheet Section 7.3.2. */
#define TAC5112_FS_MIN_HZ 4000U	  /**< Minimum supported sample rate, in Hz. */
#define TAC5112_FS_MAX_HZ 768000U /**< Maximum supported sample rate, in Hz. */

/** Temporary placeholder property ID for EQ gain. */
#define AUDIO_PROPERTY_EQ_GAIN (AUDIO_PROPERTY_INPUT_MUTE + 1)

/**
 * @brief PLL reference clock source.
 *
 * Values match CLK_CFG2.CLK_SRC_SEL[2:0] (P0_R52_D[3:1]).
 */
enum tac5112_pll_clk_src {
	TAC5112_PLL_CLK_SRC_PASI_BCLK = 0, /**< Primary ASI BCLK. */
	TAC5112_PLL_CLK_SRC_CCLK_PASI_FSYNC =
		1,			   /**< CCLK synchronized with the primary ASI FSYNC. */
	TAC5112_PLL_CLK_SRC_SASI_BCLK = 2, /**< Secondary ASI BCLK. */
	TAC5112_PLL_CLK_SRC_CCLK_SASI_FSYNC =
		3,			    /**< CCLK synchronized with the secondary ASI FSYNC. */
	TAC5112_PLL_CLK_SRC_FIXED_CCLK = 4, /**< Fixed CCLK frequency (controller mode only). */
	TAC5112_PLL_CLK_SRC_INTERNAL_OSC =
		5, /**< Internal oscillator (custom clock configuration only). */
};

/**
 * @brief TAC5112 PLL configuration.
 *
 * The TAC5112 contains a smart auto-configuration block (datasheet
 * Section 7.3.2) that derives every internal clock, including the PLL
 * P/J/D and N/M dividers, purely by observing the incoming BCLK/FSYNC
 * frequency ratio. TI recommends this "auto" mode for all standard audio
 * use cases, and it is what tac5112_configure_pll() programs when
 * @ref tac5112_pll_config.auto_config is true.
 *
 * "Custom" mode (@ref tac5112_pll_config.auto_config = false) exposes the raw
 * PLL divider registers (Section 8.1.3) for applications that must use a clock
 * ratio outside of the auto-detected table (Table 7-7 / 7-8) and have derived
 * P/J/D/N/M values from TI's "Clocking Configuration of Device and Flexible
 * Clocking For TAx5x1x Family" application note. This driver does not compute
 * those values itself: it only validates that each field fits within its
 * documented register range and writes them verbatim, since the closed-form
 * PLL frequency equation is not reproduced in the register-level section of
 * the datasheet.
 */
struct tac5112_pll_config {
	/** Use the device's automatic clock/PLL configuration (recommended). */
	bool auto_config;
	/** Allow the PLL to lock in fractional (non-integer) mode when auto_config. */
	bool fractional_allowed;
	/** Reference clock fed to the PLL / PDIV. */
	enum tac5112_pll_clk_src clk_src;

	/* --- Custom-mode-only fields; ignored when auto_config is true --- */

	/** PLL_PDIV[7:0] raw register value (0=256, 1=1, 2=2, ... 255=255). */
	uint8_t pdiv;
	/** PLL_JMUL[8:0] integer multiplier, valid range [1, 511]. */
	uint16_t jmul;
	/** PLL_DMUL[13:0] fractional multiplier numerator /10000, range [0, 9999]. */
	uint16_t dmul;
	/** NDIV[2:0] raw register value (0=8, 1=1, 2=2, ... 7=7). */
	uint8_t ndiv;
	/** MDIV[5:0] raw register value (0=64, 1=1, 2=2, ... 63=63). */
	uint8_t mdiv;
	/** PDM_DIV[2:0] raw register value (0=1, 1=2, 2=4, 3=8, 4=16). */
	uint8_t pdm_div_sel;
	/** DIG_ADC_MODCLK_DIV[1:0] raw register value (0=1, 1=2, 2=4). */
	uint8_t adc_modclk_div_sel;
};

/*
 * ---------------------------------------------------------------------
 * Driver config / runtime data
 * ---------------------------------------------------------------------
 */

/** @brief Per-channel property cache so mute/unmute can restore the last volume. */
struct tac5112_chan_cache {
	int16_t vol_half_db; /**< Last requested volume, in 0.5 dB steps. */
	bool muted;	     /**< Current mute state of the channel. */
};

/** @brief TAC5112 driver runtime data. */
struct tac5112_data {
	/** Serializes multi-register sequences and the property cache. */
	struct k_mutex lock;
	/** Cached current page-select register value (host-side mirror). */
	uint8_t page_cache;
	/** Set once configure() has completed successfully. */
	bool configured;
	/** Output (DAC) property cache, indexed by 0=left, 1=right. */
	struct tac5112_chan_cache out_cache[2];
	/** Input (ADC) property cache, indexed by 0=left, 1=right. */
	struct tac5112_chan_cache in_cache[2];
	/** User-registered fault callback, or NULL. */
	audio_codec_error_callback_t error_cb;
	/** Deferred (thread-context) fault processing work item. */
	struct k_work fault_work;
	/** GPIO interrupt callback bookkeeping for the optional IRQ line. */
	struct gpio_callback fault_gpio_cb;
	/** Back-pointer so the work handler can reach the device. */
	const struct device *self;
};

/** @brief TAC5112 driver instance configuration (devicetree-derived). */
struct tac5112_config {
	struct i2c_dt_spec bus;		/**< I2C bus and address of the codec. */
	struct gpio_dt_spec reset_gpio; /**< Optional hardware reset line. */
	struct gpio_dt_spec fault_gpio; /**< Optional fault/IRQ input line. */
};

/*
 * ---------------------------------------------------------------------
 * Driver-internal helpers exposed for whitebox unit testing.
 *
 * These are not part of the public Zephyr audio codec API and may change
 * without notice; the ztest suite in tests/drivers/audio/tac5112 adds
 * drivers/audio/ to its include path (see that test's CMakeLists.txt)
 * and includes this header directly precisely to exercise them.
 *
 * None of the four functions below lock struct tac5112_data::lock
 * internally. tac5112_configure_pll() and the driver's own API callbacks
 * take the lock once for their whole multi-register sequence and then
 * call these freely (k_mutex is recursive for its owning thread, so
 * nested acquisition by the same thread never deadlocks). Application
 * code calling tac5112_reg_write()/tac5112_reg_read()/tac5112_reg_update()
 * directly and concurrently with the codec API from another thread must
 * take dev->data's lock itself.
 * ---------------------------------------------------------------------
 */

/**
 * @brief Write a single codec register.
 *
 * Selects the register's page if it differs from the cached page, then writes
 * @p val. Does not take the driver lock.
 *
 * @param[in] dev Codec device.
 * @param[in] reg Register (page and address) to write.
 * @param[in] val Value to write.
 *
 * @retval 0 On success.
 * @retval -errno Negative error code propagated from the I2C API.
 */
int tac5112_reg_write(const struct device *dev, struct tac5112_reg reg, uint8_t val);

/**
 * @brief Read a single codec register.
 *
 * Selects the register's page if it differs from the cached page, then reads
 * into @p val. Does not take the driver lock.
 *
 * @param[in]  dev Codec device.
 * @param[in]  reg Register (page and address) to read.
 * @param[out] val Location that receives the register value.
 *
 * @retval 0 On success.
 * @retval -errno Negative error code propagated from the I2C API.
 */
int tac5112_reg_read(const struct device *dev, struct tac5112_reg reg, uint8_t *val);

/**
 * @brief Read-modify-write a single codec register.
 *
 * Updates only the bits set in @p mask with the corresponding bits of
 * @p val, leaving the remaining bits unchanged. Does not take the driver
 * lock.
 *
 * @param[in] dev  Codec device.
 * @param[in] reg  Register (page and address) to update.
 * @param[in] mask Bits to modify.
 * @param[in] val  New values for the bits selected by @p mask.
 *
 * @retval 0 On success.
 * @retval -errno Negative error code propagated from the I2C API.
 */
int tac5112_reg_update(const struct device *dev, struct tac5112_reg reg, uint8_t mask, uint8_t val);

/**
 * @brief Configure the codec PLL and clock tree.
 *
 * Programs automatic clock configuration, or the raw PLL and divider
 * registers in custom mode, according to @p pll. In custom mode, each field
 * is validated against its documented register range. Takes the driver lock
 * for the whole multi-register sequence.
 *
 * @param[in] dev Codec device.
 * @param[in] pll PLL configuration to apply.
 *
 * @retval 0 On success.
 * @retval -EINVAL A custom-mode field is outside its valid range.
 * @retval -errno Negative error code propagated from the I2C API.
 */
int tac5112_configure_pll(const struct device *dev, const struct tac5112_pll_config *pll);

/**
 * @brief Convert a volume in 0.5 dB steps to a codec digital-volume value.
 *
 * @param[in]  is_output True to convert for the output (DAC) range, false for
 *                       the input (ADC) range.
 * @param[in]  half_db   Requested volume, in 0.5 dB steps.
 * @param[out] dvol_out  Location that receives the register value.
 *
 * @retval 0 On success.
 * @retval -EINVAL @p half_db is outside the selected range.
 */
int tac5112_dvol_from_half_db(bool is_output, int32_t half_db, uint8_t *dvol_out);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_TAC5112_H_ */
