/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * ztest suite for the TAC5112 audio codec driver. Runs on native_sim
 * against the I2C emulator in drivers/audio/tac5112_emul.c (see
 * boards/native_sim.overlay and prj.conf wiring), so the *real* driver
 * code and *real* I2C stack are exercised end to end.
 *
 * A note on testing NULL `dev` pointers: the convenience wrappers in
 * <zephyr/audio/codec.h> (audio_codec_configure(), audio_codec_set_property(),
 * ...) fetch the driver API table via `dev->api` *before* calling into the
 * driver, so passing dev == NULL to those wrappers directly is a crash, not
 * a testable error path -- by design, matching every other Zephyr device
 * subsystem. To still exercise this driver's own NULL-`dev` guards, the
 * tests below fetch `fixture->dev->api` once (a valid, non-NULL dev) and
 * then invoke the driver's callback function pointers directly, which is
 * safe because no further NULL dereference happens before the callback's
 * own tac5112_dev_valid() check runs.
 */

#include <errno.h>

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>

#include "tac5112.h"

struct tac5112_fixture {
	const struct device *dev;
};

static struct audio_codec_cfg valid_codec_cfg(void)
{
	struct audio_codec_cfg cfg = {0};

	cfg.mclk_freq = 0U; /* not used in ASI-target mode */
	cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	cfg.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE;
	cfg.dai_cfg.i2s.word_size = AUDIO_PCM_WIDTH_16_BITS;
	cfg.dai_cfg.i2s.channels = 2U;
	cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	cfg.dai_cfg.i2s.frame_clk_freq = 48000U;

	return cfg;
}

static void configure_dut(const struct device *dev)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	zassert_ok(audio_codec_configure(dev, &cfg), "baseline configure() failed");
}

static void *tac5112_suite_setup(void)
{
	static struct tac5112_fixture fixture;

	fixture.dev = DEVICE_DT_GET(DT_ALIAS(tac5112_0));
	zassert_true(device_is_ready(fixture.dev), "TAC5112 emulated device is not ready");

	return &fixture;
}

ZTEST_SUITE(tac5112, NULL, tac5112_suite_setup, NULL, NULL, NULL);

/* ---------------------------------------------------------------------
 * configure()
 * ---------------------------------------------------------------------
 */

ZTEST_F(tac5112, test_configure_accepts_valid_i2s_target_config)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();
	uint8_t val;

	zassert_ok(audio_codec_configure(fixture->dev, &cfg));

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PASI_CFG0, &val));
	zassert_equal((val & TAC5112_PASI_FORMAT_MASK) >> TAC5112_PASI_FORMAT_SHIFT,
		      TAC5112_PASI_FORMAT_I2S, "I2S format not programmed, got 0x%02x", val);
	zassert_equal((val & TAC5112_PASI_WLEN_MASK) >> TAC5112_PASI_WLEN_SHIFT,
		      TAC5112_PASI_WLEN_16, "16-bit word length not programmed, got 0x%02x", val);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &val));
	zassert_equal(val, TAC5112_CH_EN_IN_CH1_BIT | TAC5112_CH_EN_IN_CH2_BIT |
				   TAC5112_CH_EN_OUT_CH1_BIT | TAC5112_CH_EN_OUT_CH2_BIT);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PWR_CFG, &val));
	zassert_equal(val, TAC5112_PWR_CFG_ADC_PDZ_BIT | TAC5112_PWR_CFG_MICBIAS_PDZ_BIT);
}

ZTEST_F(tac5112, test_configure_rejects_null_cfg)
{
	const struct audio_codec_api *api = fixture->dev->api;

	/* audio_codec_configure(dev, NULL) is safe to call through the
	 * wrapper: dev is non-NULL, so only this driver's own cfg == NULL
	 * check is exercised.
	 */
	zassert_equal(audio_codec_configure(fixture->dev, NULL), -EINVAL);

	/* Directly exercise the dev == NULL branch of codec_configure() by
	 * calling the API table entry without going through the wrapper
	 * (see file header comment for why the wrapper itself can't be used
	 * with dev == NULL).
	 */
	struct audio_codec_cfg cfg = valid_codec_cfg();

	zassert_equal(api->configure(NULL, &cfg), -EINVAL);
}

ZTEST_F(tac5112, test_configure_rejects_wrong_channel_count)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	cfg.dai_cfg.i2s.channels = 1U;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	cfg.dai_cfg.i2s.channels = 4U;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);
}

ZTEST_F(tac5112, test_configure_rejects_out_of_range_sample_rate)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	cfg.dai_cfg.i2s.frame_clk_freq = 1000U; /* below the 4 kHz floor */
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	cfg.dai_cfg.i2s.frame_clk_freq = 900000U; /* above the 768 kHz ceiling */
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);
}

