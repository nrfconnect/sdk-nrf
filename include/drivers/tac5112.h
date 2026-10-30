/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 *
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
 * @defgroup tac5112_driver Drivers audio TAC5112
 * @{
 * @brief Public TI TAC5112 API
 */

/**
 * @brief PLL reference clock source.
 *
 * Values match CLK_CFG2.CLK_SRC_SEL[2:0] (P0_R52_D[3:1]).
 */
enum tac5112_pll_clk_src {
	/** Primary ASI BCLK. */
	TAC5112_PLL_CLK_SRC_PASI_BCLK = 0,
	/** CCLK synchronized with the primary ASI FSYNC. */
	TAC5112_PLL_CLK_SRC_CCLK_PASI_FSYNC = 1,
	/** Secondary ASI BCLK. */
	TAC5112_PLL_CLK_SRC_SASI_BCLK = 2,
	/** CCLK synchronized with the secondary ASI FSYNC. */
	TAC5112_PLL_CLK_SRC_CCLK_SASI_FSYNC = 3,
	/** Fixed CCLK frequency (controller mode only). */
	TAC5112_PLL_CLK_SRC_FIXED_CCLK = 4,
	/** Internal oscillator (custom clock configuration only). */
	TAC5112_PLL_CLK_SRC_INTERNAL_OSC = 5,
};

/**
 * @brief TAC5112 PLL configuration.
 *
 * The TAC5112 contains a smart auto-configuration block (datasheet
 * Section 7.3.2) that derives every internal clock, including the PLL
 * P/J/D and N/M dividers, purely by observing the incoming BCLK/FSYNC
 * frequency ratio. TI recommends this "auto" mode for all standard audio
 * use cases, and it is what tac5112_pll_configure() programs when
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
 * tac5112_pll_trim() and enables runtime trimming; an auto-mode call clears
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
int tac5112_pll_configure(const struct device *dev, const struct tac5112_pll_config *pll);

/**
 * @brief Fine-trim the PLL sample rate by a fractional offset (clock recovery).
 *
 * Retunes the codec's fractional PLL multiplier by @p ppb parts-per-billion
 * relative to the JMUL/DMUL baseline last programmed by a custom-mode
 * tac5112_pll_configure(), letting an application steer the codec's audio clock
 * to track an external timeline (for example, an adaptive audio-clock-recovery
 * loop nulling a render-timestamp error when the codec is the ASI clock
 * controller). Only the DMUL fractional numerator is rewritten
 * (CLK_CFG16[5:0] + CLK_CFG17) and the PLL is not disabled, so the update is
 * applied to the running clock. The datasheet does not describe how the PLL
 * behaves when its multiplier changes while running (whether it stays locked or
 * relocks, or how long it takes), so verify the effect on the audio clock on
 * the target hardware.
 *
 * Operating range caveat: the PLL accepts an input of 10 MHz to 20 MHz when
 * DMUL is non-zero (fractional mode) but 256 kHz to 20 MHz when DMUL is zero
 * (integer mode), and its output (CLK_SYS) must stay within 67 MHz to 116 MHz
 * (TI SLAAEP0, Table 3-2). The driver does not know the PLL input frequency
 * and cannot check this. A trim that changes DMUL from or to zero switches the
 * PLL between integer and fractional mode, so with a baseline DMUL of zero and
 * a PLL input (after PDIV) below 10 MHz, any non-zero trim would put the PLL
 * outside its fractional-mode input range and it may lose lock. Choose the
 * clock source and PDIV so the PLL input is 10 MHz to 20 MHz, keep the baseline
 * DMUL non-zero and near mid-range, and keep JMUL.DMUL such that CLK_SYS stays
 * in range across the whole trim span.
 *
 * The correction is applied against the baseline every call (it is absolute,
 * not cumulative), so a control loop passes its total offset from nominal each
 * period. The reachable range is bounded by the DMUL register span around the
 * baseline; a baseline DMUL near mid-range leaves headroom to trim in both
 * directions. A correction beyond that range is limited to the DMUL limits (0
 * or 9999); the limited value is applied and the call returns -ERANGE so a
 * control loop can tell it has run out of range and needs a new baseline
 * (tac5112_pll_configure() with a different JMUL/DMUL).
 *
 * Requires an active custom-mode PLL: auto mode (including after
 * audio_codec_configure()) recomputes DMUL internally, so trimming is rejected
 * with -ENOTSUP until the next custom tac5112_pll_configure(). Takes the driver
 * lock.
 *
 * @param[in] dev Codec device.
 * @param[in] ppb Fractional sample-rate offset, in parts-per-billion (signed).
 *
 * @retval 0 On success.
 * @retval -EINVAL @p dev is invalid.
 * @retval -ERANGE The requested offset is outside the DMUL range around the baseline.
 *         The nearest limit (0 or 9999) was applied, so the clock moved as far as
 *         it can but not as far as requested.
 * @retval -ENOTSUP No custom-mode PLL is active.
 * @retval -errno Negative error code propagated from the I2C API. If only part of the
 *         update was written the driver restores the previous multiplier; if that
 *         also fails, trimming is disabled (-ENOTSUP) until the next custom
 *         tac5112_pll_configure().
 */
int tac5112_pll_trim(const struct device *dev, int32_t ppb);

/**
 * @brief Compute a trimmed PLL DMUL numerator from a baseline and a ppb offset.
 *
 * The sample rate is proportional to the PLL multiplier M = jmul + dmul/10000,
 * so a fractional offset of @p ppb parts-per-billion changes the DMUL numerator
 * (M scaled by 10000) by (jmul*10000 + dmul) * ppb / 1e9, rounded to nearest.
 * The result is clamped to [0, TAC5112_PLL_DMUL_MAX]; a correction larger than
 * that window saturates and needs a new JMUL via tac5112_pll_configure().
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
 * Rewrites the CH_EN register from the given bitmasks (bit 0 = channel 1,
 * bit 1 = channel 2) and powers the ADC and MICBIAS up or down according to
 * whether any input channel is enabled. The DAC power state (controlled by
 * audio_codec_start_output()/start()) is left unchanged. Takes the driver lock
 * for the register sequence, so it is safe to call after audio_codec_configure()
 * to re-select channels.
 *
 * @param[in] dev             Codec device.
 * @param[in] input_channels  ADC input channel bitmask (0x0 to 0x3).
 * @param[in] output_channels DAC output channel bitmask (0x0 to 0x3).
 *
 * @retval 0 On success.
 * @retval -EINVAL @p dev is invalid or a mask has bits outside 0x0 to 0x3.
 * @retval -errno Negative error code propagated from the I2C API.
 */
int tac5112_channels_set(const struct device *dev, uint8_t input_channels, uint8_t output_channels);

/**
 * @brief Build a CH_EN register value from input/output channel bitmasks.
 *
 * Maps channel-mask bit i (channel i+1) to its CH_EN register bit: input
 * channels occupy the upper nibble (CH1=BIT(7), CH2=BIT(6)), output channels
 * the lower nibble (CH1=BIT(3), CH2=BIT(2)). Bits above bit 1 in either mask
 * are ignored.
 *
 * @param[in] input_channels  ADC input channel bitmask.
 * @param[in] output_channels DAC output channel bitmask.
 *
 * @return The corresponding CH_EN register value.
 */
uint8_t tac5112_ch_en_from_masks(uint8_t input_channels, uint8_t output_channels);

/** @brief Signal path a volume or mute applies to. */
enum tac5112_path {
	/** Output (DAC) path. */
	TAC5112_PATH_OUTPUT,
	/** Input (ADC) path. */
	TAC5112_PATH_INPUT,
};

/**
 * @brief Convert a volume in 0.5 dB steps to a codec digital-volume value.
 *
 * @param[in]  path      Signal path the volume applies to; selects the output
 *                       (DAC) or input (ADC) range.
 * @param[in]  half_db   Requested volume, in 0.5 dB steps.
 * @param[out] dvol_out  Location that receives the register value.
 *
 * @retval 0 On success.
 * @retval -EINVAL @p path is not valid, @p dvol_out is NULL, or @p half_db is
 *         outside the range of the selected path.
 */
int tac5112_dvol_from_half_db(enum tac5112_path path, int32_t half_db, uint8_t *dvol_out);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* DRIVERS_AUDIO_TAC5112_H_ */
