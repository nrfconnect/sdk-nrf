/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Zephyr audio codec driver for the Texas Instruments TAC5112 low-power
 * stereo audio codec, targeting an nRF5-series host communicating over
 * I2C for control and I2S for audio data (the codec operating as the
 * I2S bus target/peripheral).
 *
 * Conformance notes:
 *  - Implements the mandatory members of `struct audio_codec_driver_api`
 *    (configure, start_output, stop_output, set_property,
 *    apply_properties) plus the optional clear_errors,
 *    register_error_callback, start and stop members.
 *  - Every function reachable from the public API validates its `dev`
 *    (and any other pointer arguments) before dereferencing it.
 *  - Every array/cache index derived from an `audio_channel_t` is
 *    produced by tac5112_channel_mask() and range-checked again before
 *    use; no external input is ever used directly as an array index.
 *  - No unbounded/unsafe C string or memory functions (strcpy, sprintf,
 *    etc.) are used; there are no variable-length string operations in
 *    this driver.
 */

#define DT_DRV_COMPAT ti_tac5112

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/audio/codec.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "tac5112.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tac5112);

/* Number of physical stereo channels (left, right). */
#define TAC5112_NUM_CHAN 2U

/**
 * @brief Validate that @p dev is a usable device handle for this driver.
 *
 * Called at the top of every function reachable from the public API
 * before `dev->data` or `dev->config` is dereferenced.
 */
static inline bool tac5112_dev_valid(const struct device *dev)
{
	return (dev != NULL) && (dev->data != NULL) && (dev->config != NULL);
}

/**
 * @brief Map a Zephyr audio_channel_t to the TAC5112's two physical
 * stereo channels.
 *
 * This is the only place that decides whether the left (index 0) and/or
 * right (index 1) physical channel is addressed; every other function in
 * this file that indexes a per-channel cache array receives its index
 * exclusively through this function's output (or a value already bounds
 * checked the same way), so an out-of-range index can never reach an
 * array access.
 */
static void tac5112_channel_mask(audio_channel_t channel, bool *do_left, bool *do_right)
{
	*do_left = false;
	*do_right = false;

	switch (channel) {
	case AUDIO_CHANNEL_ALL:
		*do_left = true;
		*do_right = true;
		break;
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		*do_left = true;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		*do_right = true;
		break;
	default:
		/* LFE / CENTER / REAR* / SIDE* and any codec-private channel
		 * IDs are left as (false, false): TAC5112 is a 2-channel
		 * device and has no physical output/input for them.
		 */
		break;
	}
}

int tac5112_dvol_from_half_db(bool is_output, int32_t half_db, uint8_t *dvol_out)
{
	int32_t min_half_db;
	int32_t max_half_db;
	int32_t zero_db_reg;
	int32_t reg;

	if (dvol_out == NULL) {
		return -EINVAL;
	}

	if (is_output) {
		zero_db_reg = (int32_t)TAC5112_DAC_DVOL_0DB;
		min_half_db = TAC5112_DAC_VOL_MIN_HALF_DB;
		max_half_db = TAC5112_DAC_VOL_MAX_HALF_DB;
	} else {
		zero_db_reg = (int32_t)TAC5112_ADC_DVOL_0DB;
		min_half_db = TAC5112_ADC_VOL_MIN_HALF_DB;
		max_half_db = TAC5112_ADC_VOL_MAX_HALF_DB;
	}

	/* Range-check on the full 32-bit width *before* any narrowing, so a
	 * huge value can never be truncated into an in-range register code.
	 */
	if ((half_db < min_half_db) || (half_db > max_half_db)) {
		return -EINVAL;
	}

	reg = zero_db_reg + half_db;

	/* Defensive clamp: the range check above already guarantees
	 * 1 <= reg <= 255, but never trust arithmetic alone to keep a value
	 * inside the bounds of a uint8_t register field.
	 */
	if (reg < (int32_t)TAC5112_DAC_DVOL_MIN) {
		reg = (int32_t)TAC5112_DAC_DVOL_MIN;
	} else if (reg > 0xFF) {
		reg = 0xFF;
	}

	*dvol_out = (uint8_t)reg;
	return 0;
}