ZTEST_F(tac5112, test_configure_rejects_bus_controller_mode)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	/* Clearing the TARGET bits requests that the codec act as bus
	 * controller for BCLK/FSYNC, which this driver deliberately does
	 * not implement (see tac5112_configure_dai()).
	 */
	cfg.dai_cfg.i2s.options = 0U;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -ENOTSUP);
}

ZTEST_F(tac5112, test_configure_rejects_unsupported_word_size)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	cfg.dai_cfg.i2s.word_size = 0U;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);
}

/* ---------------------------------------------------------------------
 * start_output() / stop_output() / start() / stop()
 * ---------------------------------------------------------------------
 */

ZTEST_F(tac5112, test_start_stop_output_toggles_dac_pdz_only)
{
	uint8_t before;
	uint8_t after;

	configure_dut(fixture->dev);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PWR_CFG, &before));
	zassert_equal(before & TAC5112_PWR_CFG_DAC_PDZ_BIT, 0, "DAC should start powered down");

	audio_codec_start_output(fixture->dev);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PWR_CFG, &after));
	zassert_equal(after, before | TAC5112_PWR_CFG_DAC_PDZ_BIT);

	audio_codec_stop_output(fixture->dev);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PWR_CFG, &after));
	zassert_equal(after, before);
}

ZTEST_F(tac5112, test_start_stop_direction_bitmap_validation)
{
	configure_dut(fixture->dev);

	zassert_ok(audio_codec_start(fixture->dev, AUDIO_DAI_DIR_RX));
	zassert_ok(audio_codec_stop(fixture->dev, AUDIO_DAI_DIR_TXRX));

	/* 0 and any bit above AUDIO_DAI_DIR_TXRX (0x3) must be rejected. */
	zassert_equal(audio_codec_start(fixture->dev, 0U), -EINVAL);
	zassert_equal(audio_codec_start(fixture->dev, 0x04U), -EINVAL);
	zassert_equal(audio_codec_stop(fixture->dev, 0x80U), -EINVAL);
}

/* ---------------------------------------------------------------------
 * set_property(): volume / mute
 * ---------------------------------------------------------------------
 */

ZTEST_F(tac5112, test_output_volume_bounds_checking)
{
	audio_property_value_t val;
	uint8_t before;
	uint8_t after;

	configure_dut(fixture->dev);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1A_CFG0, &before));

	/* One past the valid range on each side must be rejected, and must
	 * not have modified hardware state.
	 */
	val.vol = TAC5112_DAC_VOL_MIN_HALF_DB - 1;
	zassert_equal(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_FRONT_LEFT, val),
		      -EINVAL);
	val.vol = TAC5112_DAC_VOL_MAX_HALF_DB + 1;
	zassert_equal(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_FRONT_LEFT, val),
		      -EINVAL);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1A_CFG0, &after));
	zassert_equal(before, after, "rejected volume write must not touch hardware");

	/* 0 dB (val.vol == 0) must land exactly on the documented 0 dB code. */
	val.vol = 0;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_FRONT_LEFT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1A_CFG0, &after));
	zassert_equal(after, TAC5112_DAC_DVOL_0DB);
	/* Left channel uses two DAC cores (1A/1B) for differential drive;
	 * both must be written identically.
	 */
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1B_CFG0, &after));
	zassert_equal(after, TAC5112_DAC_DVOL_0DB);

	/* The extreme ends of the range must be accepted. */
	val.vol = (int)TAC5112_DAC_VOL_MIN_HALF_DB;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_FRONT_LEFT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1A_CFG0, &after));
	zassert_equal(after, TAC5112_DAC_DVOL_MIN);

	val.vol = (int)TAC5112_DAC_VOL_MAX_HALF_DB;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_FRONT_LEFT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1A_CFG0, &after));
	zassert_equal(after, TAC5112_DAC_DVOL_MAX);
}

ZTEST_F(tac5112, test_mute_then_unmute_restores_previous_volume)
{
	audio_property_value_t val;
	uint8_t reg;

	configure_dut(fixture->dev);

	val.vol = 10; /* +5.0 dB */
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DAC_DVOL_0DB + 10);

	val.mute = true;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_MUTE,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DVOL_MUTE);

	val.mute = false;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_MUTE,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DAC_DVOL_0DB + 10, "unmute must restore the cached volume");
}

