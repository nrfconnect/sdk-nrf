/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup Drivers audio TAC5112
 * @{
 * @brief Public API for the Texas Instruments TAC5112 low-power stereo audio codec.
 */

#ifndef DRIVERS_AUDIO_TAC5112_H_
#define DRIVERS_AUDIO_TAC5112_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PLL reference clock source.
 *
 * Values match CLK_CFG2.CLK_SRC_SEL[2:0] (P0_R52_D[3:1]).
 */
enum tac5112_pll_clk_src {
	/**< Primary ASI BCLK. */
	TAC5112_PLL_CLK_SRC_PASI_BCLK = 0,
	/**< CCLK synchronized with the primary ASI FSYNC. */
	TAC5112_PLL_CLK_SRC_CCLK_PASI_FSYNC = 1,
	/**< Secondary ASI BCLK. */
	TAC5112_PLL_CLK_SRC_SASI_BCLK = 2,
	/**< CCLK synchronized with the secondary ASI FSYNC. */
	TAC5112_PLL_CLK_SRC_CCLK_SASI_FSYNC = 3,
	/**< Fixed CCLK frequency (controller mode only). */
	TAC5112_PLL_CLK_SRC_FIXED_CCLK = 4,
	/**< Internal oscillator (custom clock configuration only). */
	TAC5112_PLL_CLK_SRC_INTERNAL_OSC = 5,
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

/**
 * @brief Configure the codec PLL and clock tree.
 *
 * Programs automatic clock configuration, or the raw PLL and divider
 * registers in custom mode, according to @p pll. In custom mode, each field
 * is validated against its documented register range. Takes the driver lock
 * for the whole multi-register sequence.
 *
 * A successful custom-mode call records @p pll's JMUL/DMUL as the baseline for
 * tac5112_trim_pll() and enables runtime trimming; an auto-mode call clears
 * that baseline and disables trimming, because the device recomputes the
 * fractional multiplier itself in auto mode. audio_codec_configure() always
 * reprograms auto mode, so it resets any prior custom PLL and disables trim.
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
 * @brief Fine-trim the PLL sample rate by a fractional offset (clock recovery).
 *
 * Retunes the codec's fractional PLL multiplier by @p ppb parts-per-billion
 * relative to the JMUL/DMUL baseline last programmed by a custom-mode
 * tac5112_configure_pll(), letting an application steer the codec's audio clock
 * to track an external timeline (for example, an adaptive audio-clock-recovery
 * loop nulling a render-timestamp error when the codec is the ASI clock
 * controller). Only the DMUL fractional numerator is rewritten
 * (CLK_CFG16[5:0] + CLK_CFG17), without disabling the PLL, so the update
 * retunes the running clock rather than forcing a relock.
 *
 * The correction is applied against the baseline every call (it is absolute,
 * not cumulative), so a control loop passes its total offset from nominal each
 * period. The reachable range is bounded by the DMUL register span around the
 * baseline; a baseline DMUL near mid-range leaves headroom to trim in both
 * directions, and corrections beyond that range saturate at the DMUL limits.
 *
 * Requires an active custom-mode PLL: auto mode (including after
 * audio_codec_configure()) recomputes DMUL internally, so trimming is rejected
 * with -ENOTSUP until the next custom tac5112_configure_pll(). Takes the driver
 * lock.
 *
 * @param[in] dev Codec device.
 * @param[in] ppb Fractional sample-rate offset, in parts-per-billion (signed).
 *
 * @retval 0 On success.
 * @retval -EINVAL @p dev is invalid.
 * @retval -ENOTSUP No custom-mode PLL is active.
 * @retval -errno Negative error code propagated from the I2C API.
 */
int tac5112_trim_pll(const struct device *dev, int32_t ppb);

/**
 * @brief Compute a trimmed PLL DMUL numerator from a baseline and a ppb offset.
 *
 * The sample rate is proportional to the PLL multiplier M = jmul + dmul/10000,
 * so a fractional offset of @p ppb parts-per-billion changes the DMUL numerator
 * (M scaled by 10000) by (jmul*10000 + dmul) * ppb / 1e9, rounded to nearest.
 * The result is clamped to [0, TAC5112_PLL_DMUL_MAX]; a correction larger than
 * that window saturates and needs a new JMUL via tac5112_configure_pll().
 *
 * @param[in] jmul PLL integer multiplier baseline.
 * @param[in] dmul PLL fractional multiplier baseline.
 * @param[in] ppb  Fractional offset in parts-per-billion (signed).
 *
 * @return The trimmed, clamped DMUL numerator.
 */
uint16_t tac5112_dmul_trim(uint16_t jmul, uint16_t dmul, int32_t ppb);

/**
 * @brief Select which record and playback channels are enabled at runtime.
 *
 * Rewrites the CH_EN register from the given bitmasks (bit 0 = channel 1 up to
 * bit 3 = channel 4) and powers the ADC and MICBIAS up or down according to
 * whether any input channel is enabled. The DAC power state (controlled by
 * audio_codec_start_output()/start()) is left unchanged. Takes the driver lock
 * for the register sequence, so it is safe to call after audio_codec_configure()
 * to re-select channels, for example when switching to a TDM configuration.
 *
 * @note Due to the use of I2S channels 1 and 2 (0x3) are currently supported.
 *
 * @param[in] dev             Codec device.
 * @param[in] input_channels  ADC input channel bitmask (0x0 to 0x3).
 * @param[in] output_channels DAC output channel bitmask (0x0 to 0x3).
 *
 * @retval 0 On success.
 * @retval -EINVAL @p dev is invalid or a mask has bits outside 0x0 to 0x3.
 * @retval -errno Negative error code propagated from the I2C API.
 */
int tac5112_set_channels(const struct device *dev, uint8_t input_channels, uint8_t output_channels);

/**
 * @brief Build a CH_EN register value from input/output channel bitmasks.
 *
 * Maps channel-mask bit i (channel i+1) to its CH_EN register bit: input
 * channels occupy the upper nibble (CH1=BIT(7) ... CH4=BIT(4)), output channels
 * the lower nibble (CH1=BIT(3) ... CH4=BIT(0)). Bits above bit 3 in either mask
 * are ignored.
 *
 * @param[in] input_channels  ADC input channel bitmask.
 * @param[in] output_channels DAC output channel bitmask.
 *
 * @return The corresponding CH_EN register value.
 */
uint8_t tac5112_ch_en_from_masks(uint8_t input_channels, uint8_t output_channels);

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

#endif /* DRIVERS_AUDIO_TAC5112_H_ */