int tac5112_reg_write(const struct device *dev, struct tac5112_reg reg, uint8_t val)
{
	struct tac5112_data *data;
	const struct tac5112_config *cfg;
	int ret;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	data = dev->data;
	cfg = dev->config;

	if (data->page_cache != reg.page) {
		ret = i2c_reg_write_byte_dt(&cfg->bus, TAC5112_REG_PAGE_CFG.addr, reg.page);
		if (ret < 0) {
			LOG_ERR("Page select %u failed (%d)", reg.page, ret);
			return ret;
		}
		data->page_cache = reg.page;
	}

	ret = i2c_reg_write_byte_dt(&cfg->bus, reg.addr, val);
	if (ret < 0) {
		LOG_ERR("Write pg:%u reg:0x%02x val:0x%02x failed (%d)", reg.page, reg.addr, val,
			ret);
		/* The bus transaction failed; do not trust that the device's
		 * page pointer is still where we think it is.
		 */
		data->page_cache = TAC5112_PAGE_CACHE_INVALID;
		return ret;
	}

	LOG_DBG("WR pg:%u reg:0x%02x val:0x%02x", reg.page, reg.addr, val);
	return 0;
}

int tac5112_reg_read(const struct device *dev, struct tac5112_reg reg, uint8_t *val)
{
	struct tac5112_data *data;
	const struct tac5112_config *cfg;
	int ret;

	if (!tac5112_dev_valid(dev) || (val == NULL)) {
		return -EINVAL;
	}

	data = dev->data;
	cfg = dev->config;

	if (data->page_cache != reg.page) {
		ret = i2c_reg_write_byte_dt(&cfg->bus, TAC5112_REG_PAGE_CFG.addr, reg.page);
		if (ret < 0) {
			LOG_ERR("Page select %u failed (%d)", reg.page, ret);
			return ret;
		}
		data->page_cache = reg.page;
	}

	ret = i2c_reg_read_byte_dt(&cfg->bus, reg.addr, val);
	if (ret < 0) {
		LOG_ERR("Read pg:%u reg:0x%02x failed (%d)", reg.page, reg.addr, ret);
		data->page_cache = TAC5112_PAGE_CACHE_INVALID;
		return ret;
	}

	LOG_DBG("RD pg:%u reg:0x%02x val:0x%02x", reg.page, reg.addr, *val);
	return 0;
}

int tac5112_reg_update(const struct device *dev, struct tac5112_reg reg, uint8_t mask, uint8_t val)
{
	uint8_t cur;
	int ret;

	ret = tac5112_reg_read(dev, reg, &cur);
	if (ret < 0) {
		return ret;
	}

	cur = (uint8_t)((cur & (uint8_t)~mask) | (val & mask));

	return tac5112_reg_write(dev, reg, cur);
}