ZTEST_F(tac5112, test_input_volume_uses_adc_range)
{
	audio_property_value_t val;
	uint8_t reg;

	configure_dut(fixture->dev);

	val.vol = (int)TAC5112_ADC_VOL_MAX_HALF_DB + 1;
	zassert_equal(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_INPUT_VOLUME,
					       AUDIO_CHANNEL_ALL, val),
		      -EINVAL);

	val.vol = 0;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_INPUT_VOLUME,
					    AUDIO_CHANNEL_ALL, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_ADC_CH1_CFG2, &reg));
	zassert_equal(reg, TAC5112_ADC_DVOL_0DB);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_ADC_CH2_CFG2, &reg));
	zassert_equal(reg, TAC5112_ADC_DVOL_0DB);
}

ZTEST_F(tac5112, test_set_property_rejects_unsupported_channel_and_property)
{
	audio_property_value_t val = {.vol = 0};

	configure_dut(fixture->dev);

	zassert_equal(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_LFE, val),
		      -ENOTSUP);
	zassert_equal(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       (audio_channel_t)(AUDIO_CHANNEL_PRIV_START + 5),
					       val),
		      -ENOTSUP);
	zassert_equal(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_EQ_GAIN,
					       AUDIO_CHANNEL_ALL, val),
		      -ENOTSUP);
}

ZTEST_F(tac5112, test_set_property_rejects_null_dev)
{
	const struct audio_codec_api *api = fixture->dev->api;
	audio_property_value_t val = {.vol = 0};

	zassert_equal(api->set_property(NULL, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL, val),
		      -EINVAL);
}

ZTEST_F(tac5112, test_apply_properties_is_a_no_op_success)
{
	zassert_ok(audio_codec_apply_properties(fixture->dev));
}

/* ---------------------------------------------------------------------
 * PLL control
 * ---------------------------------------------------------------------
 */

ZTEST_F(tac5112, test_pll_auto_mode_programs_clk_src_and_clears_custom_bit)
{
	struct tac5112_pll_config pll = {
		.auto_config = true,
		.fractional_allowed = false,
		.clk_src = TAC5112_PLL_CLK_SRC_SASI_BCLK,
	};
	uint8_t val;

	configure_dut(fixture->dev);
	zassert_ok(tac5112_configure_pll(fixture->dev, &pll));

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG2, &val));
	zassert_equal((val & TAC5112_CLK_SRC_SEL_MASK) >> TAC5112_CLK_SRC_SEL_SHIFT,
		      TAC5112_PLL_CLK_SRC_SASI_BCLK);
	zassert_equal(val & TAC5112_AUTO_PLL_FR_ALLOW_BIT, 0);
	zassert_equal(val & TAC5112_PLL_DIS_BIT, 0, "auto mode must leave the PLL enabled");

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG0, &val));
	zassert_equal(val & TAC5112_CUSTOM_CLK_CFG_BIT, 0);
}

ZTEST_F(tac5112, test_pll_custom_mode_writes_all_dividers)
{
	struct tac5112_pll_config pll = {
		.auto_config = false,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
		.pdiv = 2U,
		.jmul = 384U, /* > 255, exercises the JMUL MSB bit */
		.dmul = 5000U,
		.ndiv = 3U,
		.mdiv = 15U,
		.pdm_div_sel = 2U,
		.adc_modclk_div_sel = 1U,
	};
	uint8_t val;

	configure_dut(fixture->dev);
	zassert_ok(tac5112_configure_pll(fixture->dev, &pll));

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG0, &val));
	zassert_equal(val & TAC5112_CUSTOM_CLK_CFG_BIT, TAC5112_CUSTOM_CLK_CFG_BIT);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG15, &val));
	zassert_equal(val, 2U);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG18, &val));
	zassert_equal(val, (uint8_t)(384U & 0xFFU));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG16, &val));
	zassert_equal(val & TAC5112_PLL_JMUL_MSB_BIT, TAC5112_PLL_JMUL_MSB_BIT,
		      "JMUL=384 requires the MSB bit set");
	zassert_equal(val & TAC5112_PLL_DMUL_MSB_MASK, (5000U >> 8) & TAC5112_PLL_DMUL_MSB_MASK);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG17, &val));
	zassert_equal(val, (uint8_t)(5000U & 0xFFU));

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG19, &val));
	zassert_equal(val, (uint8_t)((3U << TAC5112_NDIV_SHIFT) | (2U << TAC5112_PDM_DIV_SHIFT)));

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG20, &val));
	zassert_equal(val, (uint8_t)((15U << TAC5112_MDIV_SHIFT) | 1U));
}

