/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Zephyr audio codec driver for the Texas Instruments TAC5112 low-power
 * stereo audio codec, targeting an nRF5-series host communicating over
 * I2C for control and I2S for audio data. The codec normally operates as
 * the I2S bus target/peripheral (the nRF5 host generates BCLK/FSYNC); ASI
 * controller mode, where the codec drives both BCLK and FSYNC from a CCLK
 * reference on GPIO2, is also supported (see tac5112_controller_configure())
 * for clock-recovery use cases that trim the codec clock with
 * tac5112_pll_trim().
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
#include <drivers/tac5112.h>

#include "tac5112_private.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tac5112, CONFIG_AUDIO_CODEC_LOG_LEVEL);

/** Number of physical channels (currently, the driver only supports left, right). */
#define TAC5112_NUM_CHAN 2U

/** Rate family selected by FS_MODE in CNT_CLK_CFG2. */
enum tac5112_fs_mode {
	TAC5112_FS_MODE_48KHZ_BASE = 0,
	TAC5112_FS_MODE_44_1KHZ_BASE = TAC5112_FS_MODE_44K1_BIT,
};

/** Allowed difference between a requested rate and a table entry, in Hz. */
#define TAC5112_SAMP_RATE_TOL_HZ 1U

/**
 * A sample rate together with the controller-mode settings that produce it.
 *
 * The codec generates a rate when PASI_SAMP_RATE is set to @c code and FS_MODE to
 * @c fs_mode. The supported rates are listed in tac5112_samp_rates below, one
 * entry of this type for each.
 */
struct tac5112_samp_rate {
	/** Sample rate, in Hz, rounded to the nearest hertz. */
	uint32_t hz;
	/** PASI_SAMP_RATE register value that selects this rate. */
	uint8_t code;
	/** FS_MODE setting that selects the 48 kHz or the 44.1 kHz rate family. */
	enum tac5112_fs_mode fs_mode;
};

/**
 * Table entries for one PASI_SAMP_RATE code: the 48 kHz-family rate @p _hz
 * (FS_MODE = 0) and that rate scaled by 44.1/48 (FS_MODE = 1), rounded to the
 * nearest hertz.
 */
#define TAC5112_SAMP_RATE(_code, _hz)                                                              \
	{(_hz), (_code), TAC5112_FS_MODE_48KHZ_BASE},                                              \
	{                                                                                          \
		(((_hz) * 441U + 240U) / 480U), (_code), TAC5112_FS_MODE_44_1KHZ_BASE              \
	}

/**
 * Sample rates the codec can generate in controller mode that the driver
 * supports. Each PASI_SAMP_RATE code gives a 48 kHz-family rate with
 * FS_MODE = 0 and that rate scaled by 44.1/48 with FS_MODE = 1, so most codes
 * appear twice (see TAC5112_SAMP_RATE()).
 *
 * The codec also defines rates below TAC5112_FS_MIN_HZ (codes 39 and 40 in both
 * families, and code 38 in the 44.1 kHz family). They are omitted because
 * tac5112_dai_config_valid() rejects such rates before this table is searched.
 */
static const struct tac5112_samp_rate tac5112_samp_rates[] = {
	TAC5112_SAMP_RATE(1, 768000),
	TAC5112_SAMP_RATE(2, 614400),
	TAC5112_SAMP_RATE(3, 512000),
	TAC5112_SAMP_RATE(4, 438857),
	TAC5112_SAMP_RATE(5, 384000),
	TAC5112_SAMP_RATE(6, 341333),
	TAC5112_SAMP_RATE(7, 307200),
	TAC5112_SAMP_RATE(8, 256000),
	TAC5112_SAMP_RATE(9, 219429),
	TAC5112_SAMP_RATE(10, 192000),
	TAC5112_SAMP_RATE(11, 170667),
	TAC5112_SAMP_RATE(12, 153600),
	TAC5112_SAMP_RATE(13, 128000),
	TAC5112_SAMP_RATE(14, 109714),
	TAC5112_SAMP_RATE(15, 96000),
	TAC5112_SAMP_RATE(16, 85333),
	TAC5112_SAMP_RATE(17, 76800),
	TAC5112_SAMP_RATE(18, 64000),
	TAC5112_SAMP_RATE(19, 54857),
	TAC5112_SAMP_RATE(20, 48000),
	TAC5112_SAMP_RATE(21, 42667),
	TAC5112_SAMP_RATE(22, 38400),
	TAC5112_SAMP_RATE(23, 32000),
	TAC5112_SAMP_RATE(24, 27429),
	TAC5112_SAMP_RATE(25, 24000),
	TAC5112_SAMP_RATE(26, 21333),
	TAC5112_SAMP_RATE(27, 19200),
	TAC5112_SAMP_RATE(28, 16000),
	TAC5112_SAMP_RATE(29, 13714),
	TAC5112_SAMP_RATE(30, 12000),
	TAC5112_SAMP_RATE(31, 10667),
	TAC5112_SAMP_RATE(32, 9600),
	TAC5112_SAMP_RATE(33, 8000),
	TAC5112_SAMP_RATE(34, 6857),
	TAC5112_SAMP_RATE(35, 6000),
	TAC5112_SAMP_RATE(36, 5333),
	TAC5112_SAMP_RATE(37, 4800),
	/* Code 38 in the 44.1 kHz family (3675 Hz) is below TAC5112_FS_MIN_HZ. */
	{4000, 38, TAC5112_FS_MODE_48KHZ_BASE},
};

/** CCLK frequencies selectable with CCLK_FREQ_SEL (fixed-CCLK mode). */
static const uint32_t tac5112_cclk_freqs_hz[] = {
	12000000U, 12288000U, 13000000U, 16000000U, 19200000U, 19680000U, 24000000U, 24576000U,
};

/**
 * @brief Validate that @p dev is a usable device handle for this driver.
 *
 * Called at the top of every function reachable from the public API
 * before `dev->data` or `dev->config` is dereferenced.
 *
 * @param[in] dev  Device handle to validate.
 *
 * @retval true on success.
 * @retval false if @p dev is unusable.
 */
static inline bool tac5112_dev_valid(const struct device *dev)
{
	return (dev != NULL) && (dev->data != NULL) && (dev->config != NULL);
}

/**
 * @brief Validate the Digital Audio Interface (DAI) configuration and set
 * the I2S format and word size.
 *
 * @param[in]  cfg       Codec configuration parameters.
 * @param[out] format    Pointer to the I2S format
 * @param[out] word_len  Pointer to the I2S word length.
 *
 * @return 0 if successful, error otherwise.
 */