int tac5112_configure_pll(const struct device *dev, const struct tac5112_pll_config *pll)
{
	struct tac5112_data *data;
	uint8_t clk_cfg2_val;
	int ret;

	if (!tac5112_dev_valid(dev) || (pll == NULL)) {
		return -EINVAL;
	}

	if ((unsigned int)pll->clk_src > TAC5112_CLK_SRC_SEL_MAX) {
		LOG_ERR("Invalid PLL clock source %u", (unsigned int)pll->clk_src);
		return -EINVAL;
	}

	if (!pll->auto_config) {
		if ((pll->jmul < TAC5112_PLL_JMUL_MIN) || (pll->jmul > TAC5112_PLL_JMUL_MAX)) {
			LOG_ERR("PLL JMUL %u out of range [%u, %u]", pll->jmul,
				TAC5112_PLL_JMUL_MIN, TAC5112_PLL_JMUL_MAX);
			return -EINVAL;
		}
		if (pll->dmul > TAC5112_PLL_DMUL_MAX) {
			LOG_ERR("PLL DMUL %u exceeds max %u", pll->dmul, TAC5112_PLL_DMUL_MAX);
			return -EINVAL;
		}
		if (pll->ndiv > TAC5112_PLL_NDIV_MAX) {
			LOG_ERR("PLL NDIV %u exceeds max %u", pll->ndiv, TAC5112_PLL_NDIV_MAX);
			return -EINVAL;
		}
		if (pll->mdiv > TAC5112_PLL_MDIV_MAX) {
			LOG_ERR("PLL MDIV %u exceeds max %u", pll->mdiv, TAC5112_PLL_MDIV_MAX);
			return -EINVAL;
		}
		if (pll->pdm_div_sel > TAC5112_PLL_PDM_DIV_SEL_MAX) {
			LOG_ERR("PLL PDM_DIV_SEL %u exceeds max %u", pll->pdm_div_sel,
				TAC5112_PLL_PDM_DIV_SEL_MAX);
			return -EINVAL;
		}
		if (pll->adc_modclk_div_sel > TAC5112_ADC_MODCLK_DIV_SEL_MAX) {
			LOG_ERR("ADC_MODCLK_DIV_SEL %u exceeds max %u", pll->adc_modclk_div_sel,
				TAC5112_ADC_MODCLK_DIV_SEL_MAX);
			return -EINVAL;
		}
	}

	data = dev->data;

	/* k_mutex is recursive for its owning thread, so it is safe for this
	 * function to be called both directly by an application and from
	 * codec_configure() (which already holds the lock).
	 */
	k_mutex_lock(&data->lock, K_FOREVER);

	clk_cfg2_val = (uint8_t)(((uint8_t)pll->clk_src << TAC5112_CLK_SRC_SEL_SHIFT) |
				 (pll->fractional_allowed ? TAC5112_AUTO_PLL_FR_ALLOW_BIT : 0U));

	ret = tac5112_reg_update(dev, TAC5112_REG_CLK_CFG2,
				 (uint8_t)(TAC5112_PLL_DIS_BIT | TAC5112_AUTO_PLL_FR_ALLOW_BIT |
					   TAC5112_CLK_SRC_SEL_MASK),
				 clk_cfg2_val);
	if (ret < 0) {
		goto unlock;
	}

	ret = tac5112_reg_update(dev, TAC5112_REG_CLK_CFG0, TAC5112_CUSTOM_CLK_CFG_BIT,
				 pll->auto_config ? 0U : TAC5112_CUSTOM_CLK_CFG_BIT);
	if (ret < 0) {
		goto unlock;
	}

	if (pll->auto_config) {
		LOG_DBG("PLL: auto clock configuration, clk_src=%u", (unsigned int)pll->clk_src);
		ret = 0;
		goto unlock;
	}

	ret = tac5112_reg_write(dev, TAC5112_REG_CLK_CFG15, pll->pdiv);
	if (ret < 0) {
		goto unlock;
	}

	{
		uint8_t jmul_msb = (uint8_t)((pll->jmul >> 8) & 0x1U);
		uint8_t jmul_lsb = (uint8_t)(pll->jmul & 0xFFU);
		uint8_t dmul_msb = (uint8_t)((pll->dmul >> 8) & TAC5112_PLL_DMUL_MSB_MASK);
		uint8_t dmul_lsb = (uint8_t)(pll->dmul & 0xFFU);
		uint8_t clk_cfg16 =
			(uint8_t)((jmul_msb != 0U ? TAC5112_PLL_JMUL_MSB_BIT : 0U) | dmul_msb);

		ret = tac5112_reg_write(dev, TAC5112_REG_CLK_CFG16, clk_cfg16);
		if (ret < 0) {
			goto unlock;
		}
		ret = tac5112_reg_write(dev, TAC5112_REG_CLK_CFG17, dmul_lsb);
		if (ret < 0) {
			goto unlock;
		}
		ret = tac5112_reg_write(dev, TAC5112_REG_CLK_CFG18, jmul_lsb);
		if (ret < 0) {
			goto unlock;
		}
	}

	ret = tac5112_reg_write(dev, TAC5112_REG_CLK_CFG19,
				(uint8_t)((pll->ndiv << TAC5112_NDIV_SHIFT) |
					  (pll->pdm_div_sel << TAC5112_PDM_DIV_SHIFT)));
	if (ret < 0) {
		goto unlock;
	}

	ret = tac5112_reg_write(
		dev, TAC5112_REG_CLK_CFG20,
		(uint8_t)((pll->mdiv << TAC5112_MDIV_SHIFT) | pll->adc_modclk_div_sel));

	LOG_DBG("PLL: custom cfg pdiv=%u jmul=%u dmul=%u ndiv=%u mdiv=%u", pll->pdiv, pll->jmul,
		pll->dmul, pll->ndiv, pll->mdiv);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Configure the primary audio serial interface (format, word
 * length) from a Zephyr audio_codec_cfg/i2s_config pair.
 *
 * Only I2S bus *target* (peripheral) operation is supported: the host
 * (nRF5) must generate BCLK and FSYNC. Requesting controller mode for
 * either clock is rejected with -ENOTSUP rather than silently
 * misconfigured, since correctly generating those clocks internally
 * requires additional CNT_CLK_CFG programming not exercised by this
 * driver.
 */
static int tac5112_configure_dai(const struct device *dev, const struct audio_codec_cfg *cfg)
{
	const struct i2s_config *i2s = &cfg->dai_cfg.i2s;
	uint8_t format;
	uint8_t wlen;
	uint8_t val;

	if ((i2s->frame_clk_freq < TAC5112_FS_MIN_HZ) ||
	    (i2s->frame_clk_freq > TAC5112_FS_MAX_HZ)) {
		LOG_ERR("Unsupported sample rate %u Hz (TAC5112 supports 4 kHz-768 kHz)",
			i2s->frame_clk_freq);
		return -EINVAL;
	}

	if (i2s->channels != TAC5112_NUM_CHAN) {
		LOG_ERR("TAC5112 is a 2-channel (stereo) codec, got channels=%u", i2s->channels);
		return -EINVAL;
	}

	/* I2S_OPT_xxx_CLK_TARGET is a set bit (BIT(1)/BIT(2)); the paired
	 * "_CONTROLLER" macros are defined as the *absence* of that bit
	 * (value 0), so they must never be tested with bitwise AND.
	 */
	if (((i2s->options & I2S_OPT_BIT_CLK_TARGET) == 0U) ||
	    ((i2s->options & I2S_OPT_FRAME_CLK_TARGET) == 0U)) {
		LOG_ERR("TAC5112 driver only supports ASI target mode; request "
			"I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET");
		return -ENOTSUP;
	}

	switch (cfg->dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		format = TAC5112_PASI_FORMAT_I2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		format = TAC5112_PASI_FORMAT_LJ;
		break;
	case AUDIO_DAI_TYPE_PCMA:
	case AUDIO_DAI_TYPE_PCMB:
	case AUDIO_DAI_TYPE_PCM:
		format = TAC5112_PASI_FORMAT_TDM;
		break;
	default:
		LOG_ERR("Unsupported dai_type %d", (int)cfg->dai_type);
		return -ENOTSUP;
	}

	switch (i2s->word_size) {
	case AUDIO_PCM_WIDTH_16_BITS:
		wlen = TAC5112_PASI_WLEN_16;
		break;
	case AUDIO_PCM_WIDTH_20_BITS:
		wlen = TAC5112_PASI_WLEN_20;
		break;
	case AUDIO_PCM_WIDTH_24_BITS:
		wlen = TAC5112_PASI_WLEN_24;
		break;
	case AUDIO_PCM_WIDTH_32_BITS:
		wlen = TAC5112_PASI_WLEN_32;
		break;
	default:
		LOG_ERR("Unsupported word size %u", i2s->word_size);
		return -EINVAL;
	}

	val = (uint8_t)((format << TAC5112_PASI_FORMAT_SHIFT) | (wlen << TAC5112_PASI_WLEN_SHIFT));

	return tac5112_reg_update(dev, TAC5112_REG_PASI_CFG0,
				  (uint8_t)(TAC5112_PASI_FORMAT_MASK | TAC5112_PASI_WLEN_MASK),
				  val);
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct tac5112_data *data;
	struct tac5112_pll_config pll = {
		.auto_config = true,
		.fractional_allowed = true,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
	};
	int ret;

	if (!tac5112_dev_valid(dev) || (cfg == NULL)) {
		return -EINVAL;
	}

	if (cfg->dai_type == AUDIO_DAI_TYPE_INVALID) {
		LOG_ERR("Invalid dai_type");
		return -EINVAL;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	data->page_cache = TAC5112_PAGE_CACHE_INVALID;

	ret = tac5112_reg_write(dev, TAC5112_REG_SW_RESET, TAC5112_SW_RESET_BIT);
	if (ret < 0) {
		goto unlock;
	}
	k_msleep(TAC5112_T_RESET_SETTLE_MS);
	/* SW_RESET also resets the device's page pointer to 0. */
	data->page_cache = 0U;

	/* Exit sleep mode (Section 7.4.2, "Active Mode"). */
	ret = tac5112_reg_update(dev, TAC5112_REG_DEV_MISC_CFG, TAC5112_SLEEP_ENZ_BIT,
				 TAC5112_SLEEP_ENZ_BIT);
	if (ret < 0) {
		goto unlock;
	}
	k_msleep(TAC5112_T_SLEEP_EXIT_MIN_MS);

	ret = tac5112_configure_dai(dev, cfg);
	if (ret < 0) {
		goto unlock;
	}

	/* tac5112_configure_pll() takes data->lock itself; k_mutex is
	 * recursive for its owning thread, so calling it while already
	 * holding the lock here just re-enters it rather than deadlocking,
	 * and keeps the whole configure() sequence atomic with respect to
	 * other threads.
	 */
	ret = tac5112_configure_pll(dev, &pll);
	if (ret < 0) {
		goto unlock;
	}

	/* Enable both stereo record and playback channels; PWR_CFG below
	 * gates the analog blocks. audio_codec_start_output()/stop_output()
	 * (and the optional start()/stop()) toggle PWR_CFG without touching
	 * CH_EN again.
	 */
	ret = tac5112_reg_write(dev, TAC5112_REG_CH_EN,
				(uint8_t)(TAC5112_CH_EN_IN_CH1_BIT | TAC5112_CH_EN_IN_CH2_BIT |
					  TAC5112_CH_EN_OUT_CH1_BIT | TAC5112_CH_EN_OUT_CH2_BIT));
	if (ret < 0) {
		goto unlock;
	}

	/* Bring up the record path + MICBIAS immediately; the DAC (playback)
	 * path is powered on lazily by start_output()/start() so that a
	 * freshly configured device does not drive its outputs until asked.
	 */
	ret = tac5112_reg_write(
		dev, TAC5112_REG_PWR_CFG,
		(uint8_t)(TAC5112_PWR_CFG_ADC_PDZ_BIT | TAC5112_PWR_CFG_MICBIAS_PDZ_BIT));
	if (ret < 0) {
		goto unlock;
	}

	/*
	 * Fault interrupt reporting on the codec IRQ pin (GPIO1, which resets
	 * to a chip-interrupt output). The IRQ line is optionally routed to
	 * the host via the fault-gpios devicetree property; see
	 * codec_register_error_callback() and tac5112_fault_isr().
	 *
	 * INT_CFG is set active-low / assert-on-unmasked-latched-event
	 * (matches reset, written explicitly for intent), and the output
	 * short-circuit and virtual-ground fault interrupts (INT_MASK4) are
	 * unmasked (bit clear = enabled). The clock-error and PLL-lock
	 * interrupts (INT_MASK0) are intentionally left masked for now.
	 */
	ret = tac5112_reg_write(dev, TAC5112_REG_INT_CFG, TAC5112_INT_CFG_FAULT_IRQ);
	if (ret < 0) {
		goto unlock;
	}
	ret = tac5112_reg_update(
		dev, TAC5112_REG_INT_MASK4,
		(uint8_t)(TAC5112_INT_MASK4_OUT_SC_BIT | TAC5112_INT_MASK4_DRVR_VG_BIT), 0U);
	if (ret < 0) {
		goto unlock;
	}

	data->configured = true;
	ret = 0;

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	struct tac5112_data *data;
	uint8_t mask = 0;
	uint8_t val = 0;
	int ret;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	if ((dir == 0U) || ((dir & (uint8_t)~AUDIO_DAI_DIR_TXRX) != 0U)) {
		LOG_ERR("Invalid direction bitmap 0x%02x", dir);
		return -EINVAL;
	}

	if (dir & AUDIO_DAI_DIR_TX) {
		mask |= TAC5112_PWR_CFG_DAC_PDZ_BIT;
		val |= TAC5112_PWR_CFG_DAC_PDZ_BIT;
	}
	if (dir & AUDIO_DAI_DIR_RX) {
		mask |= TAC5112_PWR_CFG_ADC_PDZ_BIT;
		val |= TAC5112_PWR_CFG_ADC_PDZ_BIT;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = tac5112_reg_update(dev, TAC5112_REG_PWR_CFG, mask, val);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	struct tac5112_data *data;
	uint8_t mask = 0;
	int ret;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	if ((dir == 0U) || ((dir & (uint8_t)~AUDIO_DAI_DIR_TXRX) != 0U)) {
		LOG_ERR("Invalid direction bitmap 0x%02x", dir);
		return -EINVAL;
	}

	if (dir & AUDIO_DAI_DIR_TX) {
		mask |= TAC5112_PWR_CFG_DAC_PDZ_BIT;
	}
	if (dir & AUDIO_DAI_DIR_RX) {
		mask |= TAC5112_PWR_CFG_ADC_PDZ_BIT;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = tac5112_reg_update(dev, TAC5112_REG_PWR_CFG, mask, 0U);
	k_mutex_unlock(&data->lock);

	return ret;
}

static void codec_start_output(const struct device *dev)
{
	if (!tac5112_dev_valid(dev)) {
		return;
	}

	if (!((struct tac5112_data *)dev->data)->configured) {
		LOG_WRN("start_output() called before a successful configure()");
	}

	(void)codec_start(dev, AUDIO_DAI_DIR_TX);
}

static void codec_stop_output(const struct device *dev)
{
	if (!tac5112_dev_valid(dev)) {
		return;
	}

	(void)codec_stop(dev, AUDIO_DAI_DIR_TX);
}

/**
 * @brief Apply one property to one physical channel (0=left, 1=right).
 *
 * @p idx is always produced by tac5112_channel_mask() and is therefore
 * guaranteed by the caller to be 0 or 1; this function re-checks that
 * invariant anyway before it is used to index a cache array, since the
 * cost of the check is negligible and the consequence of skipping it
 * (an out-of-bounds write) is not.
 */
static int tac5112_apply_property(const struct device *dev, audio_property_t property, uint8_t idx,
				  audio_property_value_t val)
{
	struct tac5112_data *data = dev->data;
	struct tac5112_chan_cache *cache;
	bool is_output;
	uint8_t dvol;
	int ret;

	if (idx >= TAC5112_NUM_CHAN) {
		return -EINVAL;
	}

	is_output = (property == AUDIO_PROPERTY_OUTPUT_VOLUME) ||
		    (property == AUDIO_PROPERTY_OUTPUT_MUTE);
	cache = is_output ? &data->out_cache[idx] : &data->in_cache[idx];

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
	case AUDIO_PROPERTY_INPUT_VOLUME:
		ret = tac5112_dvol_from_half_db(is_output, (int32_t)val.vol, &dvol);
		if (ret < 0) {
			LOG_ERR("Volume %d (0.5 dB units) out of range", val.vol);
			return ret;
		}
		cache->vol_half_db = (int16_t)val.vol;
		if (cache->muted) {
			/* Stay muted; the new volume takes effect on unmute. */
			return 0;
		}
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
	case AUDIO_PROPERTY_INPUT_MUTE:
		cache->muted = val.mute;
		if (val.mute) {
			dvol = TAC5112_DVOL_MUTE;
		} else {
			ret = tac5112_dvol_from_half_db(is_output, cache->vol_half_db, &dvol);
			if (ret < 0) {
				return ret;
			}
		}
		break;
	default:
		return -ENOTSUP;
	}

	if (is_output) {
		struct tac5112_reg reg_a =
			(idx == 0U) ? TAC5112_REG_DAC_CH1A_CFG0 : TAC5112_REG_DAC_CH2A_CFG0;
		struct tac5112_reg reg_b =
			(idx == 0U) ? TAC5112_REG_DAC_CH1B_CFG0 : TAC5112_REG_DAC_CH2B_CFG0;

		ret = tac5112_reg_write(dev, reg_a, dvol);
		if (ret < 0) {
			return ret;
		}
		return tac5112_reg_write(dev, reg_b, dvol);
	}

	{
		struct tac5112_reg adc_reg =
			(idx == 0U) ? TAC5112_REG_ADC_CH1_CFG2 : TAC5112_REG_ADC_CH2_CFG2;

		return tac5112_reg_write(dev, adc_reg, dvol);
	}
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	struct tac5112_data *data;
	bool do_left;
	bool do_right;
	int ret = 0;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	if (property == AUDIO_PROPERTY_EQ_GAIN) {
		/* Biquad/EQ coefficients require 32-bit fixed-point
		 * programmable-coefficient registers not modeled by this
		 * driver; reject explicitly rather than writing something
		 * unverified.
		 */
		LOG_ERR("EQ_GAIN is not supported by this driver");
		return -ENOTSUP;
	}

	if ((property != AUDIO_PROPERTY_OUTPUT_VOLUME) &&
	    (property != AUDIO_PROPERTY_OUTPUT_MUTE) && (property != AUDIO_PROPERTY_INPUT_VOLUME) &&
	    (property != AUDIO_PROPERTY_INPUT_MUTE)) {
		LOG_ERR("Property %d not supported", (int)property);
		return -ENOTSUP;
	}

	tac5112_channel_mask(channel, &do_left, &do_right);
	if (!do_left && !do_right) {
		LOG_ERR("Unsupported channel %d", (int)channel);
		return -ENOTSUP;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (do_left) {
		ret = tac5112_apply_property(dev, property, 0U, val);
	}
	if ((ret == 0) && do_right) {
		ret = tac5112_apply_property(dev, property, 1U, val);
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int codec_apply_properties(const struct device *dev)
{
	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	/* codec_set_property() writes to hardware synchronously; there is
	 * nothing cached to flush.
	 */
	return 0;
}

static uint32_t tac5112_read_fault_bits(const struct device *dev)
{
	uint32_t errors = 0;
	uint8_t live;

	if (tac5112_reg_read(dev, TAC5112_REG_OUT_CH1_LIVE, &live) == 0) {
		if (live & (TAC5112_OUT_CH_SC_OUTP_BIT | TAC5112_OUT_CH_SC_OUTM_BIT)) {
			errors |= AUDIO_CODEC_ERROR_OVERCURRENT;
		}
		if (live & (TAC5112_OUT_CH_VG_FAULT_P_BIT | TAC5112_OUT_CH_VG_FAULT_M_BIT)) {
			errors |= AUDIO_CODEC_ERROR_DC;
		}
	}

	if (tac5112_reg_read(dev, TAC5112_REG_OUT_CH2_LIVE, &live) == 0) {
		if (live & (TAC5112_OUT_CH_SC_OUTP_BIT | TAC5112_OUT_CH_SC_OUTM_BIT)) {
			errors |= AUDIO_CODEC_ERROR_OVERCURRENT;
		}
		if (live & (TAC5112_OUT_CH_VG_FAULT_P_BIT | TAC5112_OUT_CH_VG_FAULT_M_BIT)) {
			errors |= AUDIO_CODEC_ERROR_DC;
		}
	}

	if (tac5112_reg_read(dev, TAC5112_REG_AVDD_IOVDD_STS, &live) == 0) {
		if (live & TAC5112_BRWNOUT_SHDN_STS_BIT) {
			errors |= AUDIO_CODEC_ERROR_UNDERVOLTAGE;
		}
	}

	return errors;
}

static int codec_clear_errors(const struct device *dev)
{
	struct tac5112_data *data;
	uint8_t tmp;
	int ret;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* INT_LTCH0 / OUT_CH1_LTCH / OUT_CH2_LTCH are self-clearing on read
	 * (datasheet Section 8.1.2) and de-assert the codec's fault/interrupt
	 * output once all latched conditions have cleared.
	 */
	ret = tac5112_reg_read(dev, TAC5112_REG_INT_LTCH0, &tmp);
	if (ret == 0) {
		ret = tac5112_reg_read(dev, TAC5112_REG_OUT_CH1_LTCH, &tmp);
	}
	if (ret == 0) {
		ret = tac5112_reg_read(dev, TAC5112_REG_OUT_CH2_LTCH, &tmp);
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static void tac5112_fault_work_handler(struct k_work *work)
{
	struct tac5112_data *data = CONTAINER_OF(work, struct tac5112_data, fault_work);
	const struct device *dev = data->self;
	audio_codec_error_callback_t cb;
	uint32_t errors;

	if (dev == NULL) {
		return;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	errors = tac5112_read_fault_bits(dev);
	cb = data->error_cb;
	k_mutex_unlock(&data->lock);

	if ((errors != 0U) && (cb != NULL)) {
		cb(dev, errors);
	}
}

static void tac5112_fault_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct tac5112_data *data;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	if (cb == NULL) {
		return;
	}

	data = CONTAINER_OF(cb, struct tac5112_data, fault_gpio_cb);
	(void)k_work_submit(&data->fault_work);
}

static int codec_register_error_callback(const struct device *dev, audio_codec_error_callback_t cb)
{
	struct tac5112_data *data;
	const struct tac5112_config *cfg;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	data = dev->data;
	cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->fault_gpio)) {
		LOG_WRN("No fault-gpios in devicetree; registered callback will never fire "
			"asynchronously (poll clear_errors()/DEV_STS registers instead)");
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->error_cb = cb;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int tac5112_init(const struct device *dev)
{
	struct tac5112_data *data;
	const struct tac5112_config *cfg;
	int ret;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	data = dev->data;
	cfg = dev->config;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	k_work_init(&data->fault_work, tac5112_fault_work_handler);
	data->self = dev;
	data->page_cache = TAC5112_PAGE_CACHE_INVALID;
	data->configured = false;
	data->error_cb = NULL;
	memset(&data->out_cache, 0, sizeof(data->out_cache));
	memset(&data->in_cache, 0, sizeof(data->in_cache));

	if (gpio_is_ready_dt(&cfg->reset_gpio)) {
		/* Inactive (per reset-gpios GPIO_ACTIVE_* flags in the
		 * devicetree) releases the codec from hardware reset.
		 */
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	if (gpio_is_ready_dt(&cfg->fault_gpio)) {
		ret = gpio_pin_configure_dt(&cfg->fault_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure fault GPIO (%d)", ret);
			return ret;
		}

		gpio_init_callback(&data->fault_gpio_cb, tac5112_fault_isr,
				   BIT(cfg->fault_gpio.pin));
		ret = gpio_add_callback_dt(&cfg->fault_gpio, &data->fault_gpio_cb);
		if (ret < 0) {
			LOG_ERR("Failed to add fault GPIO callback (%d)", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&cfg->fault_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure fault GPIO interrupt (%d)", ret);
			return ret;
		}
	}

	return 0;
}

/* DEVICE_API(audio_codec, tac5112_driver_api) = { */
static struct audio_codec_api tac5112_driver_api = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
	.clear_errors = codec_clear_errors,
	.register_error_callback = codec_register_error_callback,
	.start = codec_start,
	.stop = codec_stop,
};

#define TAC5112_INIT(inst)                                                                         \
	static struct tac5112_data tac5112_data_##inst;                                            \
	static const struct tac5112_config tac5112_config_##inst = {                               \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.fault_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, fault_gpios, {0}),                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tac5112_init, NULL, &tac5112_data_##inst,                      \
			      &tac5112_config_##inst, POST_KERNEL,                                 \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &tac5112_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TAC5112_INIT)