ZTEST_F(tac5112, test_pll_rejects_out_of_range_fields_without_side_effects)
{
	struct tac5112_pll_config base = {
		.auto_config = false,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
		.pdiv = 1U,
		.jmul = 8U,
		.dmul = 0U,
		.ndiv = 1U,
		.mdiv = 1U,
	};
	struct tac5112_pll_config bad;
	uint8_t before;
	uint8_t after;

	configure_dut(fixture->dev);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG15, &before));

	bad = base;
	bad.jmul = 0U; /* below TAC5112_PLL_JMUL_MIN */
	zassert_equal(tac5112_configure_pll(fixture->dev, &bad), -EINVAL);

	bad = base;
	bad.jmul = 512U; /* above TAC5112_PLL_JMUL_MAX */
	zassert_equal(tac5112_configure_pll(fixture->dev, &bad), -EINVAL);

	bad = base;
	bad.dmul = 10000U; /* above TAC5112_PLL_DMUL_MAX */
	zassert_equal(tac5112_configure_pll(fixture->dev, &bad), -EINVAL);

	bad = base;
	bad.ndiv = 8U; /* above TAC5112_PLL_NDIV_MAX (3-bit field) */
	zassert_equal(tac5112_configure_pll(fixture->dev, &bad), -EINVAL);

	bad = base;
	bad.mdiv = 64U; /* above TAC5112_PLL_MDIV_MAX (6-bit field) */
	zassert_equal(tac5112_configure_pll(fixture->dev, &bad), -EINVAL);

	bad = base;
	bad.pdm_div_sel = 5U; /* above TAC5112_PLL_PDM_DIV_SEL_MAX */
	zassert_equal(tac5112_configure_pll(fixture->dev, &bad), -EINVAL);

	bad = base;
	bad.adc_modclk_div_sel = 3U; /* above TAC5112_ADC_MODCLK_DIV_SEL_MAX */
	zassert_equal(tac5112_configure_pll(fixture->dev, &bad), -EINVAL);

	bad = base;
	bad.clk_src = (enum tac5112_pll_clk_src)6; /* above TAC5112_CLK_SRC_SEL_MAX */
	zassert_equal(tac5112_configure_pll(fixture->dev, &bad), -EINVAL);

	/* None of the rejected calls above may have written CLK_CFG15
	 * (PDIV) or any other divider register: rejection happens before
	 * the first I2C write.
	 */
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG15, &after));
	zassert_equal(before, after);
}

ZTEST_F(tac5112, test_pll_rejects_null_pointers)
{
	struct tac5112_pll_config pll = {.auto_config = true};

	zassert_equal(tac5112_configure_pll(NULL, &pll), -EINVAL);
	zassert_equal(tac5112_configure_pll(fixture->dev, NULL), -EINVAL);
}

/* ---------------------------------------------------------------------
 * Error handling
 * ---------------------------------------------------------------------
 */

ZTEST_F(tac5112, test_clear_errors_and_register_callback)
{
	configure_dut(fixture->dev);

	zassert_ok(audio_codec_clear_errors(fixture->dev));

	/* NULL is a legal "unregister" callback value. */
	zassert_ok(audio_codec_register_error_callback(fixture->dev, NULL));
}

ZTEST_F(tac5112, test_configure_enables_fault_interrupts)
{
	uint8_t val;

	configure_dut(fixture->dev);

	/* INT_CFG programmed to active-low, assert-on-latched-event. */
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_INT_CFG, &val));
	zassert_equal(val, TAC5112_INT_CFG_FAULT_IRQ, "INT_CFG not set for fault IRQ");

	/* Output short-circuit and virtual-ground faults unmasked (bit clear). */
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_INT_MASK4, &val));
	zassert_equal(val & (TAC5112_INT_MASK4_OUT_SC_BIT | TAC5112_INT_MASK4_DRVR_VG_BIT), 0,
		      "OUT SC / virtual-ground fault interrupts must be unmasked");

	/* Clock-error / PLL-lock (INT_MASK0) are intentionally left masked. */
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_INT_MASK0, &val));
	zassert_equal(val & TAC5112_INT_MASK0_CLK_ERR_BIT, TAC5112_INT_MASK0_CLK_ERR_BIT,
		      "clock-error interrupt should remain masked for now");
}

/* ---------------------------------------------------------------------
 * Low-level I2C helpers: pointer validation and bounds checking
 * ---------------------------------------------------------------------
 */

ZTEST_F(tac5112, test_reg_helpers_reject_null_pointers)
{
	uint8_t val;

	zassert_equal(tac5112_reg_write(NULL, TAC5112_REG_SW_RESET, 0), -EINVAL);
	zassert_equal(tac5112_reg_read(NULL, TAC5112_REG_SW_RESET, &val), -EINVAL);
	zassert_equal(tac5112_reg_read(fixture->dev, TAC5112_REG_SW_RESET, NULL), -EINVAL);
}