static int tac5112_dai_config_valid(const struct audio_codec_cfg *cfg, uint8_t *format,
				    uint8_t *word_len)
{
	const struct i2s_config *i2s;

	switch (cfg->dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		*format = TAC5112_PASI_FORMAT_I2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		*format = TAC5112_PASI_FORMAT_LJ;
		break;
	case AUDIO_DAI_TYPE_PCMA:
		/* fall through */
	case AUDIO_DAI_TYPE_PCMB:
		/* fall through */
	case AUDIO_DAI_TYPE_PCM:
		/* fall through */
	default:
		LOG_ERR("Unsupported dai_type %d", (int)cfg->dai_type);
		return -ENOTSUP;
	}

	i2s = &cfg->dai_cfg.i2s;

	if ((i2s->frame_clk_freq < TAC5112_FS_MIN_HZ) ||
	    (i2s->frame_clk_freq > TAC5112_FS_MAX_HZ)) {
		LOG_ERR("Unsupported sample rate %u Hz (TAC5112 supports 4 kHz-768 kHz)",
			i2s->frame_clk_freq);
		return -EINVAL;
	}

	bool bit_clk = (i2s->options & I2S_OPT_BIT_CLK_TARGET) != 0U;
	bool frm_clk = (i2s->options & I2S_OPT_FRAME_CLK_TARGET) != 0U;

	if (bit_clk != frm_clk) {
		LOG_ERR("Mixed clock directions are not supported: BCLK and FSYNC must both be "
			"controller or both be target");
		return -ENOTSUP;
	}

	if ((i2s->format & I2S_FMT_DATA_ORDER_LSB) != 0U) {
		LOG_ERR("LSB-first data order is not supported");
		return -ENOTSUP;
	}

	if (i2s->channels != TAC5112_NUM_CHAN) {
		LOG_ERR("TAC5112 driver requires 2 channels, got channels=%u", i2s->channels);
		return -EINVAL;
	}

	switch (i2s->word_size) {
	case AUDIO_PCM_WIDTH_16_BITS:
		*word_len = TAC5112_PASI_WLEN_16;
		break;
	case AUDIO_PCM_WIDTH_20_BITS:
		*word_len = TAC5112_PASI_WLEN_20;
		break;
	case AUDIO_PCM_WIDTH_24_BITS:
		*word_len = TAC5112_PASI_WLEN_24;
		break;
	case AUDIO_PCM_WIDTH_32_BITS:
		*word_len = TAC5112_PASI_WLEN_32;
		break;
	default:
		LOG_ERR("Unsupported word size %u", i2s->word_size);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Check if the configuration makes the HW codec the ASI clock controller.
 *
 * @param[in]  cfg  Codec configuration parameters.
 *
 * @retval true on success.
 * @retval false if @p cfg is not the ASI controller.
 */
static inline bool tac5112_is_asi_clk_controller(const struct audio_codec_cfg *cfg)
{
	return ((cfg->dai_cfg.i2s.options & I2S_OPT_BIT_CLK_TARGET) == 0U);
}

int tac5112_dvol_from_half_db(enum tac5112_path path, int32_t half_db, uint8_t *dvol_out)
{
	int32_t half_db_min;
	int32_t half_db_max;
	int32_t zero_db_reg;
	int32_t vol_db_max;
	int32_t vol_db_min;
	int32_t dvol_reg_value;

	if (dvol_out == NULL) {
		return -EINVAL;
	}

	switch (path) {
	case TAC5112_PATH_OUTPUT:
		zero_db_reg = (int32_t)TAC5112_DAC_DVOL_0DB;
		half_db_min = TAC5112_DAC_VOL_HALF_DB_MIN;
		half_db_max = TAC5112_DAC_VOL_HALF_DB_MAX;
		vol_db_max = TAC5112_DAC_DVOL_MAX;
		vol_db_min = TAC5112_DAC_DVOL_MIN;
		break;
	case TAC5112_PATH_INPUT:
		zero_db_reg = (int32_t)TAC5112_ADC_DVOL_0DB;
		half_db_min = TAC5112_ADC_VOL_HALF_DB_MIN;
		half_db_max = TAC5112_ADC_VOL_HALF_DB_MAX;
		vol_db_max = TAC5112_ADC_DVOL_MAX;
		vol_db_min = TAC5112_ADC_DVOL_MIN;
		break;
	default:
		return -EINVAL;
	}

	if ((half_db < half_db_min) || (half_db > half_db_max)) {
		return -EINVAL;
	}

	dvol_reg_value = zero_db_reg + half_db;
	dvol_reg_value = CLAMP(dvol_reg_value, vol_db_min, vol_db_max);

	*dvol_out = (uint8_t)dvol_reg_value;
	return 0;
}

/**
 * @brief Select the register page for the next access.
 *
 * The page is written before every register access instead of being cached, so
 * the driver never depends on the codec's page pointer still matching a host-side
 * copy.
 *
 * @param[in] cfg   Device configuration.
 * @param[in] page  Register page to select.
 *
 * @return 0 if successful, error otherwise.
 */
static int tac5112_page_select(const struct tac5112_config *cfg, uint8_t page)
{
	int ret;

	ret = i2c_reg_write_byte_dt(&cfg->bus, TAC5112_REG_PAGE_CFG.addr, page);

	if (ret != 0) {
		LOG_ERR("Page select %u failed (%d)", page, ret);
	}

	return ret;
}

int tac5112_reg_write(const struct device *dev, struct tac5112_reg reg, uint8_t val)
{
	int ret;
	const struct tac5112_config *cfg;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	cfg = dev->config;

	ret = tac5112_page_select(cfg, reg.page);
	if (ret != 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->bus, reg.addr, val);
	if (ret != 0) {
		LOG_ERR("Write pg:%u reg:0x%02x val:0x%02x failed (%d)", reg.page, reg.addr, val,
			ret);
		return ret;
	}

	LOG_DBG("WR pg:%u reg:0x%02x val:0x%02x", reg.page, reg.addr, val);
	return 0;
}

int tac5112_reg_write_burst(const struct device *dev, struct tac5112_reg first_reg,
			    const uint8_t *vals, size_t len)
{
	int ret;
	const struct tac5112_config *cfg;

	if (!tac5112_dev_valid(dev) || (vals == NULL) || (len == 0U) ||
	    (((size_t)first_reg.addr + len) > TAC5112_REG_ADDR_COUNT)) {
		return -EINVAL;
	}

	cfg = dev->config;

	ret = tac5112_page_select(cfg, first_reg.page);
	if (ret != 0) {
		return ret;
	}

	ret = i2c_burst_write_dt(&cfg->bus, first_reg.addr, vals, len);
	if (ret != 0) {
		LOG_ERR("Burst write pg:%u reg:0x%02x len:%u failed (%d)", first_reg.page,
			first_reg.addr, (unsigned int)len, ret);
		return ret;
	}

	LOG_DBG("WR pg:%u reg:0x%02x len:%u", first_reg.page, first_reg.addr, (unsigned int)len);
	return 0;
}

int tac5112_reg_read(const struct device *dev, struct tac5112_reg reg, uint8_t *val)
{
	int ret;
	const struct tac5112_config *cfg;

	if (!tac5112_dev_valid(dev) || (val == NULL)) {
		return -EINVAL;
	}

	cfg = dev->config;

	ret = tac5112_page_select(cfg, reg.page);
	if (ret != 0) {
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&cfg->bus, reg.addr, val);
	if (ret != 0) {
		LOG_ERR("Read pg:%u reg:0x%02x failed (%d)", reg.page, reg.addr, ret);
		return ret;
	}

	LOG_DBG("RD pg:%u reg:0x%02x val:0x%02x", reg.page, reg.addr, *val);
	return 0;
}

int tac5112_reg_update(const struct device *dev, struct tac5112_reg reg, uint8_t mask, uint8_t val)
{
	int ret;
	uint8_t reg_val_cur;
	uint8_t reg_val_new;

	ret = tac5112_reg_read(dev, reg, &reg_val_cur);
	if (ret != 0) {
		return ret;
	}

	reg_val_new = (uint8_t)((reg_val_cur & (uint8_t)~mask) | (val & mask));

	return tac5112_reg_write(dev, reg, reg_val_new);
}

static int pll_configure_auto(const struct device *dev, const struct tac5112_pll_config *pll)
{
	int ret;

	/* Auto mode is in effect: the PLL can run again. */
	ret = tac5112_reg_update(dev, TAC5112_REG_CLK_CFG2, TAC5112_PLL_DIS_BIT, 0U);
	if (ret == 0) {
		LOG_DBG("PLL: auto clock configuration, clk_src=%u", (unsigned int)pll->clk_src);
	}

	return ret;
}

static int pll_configure_custom(const struct device *dev, const struct tac5112_pll_config *pll)
{
	int ret;
	uint8_t jmul_msb;
	uint8_t jmul_lsb;
	uint8_t dmul_msb;
	uint8_t dmul_lsb;
	uint8_t clk_cfg16;

	jmul_msb = (uint8_t)((pll->jmul >> 8) & 0x1U);
	jmul_lsb = (uint8_t)(pll->jmul & 0xFFU);
	dmul_msb = (uint8_t)((pll->dmul >> 8) & TAC5112_PLL_DMUL_MSB_MASK);
	dmul_lsb = (uint8_t)(pll->dmul & 0xFFU);
	clk_cfg16 = (uint8_t)((jmul_msb != 0U ? TAC5112_PLL_JMUL_MSB_BIT : 0U) | dmul_msb);

	/* CLK_CFG15..CLK_CFG20 are contiguous: one burst writes PDIV, JMUL, DMUL,
	 * NDIV, MDIV and the divider selects. The PLL is disabled meanwhile, so
	 * the order the codec applies the bytes in does not matter.
	 */
	{
		const uint8_t dividers[] = {
			pll->pdiv,
			clk_cfg16,
			dmul_lsb,
			jmul_lsb,
			(uint8_t)((pll->ndiv << TAC5112_NDIV_SHIFT) |
				  (pll->pdm_div_sel << TAC5112_PDM_DIV_SHIFT)),
			(uint8_t)((pll->mdiv << TAC5112_MDIV_SHIFT) | pll->adc_modclk_div_sel),
		};

		ret = tac5112_reg_write_burst(dev, TAC5112_REG_CLK_CFG15, dividers,
					      ARRAY_SIZE(dividers));
		if (ret != 0) {
			return ret;
		}
	}

	/* All dividers are in place: ensure the PLL is running. */
	ret = tac5112_reg_update(dev, TAC5112_REG_CLK_CFG2, TAC5112_PLL_DIS_BIT, 0U);
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("PLL: custom cfg pdiv=%u jmul=%u dmul=%u ndiv=%u mdiv=%u", pll->pdiv, pll->jmul,
		pll->dmul, pll->ndiv, pll->mdiv);

	return 0;
}

int tac5112_pll_configure(const struct device *dev, const struct tac5112_pll_config *pll)
{
	int ret;
	struct tac5112_data *data;
	uint8_t clk_cfg2_val;

	if (!tac5112_dev_valid(dev) || (pll == NULL)) {
		return -EINVAL;
	}

	if ((unsigned int)pll->clk_src > TAC5112_CLK_SRC_SEL_MAX) {
		LOG_ERR("Invalid PLL clock source %u", (unsigned int)pll->clk_src);
		return -EINVAL;
	}

	if (pll->auto_config && (pll->clk_src == TAC5112_PLL_CLK_SRC_INTERNAL_OSC)) {
		LOG_ERR("Internal oscillator clock source requires custom PLL configuration");
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

	k_mutex_lock(&data->lock, K_FOREVER);

	data->pll_custom_init = false;

	/* The dividers are rewritten byte by byte in custom mode, and the
	 * multiplier can pass through values outside its valid range part way
	 * (for example 0x26FF -> 0x2700 via 0x27FF). A failed custom
	 * configuration can also leave such a value behind, which switching back
	 * to auto mode must not run on either. So in both modes the PLL is
	 * disabled first and only enabled once every write of the mode change
	 * succeeded; a failure part way leaves it disabled.
	 */
	clk_cfg2_val = (uint8_t)(((uint8_t)pll->clk_src << TAC5112_CLK_SRC_SEL_SHIFT) |
				 (pll->fractional_allowed ? TAC5112_AUTO_PLL_FR_ALLOW_BIT : 0U) |
				 TAC5112_PLL_DIS_BIT);

	ret = tac5112_reg_update(dev, TAC5112_REG_CLK_CFG2,
				 (uint8_t)(TAC5112_PLL_DIS_BIT | TAC5112_AUTO_PLL_FR_ALLOW_BIT |
					   TAC5112_CLK_SRC_SEL_MASK),
				 clk_cfg2_val);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/* Set the PLL mode. */
	ret = tac5112_reg_update(dev, TAC5112_REG_CLK_CFG0, TAC5112_CUSTOM_CLK_CFG_BIT,
				 pll->auto_config ? 0U : TAC5112_CUSTOM_CLK_CFG_BIT);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (pll->auto_config) {
		ret = pll_configure_auto(dev, pll);
	} else {
		ret = pll_configure_custom(dev, pll);
		if (ret == 0) {
			data->pll_custom_init = true;
			data->pll_jmul = pll->jmul;
			data->pll_dmul = pll->dmul;
		}
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief DMUL numerator after a ppb offset, before any range limiting.
 *
 * The result can be below 0 or above TAC5112_PLL_DMUL_MAX; callers decide how
 * to treat that.
 *
 * @param[in] jmul  Baseline PLL integer multiplier (JMUL).
 * @param[in] dmul  Baseline PLL fractional numerator (DMUL), 0 to 9999.
 * @param[in] ppb   Requested offset from nominal, in parts per billion.
 *
 * @return The DMUL value that gives the requested offset, not limited to the valid range.
 */
static int64_t tac5112_dmul_trim_unclamped(uint16_t jmul, uint16_t dmul, int32_t ppb)
{
	int64_t m_scaled;
	int64_t numerator;
	int64_t delta;

	/* fs is proportional to the PLL multiplier M = jmul + dmul/10000, so a
	 * fractional offset of `ppb` parts-per-billion shifts the DMUL numerator
	 * (M scaled by 10000) by dDMUL = (jmul*10000 + dmul) * ppb / 1e9.
	 */
	m_scaled = (int64_t)jmul * 10000 + (int64_t)dmul;
	numerator = m_scaled * (int64_t)ppb;

	/* Round to nearest, away from zero, for both signs. */
	if (numerator >= 0) {
		delta = (numerator + 500000000LL) / 1000000000LL;
	} else {
		delta = (numerator - 500000000LL) / 1000000000LL;
	}

	return (int64_t)dmul + delta;
}

uint16_t tac5112_dmul_trim(uint16_t jmul, uint16_t dmul, int32_t ppb)
{
	return CLAMP(tac5112_dmul_trim_unclamped(jmul, dmul, ppb), 0, TAC5112_PLL_DMUL_MAX);
}

int tac5112_pll_trim(const struct device *dev, int32_t ppb)
{
	int ret;
	struct tac5112_data *data;
	uint16_t new_dmul;
	int64_t target_dmul;
	bool saturated;
	uint8_t old_cfg16;
	uint8_t new_cfg16;
	uint8_t old_cfg17;
	uint8_t new_cfg17;
	uint8_t first_new;
	uint8_t second_new;
	uint16_t old_dmul;
	struct tac5112_reg first_reg;
	struct tac5112_reg second_reg;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->pll_custom_init) {
		LOG_ERR("PLL trim requires an active custom-mode PLL configuration");
		k_mutex_unlock(&data->lock);
		return -ENOTSUP;
	}

	target_dmul = tac5112_dmul_trim_unclamped(data->pll_jmul, data->pll_dmul, ppb);
	saturated = (target_dmul < 0) || (target_dmul > (int64_t)TAC5112_PLL_DMUL_MAX);
	new_dmul = CLAMP(target_dmul, 0, TAC5112_PLL_DMUL_MAX);

	ret = tac5112_reg_read(dev, TAC5112_REG_CLK_CFG16, &old_cfg16);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = tac5112_reg_read(dev, TAC5112_REG_CLK_CFG17, &old_cfg17);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	old_dmul = (uint16_t)(((old_cfg16 & TAC5112_PLL_DMUL_MSB_MASK) << 8) | old_cfg17);
	new_cfg16 = (uint8_t)((old_cfg16 & (uint8_t)~TAC5112_PLL_DMUL_MSB_MASK) |
			      ((new_dmul >> 8) & TAC5112_PLL_DMUL_MSB_MASK));
	new_cfg17 = (uint8_t)(new_dmul & 0xFFU);

	/* DMUL is a 14-bit value split across two registers, and the two bytes can’t be written
	 * together. The write order decides what the PLL sees between the two writes.
	 *
	 * The high bits are PLL_DMUL_MSB[5:0] in CLK_CFG16. The low byte is PLL_DMUL_LSB in
	 * CLK_CFG17. They are two separate I2C writes. Between the writes the codec has the new
	 * value in one register and the old value in the other, so for a short time DMUL is a
	 * mixture of the two. The valid range is 0 to 9999 (0x270F). Values from 10000 up are
	 * reserved. A mixed value that lands above 9999 is a reserved setting, even for a moment.
	 * Neither order is right every time, so the driver checks. The lsb_first expression works
	 * out the intermediate value for the LSB-first order: the old high byte combined with the
	 * new low byte. If that is at most TAC5112_PLL_DMUL_MAX it writes the LSB first. Otherwise
	 * it writes the MSB first.
	 */
	if ((uint16_t)(((old_dmul & 0xFF00U) | new_cfg17)) <= TAC5112_PLL_DMUL_MAX) {
		first_reg = TAC5112_REG_CLK_CFG17;
		second_reg = TAC5112_REG_CLK_CFG16;
		first_new = new_cfg17;
		second_new = new_cfg16;
	} else {
		if (((new_dmul & 0xFF00U) | (old_dmul & 0xFFU)) > TAC5112_PLL_DMUL_MAX) {
			/* Unreachable for valid old/new values; refuse rather than glitch. */
			LOG_ERR("No glitch-free DMUL update order from %u to %u", old_dmul,
				new_dmul);
			k_mutex_unlock(&data->lock);
			return -EINVAL;
		}

		first_reg = TAC5112_REG_CLK_CFG16;
		second_reg = TAC5112_REG_CLK_CFG17;
		first_new = new_cfg16;
		second_new = new_cfg17;
	}

	ret = tac5112_reg_write(dev, first_reg, first_new);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = tac5112_reg_write(dev, second_reg, second_new);
	if (ret != 0) {
		/* The first byte is already written and is not undone: a bus that
		 * just failed is unlikely to take the write back. DMUL is now a
		 * valid but unknown value, so trimming is refused until the PLL is
		 * reconfigured.
		 */
		LOG_ERR("PLL trim failed (%d); trimming disabled until the PLL is reconfigured",
			ret);
		data->pll_custom_init = false;
		k_mutex_unlock(&data->lock);
		return ret;
	}

	LOG_DBG("PLL trim %d ppb: DMUL %u -> %u", ppb, data->pll_dmul, new_dmul);

	k_mutex_unlock(&data->lock);

	if (saturated) {
		/* The nearest valid DMUL was applied, but it is short of the request. */
		LOG_WRN("PLL trim %d ppb out of range: DMUL limited to %u", ppb, new_dmul);
		return -ERANGE;
	}

	return 0;
}

int tac5112_pll_baseline_get(const struct device *dev, uint16_t *jmul, uint16_t *dmul)
{
	struct tac5112_data *data;
	int ret = 0;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->pll_custom_init) {
		ret = -ENOTSUP;
	} else {
		if (jmul != NULL) {
			*jmul = data->pll_jmul;
		}

		if (dmul != NULL) {
			*dmul = data->pll_dmul;
		}
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/**
 * @brief Configure the primary audio serial interface (format, word
 * length) from a Zephyr audio_codec_cfg/i2s_config pair.
 *
 * In target mode (I2S_OPT_BIT_CLK_TARGET and I2S_OPT_FRAME_CLK_TARGET set) the
 * host (nRF5) generates BCLK and FSYNC. With both options cleared, the codec is
 * the controller and drives both clocks; the rest of that setup (direction,
 * CCLK source, sample rate and BCLK/FSYNC ratio) is done by
 * tac5112_controller_configure(). Mixed directions are rejected by
 * tac5112_dai_config_valid().
 *
 * @param[in] dev  Device handle.
 * @param[in] cfg  Pointer to the configuration.
 *
 * @return 0 if successful, error otherwise.
 */
static int tac5112_dai_configure(const struct device *dev, const struct audio_codec_cfg *cfg)
{
	int ret;
	const struct i2s_config *i2s = &cfg->dai_cfg.i2s;
	uint8_t format;
	uint8_t wlen;
	uint8_t val;

	ret = tac5112_dai_config_valid(cfg, &format, &wlen);
	if (ret != 0) {
		LOG_ERR("Invalid DAI");
		return -EINVAL;
	}

	val = (uint8_t)((format << TAC5112_PASI_FORMAT_SHIFT) | (wlen << TAC5112_PASI_WLEN_SHIFT));

	if ((i2s->format & I2S_FMT_BIT_CLK_INV) != 0U) {
		val |= TAC5112_PASI_BCLK_POL_BIT;
	}

	if ((i2s->format & I2S_FMT_FRAME_CLK_INV) != 0U) {
		val |= TAC5112_PASI_FSYNC_POL_BIT;
	}

	ret = tac5112_reg_update(dev, TAC5112_REG_PASI_CFG0,
				 (uint8_t)(TAC5112_PASI_FORMAT_MASK | TAC5112_PASI_WLEN_MASK |
					   TAC5112_PASI_BCLK_POL_BIT | TAC5112_PASI_FSYNC_POL_BIT),
				 val);
	if (ret != 0) {
		return ret;
	}

	/* Stereo slot assignment. After reset, channel 2 sits in the left half
	 * (slot 1), which suits TDM but not I2S or left-justified: channel 1
	 * must be left slot 0 and channel 2 right slot 0 (slot 16), for both
	 * the transmit (ADC) and receive (DAC) paths.
	 */
	{
		const uint8_t slots[] = {
			(uint8_t)(TAC5112_PASI_CH_EN_BIT | TAC5112_PASI_SLOT_LEFT0),
			(uint8_t)(TAC5112_PASI_CH_EN_BIT | TAC5112_PASI_SLOT_RIGHT0),
		};

		/* Channel 1 and 2 of each path are adjacent registers. */
		ret = tac5112_reg_write_burst(dev, TAC5112_REG_PASI_TX_CH1_CFG, slots,
					      ARRAY_SIZE(slots));
		if (ret != 0) {
			return ret;
		}

		ret = tac5112_reg_write_burst(dev, TAC5112_REG_PASI_RX_CH1_CFG, slots,
					      ARRAY_SIZE(slots));
		if (ret != 0) {
			return ret;
		}
	}

	if (tac5112_is_asi_clk_controller(cfg)) {
		val = (uint8_t)(TAC5112_PASI_TX_USE_INT_BCLK_BIT |
				TAC5112_PASI_TX_USE_INT_FSYNC_BIT);
	} else {
		val = 0U;
	}

	ret = tac5112_reg_update(
		dev, TAC5112_REG_PASI_TX_CFG0,
		(uint8_t)(TAC5112_PASI_TX_USE_INT_BCLK_BIT | TAC5112_PASI_TX_USE_INT_FSYNC_BIT),
		val);

	return ret;
}

/**
 * @brief Find the controller-mode setting that produces a sample rate.
 *
 * In controller mode the codec generates FSYNC itself, and PASI_SAMP_RATE,
 * together with FS_MODE, fix the rate it produces. The requested rate must match
 * one of the rates in tac5112_samp_rates to within TAC5112_SAMP_RATE_TOL_HZ; the
 * closest match is returned.
 *
 * The tolerance only absorbs the rounding of the table values to whole hertz
 * (for example 438857.14 Hz is stored as 438857). It is not a frequency margin.
 *
 * A rate that is merely close to a supported one, such as 45 kHz, or a
 * 44.1 kHz submultiple that is not in the table, is rejected. Accepting it
 * would silently generate a different rate from the one requested.
 *
 * @param[in] fs  Requested sample rate, in Hz.
 *
 * @return The matching table entry, or NULL if the codec cannot generate @p fs.
 */
static const struct tac5112_samp_rate *tac5112_controller_rate_find(uint32_t fs)
{
	const struct tac5112_samp_rate *best = NULL;
	uint32_t best_diff = UINT32_MAX;

	for (int i = 0; i < ARRAY_SIZE(tac5112_samp_rates); i++) {
		const struct tac5112_samp_rate *rate = &tac5112_samp_rates[i];
		uint32_t diff = (fs > rate->hz) ? (fs - rate->hz) : (rate->hz - fs);

		if ((diff <= TAC5112_SAMP_RATE_TOL_HZ) && (diff < best_diff)) {
			best_diff = diff;
			best = rate;
		}
	}

	return best;
}

/**
 * @brief Validate a controller-mode configuration and work out the register settings.
 *
 * Follows TI's "Clocking Configuration of Device and Flexible Clocking For
 * TAx5x1x Family" (SLAAEG6A), Auto MCLK Ratio / Auto MCLK Fixed modes. The
 * reference clock (CCLK, from audio_codec_cfg::mclk_freq) is expected on GPIO2.
 * Both BCLK and FSYNC are generated by the codec.
 *
 * - If mclk_freq is an integer multiple of the sample rate, the codec uses
 *   CLK_SRC_SEL = CCLK synchronised with the primary FSYNC, with the ratio
 *   programmed explicitly.
 * - Otherwise mclk_freq must be one of the CCLK_FREQ_SEL frequencies and
 *   CLK_SRC_SEL = fixed CCLK is used.
 *
 * Touches no hardware and no driver state, so a rejected configuration leaves
 * the codec unchanged.
 *
 * @param[in]  cfg  Pointer to the configuration.
 * @param[out] out  Receives the settings to write.
 *
 * @retval 0 if the configuration is valid and @p out is filled in.
 * @retval -EINVAL if the clock, sample rate or BCLK ratio cannot be generated.
 */
int tac5112_controller_calc(const struct audio_codec_cfg *cfg, struct tac5112_controller_cfg *out)
{
	const struct i2s_config *i2s = &cfg->dai_cfg.i2s;
	uint32_t fs = i2s->frame_clk_freq;
	uint32_t mclk = cfg->mclk_freq;
	const struct tac5112_samp_rate *rate;

	out->cnt_clk_cfg2_mask = (uint8_t)(TAC5112_PASI_CNT_CFG_BIT | TAC5112_FS_MODE_44K1_BIT);
	out->cnt_clk_cfg2 = TAC5112_PASI_CNT_CFG_BIT;
	out->cclk_ratio = 0U;
	out->bclk_ratio = i2s->word_size * i2s->channels;

	if ((mclk < TAC5112_CCLK_MIN_HZ) || (mclk > TAC5112_CCLK_MAX_HZ)) {
		LOG_ERR("Controller mode needs mclk_freq (CCLK on GPIO2) in [%u, %u] Hz, got %u",
			TAC5112_CCLK_MIN_HZ, TAC5112_CCLK_MAX_HZ, mclk);
		return -EINVAL;
	}

	if ((mclk % fs) == 0U) {
		/* CCLK is an integer multiple of FSYNC: the codec needs that
		 * ratio explicitly, the reset value (0) only means auto-detect.
		 */
		out->cclk_ratio = mclk / fs;
		if (out->cclk_ratio > TAC5112_CCLK_FS_RATIO_MAX) {
			LOG_ERR("CCLK/FSYNC ratio %u exceeds %u", out->cclk_ratio,
				TAC5112_CCLK_FS_RATIO_MAX);
			return -EINVAL;
		}

		out->clk_src = TAC5112_PLL_CLK_SRC_CCLK_PASI_FSYNC;
	} else {
		int i;

		for (i = 0; i < ARRAY_SIZE(tac5112_cclk_freqs_hz); i++) {
			if (tac5112_cclk_freqs_hz[i] == mclk) {
				break;
			}
		}

		if (i == ARRAY_SIZE(tac5112_cclk_freqs_hz)) {
			LOG_ERR("mclk_freq %u Hz is neither a multiple of %u Hz nor a supported "
				"fixed CCLK frequency",
				mclk, fs);
			return -EINVAL;
		}

		out->clk_src = TAC5112_PLL_CLK_SRC_FIXED_CCLK;
		out->cnt_clk_cfg2_mask |= TAC5112_CCLK_FREQ_SEL_MASK;
		out->cnt_clk_cfg2 |= (uint8_t)(i << TAC5112_CCLK_FREQ_SEL_SHIFT);
	}

	rate = tac5112_controller_rate_find(fs);
	if (rate == NULL) {
		LOG_ERR("Controller mode cannot generate a %u Hz sample rate", fs);
		return -EINVAL;
	}

	if ((out->bclk_ratio == 0U) || (out->bclk_ratio > TAC5112_PASI_BCLK_FS_RATIO_MAX)) {
		return -EINVAL;
	}

	out->cnt_clk_cfg2 |= (uint8_t)rate->fs_mode;
	out->samp_rate_code = rate->code;

	return 0;
}

/**
 * @brief Write controller-mode settings to the codec.
 *
 * CCLK arrives on GPIO2, so the pin is made an input and routed to CCLK before
 * the clock registers are written. Does not validate: @p ctrl comes from
 * tac5112_controller_calc().
 *
 * @param[in] dev   Device handle.
 * @param[in] ctrl  Settings to write.
 *
 * @return 0 if successful, error otherwise.
 */
static int tac5112_controller_write(const struct device *dev,
				    const struct tac5112_controller_cfg *ctrl)
{
	int ret;

	/* CCLK arrives on GPIO2: make the pin an input and route it to CCLK. */
	ret = tac5112_reg_update(dev, TAC5112_REG_GPIO2_CFG0, TAC5112_GPIO2_CFG_MASK,
				 (uint8_t)(TAC5112_GPIO2_CFG_INPUT << TAC5112_GPIO2_CFG_SHIFT));
	if (ret != 0) {
		return ret;
	}

	ret = tac5112_reg_update(
		dev, TAC5112_REG_INTF_CFG0, TAC5112_INTF_CFG0_CCLK_SEL_MASK,
		(uint8_t)(TAC5112_INTF_CFG0_CCLK_SEL_GPIO2 << TAC5112_INTF_CFG0_CCLK_SEL_SHIFT));
	if (ret != 0) {
		return ret;
	}

	/* Controller mode, 48/44.1 kHz family and (fixed mode) CCLK frequency. */
	ret = tac5112_reg_update(dev, TAC5112_REG_CNT_CLK_CFG2, ctrl->cnt_clk_cfg2_mask,
				 ctrl->cnt_clk_cfg2);
	if (ret != 0) {
		return ret;
	}

	/* CCLK/FSYNC ratio (CCLK synchronised with FSYNC mode only). */
	if (ctrl->cclk_ratio != 0U) {
		ret = tac5112_reg_update(
			dev, TAC5112_REG_CNT_CLK_CFG0, TAC5112_CCLK_FS_RATIO_MSB_MASK,
			(uint8_t)((ctrl->cclk_ratio >> 8) & TAC5112_CCLK_FS_RATIO_MSB_MASK));
		if (ret != 0) {
			return ret;
		}

		ret = tac5112_reg_write(dev, TAC5112_REG_CNT_CLK_CFG1,
					(uint8_t)(ctrl->cclk_ratio & 0xFFU));
		if (ret != 0) {
			return ret;
		}
	}

	/* BCLK/FSYNC ratio and the sample rate the codec generates. */
	ret = tac5112_reg_update(
		dev, TAC5112_REG_CNT_CLK_CFG3, TAC5112_PASI_BCLK_FS_RATIO_MSB_MASK,
		(uint8_t)((ctrl->bclk_ratio >> 8) & TAC5112_PASI_BCLK_FS_RATIO_MSB_MASK));
	if (ret != 0) {
		return ret;
	}

	ret = tac5112_reg_write(dev, TAC5112_REG_CNT_CLK_CFG4, (uint8_t)(ctrl->bclk_ratio & 0xFFU));
	if (ret != 0) {
		return ret;
	}

	return tac5112_reg_update(dev, TAC5112_REG_CLK_CFG0, TAC5112_PASI_SAMP_RATE_MASK,
				  (uint8_t)(ctrl->samp_rate_code << TAC5112_PASI_SAMP_RATE_SHIFT));
}

/**
 * @brief Set up the codec as the primary ASI controller.
 *
 * Validates the configuration with tac5112_controller_calc() first, so nothing
 * is written for a configuration the codec cannot generate, and then writes it
 * with tac5112_controller_write().
 *
 * @param[in]  dev     Device handle.
 * @param[in]  cfg     Pointer to the configuration.
 * @param[out] clk_src The CLK_SRC_SEL value tac5112_pll_configure() must use.
 *
 * @return 0 if successful, error otherwise.
 */
static int tac5112_controller_configure(const struct device *dev, const struct audio_codec_cfg *cfg,
					enum tac5112_pll_clk_src *clk_src)
{
	struct tac5112_controller_cfg ctrl;
	int ret;

	ret = tac5112_controller_calc(cfg, &ctrl);
	if (ret != 0) {
		return ret;
	}

	ret = tac5112_controller_write(dev, &ctrl);
	if (ret != 0) {
		return ret;
	}

	*clk_src = ctrl.clk_src;

	return 0;
}

uint8_t tac5112_ch_en_from_masks(uint8_t input_channels, uint8_t output_channels)
{
	uint8_t ch_en = 0U;

	for (uint8_t ch = 0U; ch < TAC5112_MAX_CHANNELS; ch++) {
		if ((input_channels & BIT(ch)) != 0U) {
			/* Input channels occupy the upper nibble (CH1 = BIT(7)). */
			ch_en |= (uint8_t)BIT(7U - ch);
		}

		if ((output_channels & BIT(ch)) != 0U) {
			/* Output channels occupy the lower nibble (CH1 = BIT(3)). */
			ch_en |= (uint8_t)BIT(3U - ch);
		}
	}

	return ch_en;
}

int tac5112_channels_set(const struct device *dev, uint8_t input_channels, uint8_t output_channels)
{
	int ret;
	struct tac5112_data *data;
	const struct tac5112_config *cfg;
	uint8_t val;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	if (((input_channels | output_channels) & (uint8_t)~TAC5112_CHAN_MASK) != 0U) {
		LOG_ERR("Channel mask out of range (in=0x%02x out=0x%02x)", input_channels,
			output_channels);
		return -EINVAL;
	}

	data = dev->data;
	cfg = dev->config;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = tac5112_reg_write(dev, TAC5112_REG_CH_EN,
				tac5112_ch_en_from_masks(input_channels, output_channels));
	if (ret != 0) {
		LOG_DBG("Failed to write register ret: %d", ret);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (input_channels != 0U) {
		val = TAC5112_PWR_CFG_ADC_PDZ_BIT;

		/* Only microphones need the bias; line-level inputs need the ADC alone. */
		if (cfg->micbias_enable) {
			val |= TAC5112_PWR_CFG_MICBIAS_PDZ_BIT;
		}
	} else {
		val = 0U;
	}

	ret = tac5112_reg_update(
		dev, TAC5112_REG_PWR_CFG,
		(uint8_t)(TAC5112_PWR_CFG_ADC_PDZ_BIT | TAC5112_PWR_CFG_MICBIAS_PDZ_BIT), val);

	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief True while the codec's IRQ line is asserted.
 *
 * Reading the fault registers successfully does not guarantee the line has
 * released (a fault that latches while another one is still pending keeps it
 * asserted), and the interrupt is edge triggered, so a new fault would then
 * produce no edge.
 *
 * @param[in]  dev  Device handle.
 *
 * @retval true if IRQ asserted.
 * @retval false if IRQ is not asserted.
 */
static bool tac5112_irq_active(const struct device *dev)
{
	const struct tac5112_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->interrupt_gpio)) {
		LOG_WRN("GPIO is not ready");
		return false;
	}

	if (gpio_pin_get_dt(&cfg->interrupt_gpio) > 0) {
		return true;
	}

	return false;
}

/**
 * @brief Configure the codec.
 *
 * @param[in]  dev  Device handle.
 * @param[in]  cfg  Codec configuration parameters.
 *
 * @return 0 if successful, error otherwise.
 */
static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	int ret;
	uint8_t format;
	uint8_t wlen;
	uint8_t in_mask;
	uint8_t out_mask;
	struct tac5112_data *data;
	const struct tac5112_config *config;
	struct tac5112_pll_config pll = {
		.auto_config = true,
		.fractional_allowed = true,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
	};

	if (!tac5112_dev_valid(dev) || (cfg == NULL)) {
		return -EINVAL;
	}

	ret = tac5112_dai_config_valid(cfg, &format, &wlen);
	if (ret != 0) {
		LOG_ERR("Invalid DAI");
		return -EINVAL;
	}

	data = dev->data;
	config = dev->config;
	in_mask = config->input_channels;
	out_mask = config->output_channels;

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK:
		in_mask = 0U;
		break;
	case AUDIO_ROUTE_CAPTURE:
		out_mask = 0U;
		break;
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		break;
	case AUDIO_ROUTE_BYPASS:
		/* fall through */
	default:
		LOG_ERR("Unsupported dai_route %d", (int)cfg->dai_route);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = tac5112_reg_write(dev, TAC5112_REG_SW_RESET, TAC5112_SW_RESET_BIT);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	k_msleep(TAC5112_T_RESET_SETTLE_MS);

	data->configured = false;
	data->pll_custom_init = false;
	memset(&data->out_cache, 0, sizeof(data->out_cache));
	memset(&data->in_cache, 0, sizeof(data->in_cache));

	(void)k_work_cancel_delayable(&data->fault_work);
	data->fault_pending = 0U;
	data->fault_retries = 0U;

	ret = tac5112_reg_update(dev, TAC5112_REG_DEV_MISC_CFG, TAC5112_SLEEP_ENZ_BIT,
				 TAC5112_SLEEP_ENZ_BIT);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	k_msleep(TAC5112_T_SLEEP_EXIT_MIN_MS);

	ret = tac5112_dai_configure(dev, cfg);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (tac5112_is_asi_clk_controller(cfg)) {
		ret = tac5112_controller_configure(dev, cfg, &pll.clk_src);
		if (ret != 0) {
			k_mutex_unlock(&data->lock);
			return ret;
		}
	}

	/* Run the recommended automatic clock/PLL configuration. */
	ret = tac5112_pll_configure(dev, &pll);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = tac5112_channels_set(dev, in_mask, out_mask);
	if (ret != 0) {
		LOG_DBG("Failed to set channels: ret %d", ret);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = tac5112_reg_write(dev, TAC5112_REG_INT_CFG, TAC5112_INT_CFG_FAULT_IRQ);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = tac5112_reg_update(
		dev, TAC5112_REG_INT_MASK4,
		(uint8_t)(TAC5112_INT_MASK4_OUT_SC_BIT | TAC5112_INT_MASK4_DRVR_VG_BIT), 0U);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (tac5112_irq_active(dev)) {
		(void)k_work_reschedule(&data->fault_work, K_NO_WAIT);
	}

	data->configured = true;

	k_mutex_unlock(&data->lock);

	return 0;
}

/**
 * @brief Start the codec.
 *
 * @param[in]  dev  Device handle.
 * @param[in]  dir  Digital Audio Interface (DAI) direction bitmap.
 *
 * @return 0 if successful, error otherwise.
 */
static int codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	int ret;
	struct tac5112_data *data;
	uint8_t mask = 0;
	uint8_t val = 0;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	/* The directions are defined within the codec.h and all are non-zero. */
	if ((dir & AUDIO_DAI_DIR_TXRX) == 0) {
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

/**
 * @brief Stop the codec.
 *
 * @param[in]  dev  Device handle.
 * @param[in]  dir  Digital Audio Interface (DAI) direction bitmap.
 *
 * @return 0 if successful, error otherwise.
 */
static int codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	int ret;
	struct tac5112_data *data;
	uint8_t mask = 0;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	/* The directions are defined within the codec.h and all are non-zero. */
	if ((dir & AUDIO_DAI_DIR_TXRX) == 0) {
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

/**
 * @brief Start the codec output(s).
 *
 * @param[in]  dev  Device handle.
 */
static void codec_output_start(const struct device *dev)
{
	if (!tac5112_dev_valid(dev)) {
		return;
	}

	if (!((struct tac5112_data *)dev->data)->configured) {
		LOG_WRN("start_output() called before a successful configure()");
	}

	(void)codec_start(dev, AUDIO_DAI_DIR_TX);
}

/**
 * @brief Stop the codec output(s).
 *
 * @param[in]  dev  Device handle.
 */
static void codec_output_stop(const struct device *dev)
{
	if (!tac5112_dev_valid(dev)) {
		return;
	}

	(void)codec_stop(dev, AUDIO_DAI_DIR_TX);
}

/**
 * @brief Apply one property to one physical channel (0=left, 1=right).
 *
 * @param[in]  dev       Device handle.
 * @param[in]  property  Property to be applied.
 * @param[in]  idx       Physical channel index (0=left, 1=right).
 * @param[in]  val       Value of property to apply.
 *
 * @return 0 if successful, error otherwise.
 */
static int tac5112_property_apply(const struct device *dev, audio_property_t property, uint8_t idx,
				  audio_property_value_t val)
{
	int ret;
	struct tac5112_data *data = dev->data;
	struct tac5112_chan_cache *cache;
	struct tac5112_reg adc_reg;
	enum tac5112_path path;
	bool new_muted;
	int16_t new_vol;
	uint8_t dvol;

	if (idx >= TAC5112_NUM_CHAN) {
		return -EINVAL;
	}

	path = ((property == AUDIO_PROPERTY_OUTPUT_VOLUME) ||
		(property == AUDIO_PROPERTY_OUTPUT_MUTE))
		       ? TAC5112_PATH_OUTPUT
		       : TAC5112_PATH_INPUT;
	cache = (path == TAC5112_PATH_OUTPUT) ? &data->out_cache[idx] : &data->in_cache[idx];
	new_muted = cache->muted;
	new_vol = cache->vol_half_db;

	/* The cache is only committed once the hardware write(s) succeed, so a
	 * failed write never leaves the cache claiming a state the codec is not in.
	 */
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		/* fall through */
	case AUDIO_PROPERTY_INPUT_VOLUME:
		ret = tac5112_dvol_from_half_db(path, (int32_t)val.vol, &dvol);
		if (ret != 0) {
			LOG_ERR("Volume %d (0.5 dB units) out of range", val.vol);
			return ret;
		}

		new_vol = (int16_t)val.vol;
		if (cache->muted) {
			/* Mute is the value 0 in the volume register, so writing the new
			 * volume now would unmute the channel. While muted the new volume
			 * is only remembered (the cache is committed below) and the mute
			 * value is written again. Writing both registers on every update,
			 * instead of returning early from the cache, means a successful
			 * call always leaves the codec in step with the cache, even after
			 * an earlier update failed part way.
			 */
			dvol = TAC5112_DVOL_MUTE;
		}

		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		/* fall through */
	case AUDIO_PROPERTY_INPUT_MUTE:
		if (val.mute) {
			dvol = TAC5112_DVOL_MUTE;
		} else {
			ret = tac5112_dvol_from_half_db(path, cache->vol_half_db, &dvol);
			if (ret != 0) {
				return ret;
			}
		}

		new_muted = val.mute;
		break;
	default:
		return -ENOTSUP;
	}

	if (path == TAC5112_PATH_OUTPUT) {
		struct tac5112_reg reg_a =
			(idx == 0U) ? TAC5112_REG_DAC_CH1A_CFG0 : TAC5112_REG_DAC_CH2A_CFG0;
		struct tac5112_reg reg_b =
			(idx == 0U) ? TAC5112_REG_DAC_CH1B_CFG0 : TAC5112_REG_DAC_CH2B_CFG0;

		ret = tac5112_reg_write(dev, reg_a, dvol);
		if (ret != 0) {
			return ret;
		}

		ret = tac5112_reg_write(dev, reg_b, dvol);
		if (ret != 0) {
			/* A was written and B was not, and A is not written back. The
			 * cache keeps its last good state, and the next successful
			 * update writes both registers again.
			 */
			return ret;
		}
	} else {
		adc_reg = (idx == 0U) ? TAC5112_REG_ADC_CH1_CFG2 : TAC5112_REG_ADC_CH2_CFG2;

		ret = tac5112_reg_write(dev, adc_reg, dvol);
		if (ret != 0) {
			return ret;
		}
	}

	cache->muted = new_muted;
	cache->vol_half_db = new_vol;

	return 0;
}

/**
 * @brief Codec set a property.
 *
 * @param[in]  dev       Device handle.
 * @param[in]  property  Property to be applied.
 * @param[in]  channel   Channel the property applies to.
 * @param[in]  val       Value of property to apply.
 *
 * @return 0 if successful, error otherwise.
 */
static int codec_property_set(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	int ret = 0;
	struct tac5112_data *data;
	bool do_left;
	bool do_right;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	if ((property != AUDIO_PROPERTY_OUTPUT_VOLUME) &&
	    (property != AUDIO_PROPERTY_OUTPUT_MUTE) && (property != AUDIO_PROPERTY_INPUT_VOLUME) &&
	    (property != AUDIO_PROPERTY_INPUT_MUTE)) {
		LOG_ERR("Property %d not supported", (int)property);
		return -ENOTSUP;
	}

	do_left = (channel == AUDIO_CHANNEL_ALL) || (channel == AUDIO_CHANNEL_FRONT_LEFT);
	do_right = (channel == AUDIO_CHANNEL_ALL) || (channel == AUDIO_CHANNEL_FRONT_RIGHT);

	if (!do_left && !do_right) {
		LOG_ERR("Unsupported channel %d", (int)channel);
		return -ENOTSUP;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (do_left) {
		ret = tac5112_property_apply(dev, property, 0U, val);
		if (ret != 0) {
			k_mutex_unlock(&data->lock);
			return ret;
		}
	}

	if (do_right) {
		ret = tac5112_property_apply(dev, property, 1U, val);
		if (ret != 0) {
			k_mutex_unlock(&data->lock);
			return ret;
		}
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

int tac5112_volume_get(const struct device *dev, enum tac5112_path path, audio_channel_t channel,
		       int16_t *half_db, bool *muted)
{
	struct tac5112_data *data;
	const struct tac5112_chan_cache *cache;
	uint8_t idx;

	if (!tac5112_dev_valid(dev) || ((half_db == NULL) && (muted == NULL))) {
		return -EINVAL;
	}

	if ((path != TAC5112_PATH_OUTPUT) && (path != TAC5112_PATH_INPUT)) {
		return -EINVAL;
	}

	if (channel == AUDIO_CHANNEL_FRONT_LEFT) {
		idx = 0U;
	} else if (channel == AUDIO_CHANNEL_FRONT_RIGHT) {
		idx = 1U;
	} else {
		return -ENOTSUP;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	cache = (path == TAC5112_PATH_OUTPUT) ? &data->out_cache[idx] : &data->in_cache[idx];

	if (half_db != NULL) {
		*half_db = cache->vol_half_db;
	}

	if (muted != NULL) {
		*muted = cache->muted;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

/**
 * @brief Codec apply properties callback.
 *
 * @note codec_property_set() writes to hardware synchronously
 *
 * @param[in]   dev  Device handle.
 *
 * @return 0 successful.
 */
static int codec_properties_apply(const struct device *dev)
{
	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Read the codec's fault bits.
 *
 * @param[in]   dev     Device handle.
 * @param[out]  errors  Faults.
 *
 * @return 0 if successful, error otherwise.
 */
static int tac5112_fault_bits_read(const struct device *dev, uint32_t *errors)
{
	int ret;
	static const struct tac5112_reg out_ltch[] = {TAC5112_REG_OUT_CH1_LTCH,
						      TAC5112_REG_OUT_CH2_LTCH};
	uint8_t faults_reg;

	for (int i = 0; i < ARRAY_SIZE(out_ltch); i++) {
		ret = tac5112_reg_read(dev, out_ltch[i], &faults_reg);
		if (ret != 0) {
			return ret;
		}

		if (faults_reg & (TAC5112_OUT_CH_SC_OUTP_BIT | TAC5112_OUT_CH_SC_OUTM_BIT)) {
			*errors |= AUDIO_CODEC_ERROR_OVERCURRENT;
		}

		if (faults_reg & (TAC5112_OUT_CH_VG_FAULT_P_BIT | TAC5112_OUT_CH_VG_FAULT_M_BIT)) {
			*errors |= AUDIO_CODEC_ERROR_DC;
		}

		if (faults_reg &
		    (TAC5112_OUT_CH_ADC_CH1_OVRLD_BIT | TAC5112_OUT_CH_ADC_CH2_OVRLD_BIT)) {
			*errors |= AUDIO_CODEC_ERROR_OVERVOLTAGE;
		}
	}

	return 0;
}

/**
 * @brief Clear the codec's fault bits callback.
 *
 * @param[in]   dev     Device handle.
 *
 * @return 0 if successful, error otherwise.
 */
static int codec_errors_clear(const struct device *dev)
{
	int ret;
	struct tac5112_data *data;
	uint8_t tmp;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = tac5112_reg_read(dev, TAC5112_REG_INT_LTCH0, &tmp);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = tac5112_reg_read(dev, TAC5112_REG_OUT_CH1_LTCH, &tmp);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = tac5112_reg_read(dev, TAC5112_REG_OUT_CH2_LTCH, &tmp);
	if (ret != 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	(void)k_work_cancel_delayable(&data->fault_work);
	data->fault_pending = 0U;
	data->fault_retries = 0U;

	if (tac5112_irq_active(dev)) {
		(void)k_work_reschedule(&data->fault_work, K_MSEC(TAC5112_FAULT_POLL_MS));
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

/**
 * @brief Codec's fault handler.
 *
 * @param[in]   work  Handler's work.
 */
static void tac5112_fault_work_handler(struct k_work *work)
{
	int ret;
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tac5112_data *data = CONTAINER_OF(dwork, struct tac5112_data, fault_work);
	const struct device *dev = data->self;
	audio_codec_error_callback_t cb;
	uint32_t errors;

	if (dev == NULL) {
		return;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = tac5112_fault_bits_read(dev, &data->fault_pending);
	if ((ret != 0) && (data->fault_retries < TAC5112_FAULT_RETRY_MAX)) {
		data->fault_retries++;
		LOG_WRN("Fault read failed (%d), retry %u/%u", ret, data->fault_retries,
			TAC5112_FAULT_RETRY_MAX);
		(void)k_work_reschedule(&data->fault_work, K_MSEC(data->fault_retry_ms));
		k_mutex_unlock(&data->lock);
		return;
	}

	if (ret == 0) {
		data->fault_retries = 0U;

		if (tac5112_irq_active(dev)) {
			(void)k_work_reschedule(&data->fault_work, K_MSEC(TAC5112_FAULT_POLL_MS));
		}
	} else {
		LOG_WRN("Fault read still failing (%d); polling every %u ms", ret,
			TAC5112_FAULT_SLOW_RETRY_MS);
		(void)k_work_reschedule(&data->fault_work, K_MSEC(TAC5112_FAULT_SLOW_RETRY_MS));
	}

	errors = data->fault_pending;
	data->fault_pending = 0U;
	cb = data->error_cb;
	k_mutex_unlock(&data->lock);

	if ((errors != 0U) && (cb != NULL)) {
		cb(dev, errors);
	}
}

/**
 * @brief Codec's fault interrupt handler.
 *
 * Runs in interrupt context and only schedules the fault worker.
 *
 * @param[in]  port  GPIO port that raised the interrupt (unused).
 * @param[in]  cb    GPIO callback structure embedded in the driver data.
 * @param[in]  pins  Bitmask of the pins that triggered (unused).
 */
static void tac5112_fault_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct tac5112_data *data;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	if (cb == NULL) {
		return;
	}

	data = CONTAINER_OF(cb, struct tac5112_data, interrupt_gpio_cb);

	(void)k_work_reschedule(&data->fault_work, K_NO_WAIT);
}

/**
 * @brief Register the callback that receives asynchronous codec faults.
 *
 * Faults are delivered from the codec's interrupt line, so a callback can only
 * be registered if the devicetree node has @c interrupt-gpios. Passing NULL
 * unregisters the callback and is always allowed.
 *
 * @param[in]   dev  Device handle.
 * @param[in]   cb   Callback to invoke on a fault, or NULL to unregister.
 *
 * @return 0 if successful, error otherwise.
 */
static int codec_error_callback_register(const struct device *dev, audio_codec_error_callback_t cb)
{
	struct tac5112_data *data;
	const struct tac5112_config *cfg;

	if (!tac5112_dev_valid(dev)) {
		return -EINVAL;
	}

	data = dev->data;
	cfg = dev->config;

	if ((cb != NULL) && !gpio_is_ready_dt(&cfg->interrupt_gpio)) {
		LOG_ERR("No interrupt-gpios in devicetree; cannot report asynchronous faults");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->error_cb = cb;
	k_mutex_unlock(&data->lock);

	return 0;
}

/**
 * @brief Initialise the codec.
 *
 * @param[in]   dev  Device handle.
 *
 * @return 0 if successful, error otherwise.
 */
static int tac5112_init(const struct device *dev)
{
	int ret;
	struct tac5112_data *data;
	const struct tac5112_config *cfg;

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
	k_work_init_delayable(&data->fault_work, tac5112_fault_work_handler);
	data->fault_pending = 0U;
	data->fault_retries = 0U;
	data->fault_retry_ms = TAC5112_FAULT_RETRY_MS;
	data->self = dev;
	data->configured = false;
	data->error_cb = NULL;
	memset(&data->out_cache, 0, sizeof(data->out_cache));
	memset(&data->in_cache, 0, sizeof(data->in_cache));

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("Reset GPIO controller not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	if (cfg->interrupt_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->interrupt_gpio)) {
			LOG_ERR("Fault GPIO controller not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->interrupt_gpio, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("Failed to configure fault GPIO (%d)", ret);
			return ret;
		}

		gpio_init_callback(&data->interrupt_gpio_cb, tac5112_fault_isr,
				   BIT(cfg->interrupt_gpio.pin));
		ret = gpio_add_callback_dt(&cfg->interrupt_gpio, &data->interrupt_gpio_cb);
		if (ret != 0) {
			LOG_ERR("Failed to add fault GPIO callback (%d)", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to configure fault GPIO interrupt (%d)", ret);
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(audio_codec, tac5112_driver_api) = {
	.configure = codec_configure,
	.start_output = codec_output_start,
	.stop_output = codec_output_stop,
	.set_property = codec_property_set,
	.apply_properties = codec_properties_apply,
	.clear_errors = codec_errors_clear,
	.register_error_callback = codec_error_callback_register,
	.start = codec_start,
	.stop = codec_stop,
};

#define TAC5112_INIT(inst)                                                                         \
	static struct tac5112_data tac5112_data_##inst;                                            \
	static const struct tac5112_config tac5112_config_##inst = {                               \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, {0}),            \
		.input_channels = DT_INST_PROP(inst, input_channels),                              \
		.output_channels = DT_INST_PROP(inst, output_channels),                            \
		.micbias_enable = DT_INST_PROP(inst, micbias_enable),                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tac5112_init, NULL, &tac5112_data_##inst,                      \
			      &tac5112_config_##inst, POST_KERNEL,                                 \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &tac5112_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TAC5112_INIT)