ZTEST_F(tac5112, test_reg_update_preserves_untouched_bits)
{
	uint8_t before;
	uint8_t after;

	configure_dut(fixture->dev);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG2, &before));
	zassert_ok(tac5112_reg_update(fixture->dev, TAC5112_REG_CLK_CFG2,
				      TAC5112_AUTO_PLL_FR_ALLOW_BIT, 0));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG2, &after));

	zassert_equal(after, (uint8_t)(before & (uint8_t)~TAC5112_AUTO_PLL_FR_ALLOW_BIT));
}

ZTEST_F(tac5112, test_page_select_cache_survives_cross_page_access)
{
	uint8_t page0_val;
	uint8_t page1_val;
	uint8_t page3_val;

	configure_dut(fixture->dev);

	/* Interleave reads/writes across pages 0, 1 and 3 and confirm each
	 * still lands on the correct page-scoped register: catches page
	 * cache invalidation bugs.
	 */
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &page0_val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_INT_MASK0, &page1_val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG15, &page3_val));
	zassert_equal(page1_val, 0xFFU, "INT_MASK0 reset default");
	zassert_equal(page3_val, 0x01U, "CLK_CFG15 (PDIV) reset default");

	zassert_ok(tac5112_reg_write(fixture->dev, TAC5112_REG_CLK_CFG15, 7U));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &page0_val));
	zassert_equal(page0_val,
		      TAC5112_CH_EN_IN_CH1_BIT | TAC5112_CH_EN_IN_CH2_BIT |
			      TAC5112_CH_EN_OUT_CH1_BIT | TAC5112_CH_EN_OUT_CH2_BIT,
		      "page 0 register must be unaffected by a page 3 write");

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG15, &page3_val));
	zassert_equal(page3_val, 7U);
}

/* ---------------------------------------------------------------------
 * tac5112_dvol_from_half_db(): whitebox unit tests, no I2C involved
 * ---------------------------------------------------------------------
 */

ZTEST(tac5112, test_dvol_conversion_dac_range)
{
	uint8_t dvol;

	zassert_ok(tac5112_dvol_from_half_db(true, TAC5112_DAC_VOL_MIN_HALF_DB, &dvol));
	zassert_equal(dvol, TAC5112_DAC_DVOL_MIN);

	zassert_ok(tac5112_dvol_from_half_db(true, 0, &dvol));
	zassert_equal(dvol, TAC5112_DAC_DVOL_0DB);

	zassert_ok(tac5112_dvol_from_half_db(true, TAC5112_DAC_VOL_MAX_HALF_DB, &dvol));
	zassert_equal(dvol, TAC5112_DAC_DVOL_MAX);

	zassert_equal(tac5112_dvol_from_half_db(true, TAC5112_DAC_VOL_MIN_HALF_DB - 1, &dvol),
		      -EINVAL);
	zassert_equal(tac5112_dvol_from_half_db(true, TAC5112_DAC_VOL_MAX_HALF_DB + 1, &dvol),
		      -EINVAL);
}

ZTEST(tac5112, test_dvol_conversion_adc_range)
{
	uint8_t dvol;

	zassert_ok(tac5112_dvol_from_half_db(false, TAC5112_ADC_VOL_MIN_HALF_DB, &dvol));
	zassert_equal(dvol, TAC5112_ADC_DVOL_MIN);

	zassert_ok(tac5112_dvol_from_half_db(false, TAC5112_ADC_VOL_MAX_HALF_DB, &dvol));
	zassert_equal(dvol, TAC5112_ADC_DVOL_MAX);

	zassert_equal(tac5112_dvol_from_half_db(false, TAC5112_ADC_VOL_MAX_HALF_DB + 1, &dvol),
		      -EINVAL);
}

ZTEST(tac5112, test_dvol_conversion_rejects_null_output)
{
	zassert_equal(tac5112_dvol_from_half_db(true, 0, NULL), -EINVAL);
}

/* A huge out-of-range value must be rejected on its full width, not
 * silently wrapped/truncated by an internal narrowing cast before the
 * range check runs (this is the specific bug class strict bounds
 * checking is meant to prevent).
 */
ZTEST(tac5112, test_dvol_conversion_does_not_truncate_before_bounds_check)
{
	uint8_t dvol = 0xAAU;

	zassert_equal(tac5112_dvol_from_half_db(true, 65536 + 10, &dvol), -EINVAL);
	zassert_equal(dvol, 0xAAU, "rejected conversion must not touch the output parameter");
}
