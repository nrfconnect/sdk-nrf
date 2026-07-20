/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * ztest suite for the TAC5112 audio codec driver. Runs on native_sim
 * against the I2C emulator in drivers/audio/tac5112_emul.c (see
 * boards/native_sim.overlay and prj.conf wiring), so the *real* driver
 * code and *real* I2C stack are exercised end to end.
 *
 * A note on testing NULL `dev` pointers: the convenience wrappers in
 * <zephyr/audio/codec.h> (audio_codec_configure(), audio_codec_set_property(),
 * ...) fetch the driver API table (DEVICE_API_GET()) *before* calling into
 * the driver, so passing dev == NULL to those wrappers directly is a crash,
 * not a testable error path -- by design, matching every other Zephyr device
 * subsystem. To still exercise this driver's own NULL-`dev` guards, the
 * tests below fetch the API table for a valid (non-NULL) dev once and
 * then invoke the driver's callback function pointers directly, which is
 * safe because no further NULL dereference happens before the callback's
 * own tac5112_dev_valid() check runs.
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <drivers/tac5112.h>

#include "tac5112_private.h"
#include "tac5112_emul.h"

#define TEST_TA5112_CHANNELS_1 (1)
#define TEST_TA5112_CHANNELS_2 (2)
#define TEST_TA5112_CHANNELS_3 (3)
#define TEST_TA5112_CHANNELS_4 (4)

#define TEST_TAC5112_DEFAULT_CH_MASK (0x3U)

#define TEST_TAC5112_ROUTE_UNKNOWN (99)

#define TEST_TAC5112_CUSTOM_PLL (0x1388U)

#define TEST_TA5112_SAMPLE_FREQ_HZ (48000)

/* Just outside the driver's supported sample-rate range on each side, derived
 * from the driver limits so they cannot drift out of sync with it.
 */
#define TEST_TA5112_FRAME_CLK_HZ_FLOOR_BELOW   (TAC5112_FS_MIN_HZ - 1U)
#define TEST_TA5112_FRAME_CLK_HZ_CEILING_ABOVE (TAC5112_FS_MAX_HZ + 1U)

#define TEST_TAC5112_REG_CLK_CFG15 (0x07U)

#define TEST_TAC5112_DVOL		(0xAAU)
#define TEST_TAC5112_DVOL_CEILING_ABOVE (TAC5112_DAC_VOL_HALF_DB_MAX + 10)

#define TEST_TAC5112_DVOL_DB (10) /* +5.0 dB */

#define TEST_AUDIO_PROPERTY_NOT_SUPPORTED (AUDIO_PROPERTY_INPUT_MUTE + 1)

#define TEST_TAC5112_REG_CH_EN_RESET_VAL     (0xCCU)
#define TEST_TAC5112_REG_INT_MASK0_RESET_VAL (0xFFU)
#define TEST_TAC5112_REG_CLK_CFG15_RESET_VAL (0x01U)

#define TAC5112_EMUL_NODE DT_ALIAS(tac5112_0)

/* Retry delay used to park a failed-read retry for the duration of a test. */
#define TEST_TAC5112_PARKED_RETRY_MS (600000U)

struct tac5112_fixture {
	const struct device *dev;
};

static struct tac5112_fixture fixture;

/* State recorded by fault_cb(), which runs on the system workqueue. */
static struct {
	uint32_t count;
	uint32_t last_errors;
	/** Every error bit delivered so far, across all callbacks. */
	uint32_t all_errors;
	const struct device *last_dev;
} fault_log;

static struct audio_codec_cfg valid_codec_cfg(void)
{
	struct audio_codec_cfg cfg = {0};

	cfg.mclk_freq = 0U;
	cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	cfg.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE;
	cfg.dai_cfg.i2s.word_size = AUDIO_PCM_WIDTH_16_BITS;
	cfg.dai_cfg.i2s.channels = TEST_TA5112_CHANNELS_2;
	cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	cfg.dai_cfg.i2s.frame_clk_freq = TEST_TA5112_SAMPLE_FREQ_HZ;

	return cfg;
}

static void configure_dut(const struct device *dev)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	zassert_ok(audio_codec_configure(dev, &cfg), "baseline configure() failed");
}

static void fault_cb(const struct device *dev, uint32_t errors)
{
	fault_log.count++;
	fault_log.last_errors = errors;
	fault_log.all_errors |= errors;
	fault_log.last_dev = dev;
}

/* Wait for the driver's deferred fault handling triggered by the IRQ edge. */
static void fault_work_drain(const struct device *dev)
{
	struct tac5112_data *data = dev->data;
	struct k_work_sync sync;

	/* Twice: a first run may reschedule itself to retry a failed read. */
	(void)k_work_flush_delayable(&data->fault_work, &sync);
	(void)k_work_flush_delayable(&data->fault_work, &sync);
}

static void *tac5112_suite_setup(void)
{
	fixture.dev = DEVICE_DT_GET(DT_ALIAS(tac5112_0));
	zassert_true(device_is_ready(fixture.dev), "TAC5112 emulated device is not ready");

	return &fixture;
}

static void tac5112_suite_reset(void *f)
{
	ARG_UNUSED(f);

	struct tac5112_data *d = fixture.dev->data;
	struct k_work_sync sync;

	(void)k_work_cancel_delayable_sync(&d->fault_work, &sync);
	tac5112_emul_clear_failures(EMUL_DT_GET(TAC5112_EMUL_NODE));
	d->fault_pending = 0U;
	d->fault_retries = 0U;
	d->fault_retry_ms = TAC5112_FAULT_RETRY_MS;
	d->uv_reported = false;
	(void)atomic_set(&d->irq_edge, 0);
	d->irq_edge_seen = 0;

	zassert_equal(tac5112_reg_write(fixture.dev, TAC5112_REG_SW_RESET, 1), 0,
		      "Failed to reset the TAC5112");

	zassert_ok(audio_codec_register_error_callback(fixture.dev, NULL));
	memset(&fault_log, 0, sizeof(fault_log));
}

ZTEST_SUITE(tac5112, NULL, tac5112_suite_setup, tac5112_suite_reset, NULL, NULL);

/* I2S/LJ stereo: channel 1 is left slot 0, channel 2 is right slot 0, on TX and RX. */
static void expect_stereo_slots(const struct device *dev)
{
	static const struct {
		struct tac5112_reg reg;
		uint8_t expect;
	} slots[] = {
		{TAC5112_REG_PASI_TX_CH1_CFG, 0x20U},
		{TAC5112_REG_PASI_TX_CH2_CFG, 0x30U},
		{TAC5112_REG_PASI_RX_CH1_CFG, 0x20U},
		{TAC5112_REG_PASI_RX_CH2_CFG, 0x30U},
	};
	uint8_t val;

	for (size_t i = 0; i < ARRAY_SIZE(slots); i++) {
		zassert_ok(tac5112_reg_read(dev, slots[i].reg, &val));
		zassert_equal(val, slots[i].expect, "slot register 0x%02x: got 0x%02x, want 0x%02x",
			      slots[i].reg.addr, val, slots[i].expect);
	}
}

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

	expect_stereo_slots(fixture->dev);
}

ZTEST_F(tac5112, test_configure_left_justified_programs_lj_format)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();
	uint8_t val;

	cfg.dai_type = AUDIO_DAI_TYPE_LEFT_JUSTIFIED;
	cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED;
	zassert_ok(audio_codec_configure(fixture->dev, &cfg));

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PASI_CFG0, &val));
	zassert_equal((val & TAC5112_PASI_FORMAT_MASK) >> TAC5112_PASI_FORMAT_SHIFT,
		      TAC5112_PASI_FORMAT_LJ, "LJ format not programmed, got 0x%02x", val);
	zassert_equal((val & TAC5112_PASI_WLEN_MASK) >> TAC5112_PASI_WLEN_SHIFT,
		      TAC5112_PASI_WLEN_16, "word length disturbed, got 0x%02x", val);

	expect_stereo_slots(fixture->dev);
}

ZTEST_F(tac5112, test_configure_applies_clock_polarity)
{
	static const struct {
		uint32_t flags;
		uint8_t expect;
	} cases[] = {
		{0U, 0U},
		{I2S_FMT_BIT_CLK_INV, TAC5112_PASI_BCLK_POL_BIT},
		{I2S_FMT_FRAME_CLK_INV, TAC5112_PASI_FSYNC_POL_BIT},
		{I2S_FMT_BIT_CLK_INV | I2S_FMT_FRAME_CLK_INV,
		 TAC5112_PASI_BCLK_POL_BIT | TAC5112_PASI_FSYNC_POL_BIT},
	};
	uint8_t val;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct audio_codec_cfg cfg = valid_codec_cfg();

		cfg.dai_cfg.i2s.format |= cases[i].flags;
		zassert_ok(audio_codec_configure(fixture->dev, &cfg));
		zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PASI_CFG0, &val));
		zassert_equal(val & (TAC5112_PASI_BCLK_POL_BIT | TAC5112_PASI_FSYNC_POL_BIT),
			      cases[i].expect, "flags 0x%x: PASI_CFG0 0x%02x", cases[i].flags, val);
	}
}

ZTEST_F(tac5112, test_configure_applies_dai_route)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();
	uint8_t ch_en;
	uint8_t pwr;

	cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	zassert_ok(audio_codec_configure(fixture->dev, &cfg));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &ch_en));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PWR_CFG, &pwr));
	zassert_equal(ch_en, tac5112_ch_en_from_masks(0U, TEST_TAC5112_DEFAULT_CH_MASK),
		      "playback: got 0x%02x", ch_en);
	zassert_equal(pwr & TAC5112_PWR_CFG_ADC_PDZ_BIT, 0U, "playback must leave ADC off");

	cfg.dai_route = AUDIO_ROUTE_CAPTURE;
	zassert_ok(audio_codec_configure(fixture->dev, &cfg));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &ch_en));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PWR_CFG, &pwr));
	zassert_equal(ch_en, tac5112_ch_en_from_masks(TEST_TAC5112_DEFAULT_CH_MASK, 0U),
		      "capture: got 0x%02x", ch_en);
	zassert_equal(pwr & TAC5112_PWR_CFG_ADC_PDZ_BIT, TAC5112_PWR_CFG_ADC_PDZ_BIT);

	cfg.dai_route = AUDIO_ROUTE_PLAYBACK_CAPTURE;
	zassert_ok(audio_codec_configure(fixture->dev, &cfg));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &ch_en));
	zassert_equal(ch_en,
		      tac5112_ch_en_from_masks(TEST_TAC5112_DEFAULT_CH_MASK,
					       TEST_TAC5112_DEFAULT_CH_MASK),
		      "playback+capture: 0x%02x", ch_en);
}

ZTEST_F(tac5112, test_configure_rejects_unsupported_dai_route)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	cfg.dai_route = AUDIO_ROUTE_BYPASS;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -ENOTSUP);

	cfg.dai_route = (audio_route_t)TEST_TAC5112_ROUTE_UNKNOWN;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -ENOTSUP);
}

ZTEST_F(tac5112, test_configure_rejects_null_cfg)
{
	const struct audio_codec_driver_api *api = DEVICE_API_GET(audio_codec, fixture->dev);

	zassert_equal(audio_codec_configure(fixture->dev, NULL), -EINVAL);

	struct audio_codec_cfg cfg = valid_codec_cfg();

	zassert_equal(api->configure(NULL, &cfg), -EINVAL);
}

ZTEST_F(tac5112, test_configure_rejects_wrong_channel_count)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	cfg.dai_cfg.i2s.channels = TEST_TA5112_CHANNELS_1;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	cfg.dai_cfg.i2s.channels = TEST_TA5112_CHANNELS_4;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);
}

ZTEST_F(tac5112, test_configure_rejects_out_of_range_sample_rate)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	cfg.dai_cfg.i2s.frame_clk_freq = TEST_TA5112_FRAME_CLK_HZ_FLOOR_BELOW;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	cfg.dai_cfg.i2s.frame_clk_freq = TEST_TA5112_FRAME_CLK_HZ_CEILING_ABOVE;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);
}

static struct audio_codec_cfg controller_cfg(uint32_t fs, uint32_t mclk)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	cfg.dai_cfg.i2s.options = 0U; /* codec drives both BCLK and FSYNC */
	cfg.dai_cfg.i2s.frame_clk_freq = fs;
	cfg.mclk_freq = mclk;

	return cfg;
}

static uint8_t rd(const struct device *dev, struct tac5112_reg reg)
{
	uint8_t val;

	zassert_ok(tac5112_reg_read(dev, reg, &val));
	return val;
}

ZTEST_F(tac5112, test_configure_target_mode_leaves_controller_setup_off)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	zassert_ok(audio_codec_configure(fixture->dev, &cfg));

	zassert_equal(
		rd(fixture->dev, TAC5112_REG_PASI_TX_CFG0) &
			(TAC5112_PASI_TX_USE_INT_BCLK_BIT | TAC5112_PASI_TX_USE_INT_FSYNC_BIT),
		0U);
	zassert_equal(rd(fixture->dev, TAC5112_REG_CNT_CLK_CFG2) & TAC5112_PASI_CNT_CFG_BIT, 0U);
	zassert_equal(rd(fixture->dev, TAC5112_REG_INTF_CFG0) & TAC5112_INTF_CFG0_CCLK_SEL_MASK,
		      0U);
	zassert_equal(rd(fixture->dev, TAC5112_REG_CLK_CFG0) & TAC5112_PASI_SAMP_RATE_MASK,
		      TAC5112_PASI_SAMP_RATE_AUTO, "target mode must auto-detect the rate");
}

ZTEST_F(tac5112, test_configure_controller_mode_with_mclk_multiple_of_fs)
{
	/* 12.288 MHz CCLK on GPIO2, 48 kHz, 2 x 16-bit slots. */
	struct audio_codec_cfg cfg = controller_cfg(48000U, 12288000U);
	const struct device *dev = fixture->dev;

	zassert_ok(audio_codec_configure(dev, &cfg));

	expect_stereo_slots(dev);

	/* GPIO2 is an input carrying CCLK. */
	zassert_equal((rd(dev, TAC5112_REG_GPIO2_CFG0) & TAC5112_GPIO2_CFG_MASK) >>
			      TAC5112_GPIO2_CFG_SHIFT,
		      TAC5112_GPIO2_CFG_INPUT);
	zassert_equal((rd(dev, TAC5112_REG_INTF_CFG0) & TAC5112_INTF_CFG0_CCLK_SEL_MASK) >>
			      TAC5112_INTF_CFG0_CCLK_SEL_SHIFT,
		      TAC5112_INTF_CFG0_CCLK_SEL_GPIO2);

	/* Primary ASI is the controller, 48 kHz family. */
	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG2) &
			      (TAC5112_PASI_CNT_CFG_BIT | TAC5112_FS_MODE_44K1_BIT),
		      TAC5112_PASI_CNT_CFG_BIT);

	/* CCLK/FSYNC ratio 12.288 MHz / 48 kHz = 256 = 0x0100. */
	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG0) & TAC5112_CCLK_FS_RATIO_MSB_MASK, 0x01U);
	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG1), 0x00U);

	/* BCLK/FSYNC ratio 32 and the 48 kHz sample-rate setting (code 20). */
	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG3) & TAC5112_PASI_BCLK_FS_RATIO_MSB_MASK, 0U);
	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG4), 32U);
	zassert_equal((rd(dev, TAC5112_REG_CLK_CFG0) & TAC5112_PASI_SAMP_RATE_MASK) >>
			      TAC5112_PASI_SAMP_RATE_SHIFT,
		      20U);

	/* CCLK synchronised with the primary FSYNC is the clock source. */
	zassert_equal((rd(dev, TAC5112_REG_CLK_CFG2) & TAC5112_CLK_SRC_SEL_MASK) >>
			      TAC5112_CLK_SRC_SEL_SHIFT,
		      TAC5112_PLL_CLK_SRC_CCLK_PASI_FSYNC);

	/* The codec generates its output data from the internal clocks. */
	zassert_equal(rd(dev, TAC5112_REG_PASI_TX_CFG0) & (TAC5112_PASI_TX_USE_INT_BCLK_BIT |
							   TAC5112_PASI_TX_USE_INT_FSYNC_BIT),
		      TAC5112_PASI_TX_USE_INT_BCLK_BIT | TAC5112_PASI_TX_USE_INT_FSYNC_BIT);
}

ZTEST_F(tac5112, test_configure_controller_mode_44k1_family_and_wide_slots)
{
	/* 11.2896 MHz = 256 x 44.1 kHz, 2 x 32-bit slots -> ratio 64. */
	struct audio_codec_cfg cfg = controller_cfg(44100U, 11289600U);
	const struct device *dev = fixture->dev;

	cfg.dai_cfg.i2s.word_size = AUDIO_PCM_WIDTH_32_BITS;
	zassert_ok(audio_codec_configure(dev, &cfg));

	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG2) & TAC5112_FS_MODE_44K1_BIT,
		      TAC5112_FS_MODE_44K1_BIT);
	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG4), 64U);
}

ZTEST_F(tac5112, test_configure_controller_mode_programs_the_cclk_fsync_ratio)
{
	static const struct {
		uint32_t fs;
		uint32_t mclk;
		uint8_t msb;
		uint8_t lsb;
	} cases[] = {
		/* TI's reference: 22.5792 MHz / 44.1 kHz = 512 -> 0x02 / 0x00. */
		{44100U, 22579200U, 0x02U, 0x00U},
		{48000U, 12288000U, 0x01U, 0x00U},
		{48000U, 24576000U, 0x02U, 0x00U},
		{16000U, 12288000U, 0x03U, 0x00U},  /* 768 */
		{44100U, 11289600U, 0x01U, 0x00U},  /* 256 */
		{48000U, 18432000U, 0x01U, 0x80U},  /* 384: a non-zero LSB */
		{8000U, 36864000U, 0x12U, 0x00U},   /* 4608: MSB above one bit */
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct audio_codec_cfg cfg = controller_cfg(cases[i].fs, cases[i].mclk);
		const struct device *dev = fixture->dev;
		int ret = audio_codec_configure(dev, &cfg);

		zassert_ok(ret, "%u Hz / %u Hz: configure returned %d", cases[i].mclk, cases[i].fs,
			   ret);
		zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG0) & TAC5112_CCLK_FS_RATIO_MSB_MASK,
			      cases[i].msb, "%u/%u: wrong CNT_CLK_CFG0",
				  cases[i].mclk, cases[i].fs);
		zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG1), cases[i].lsb,
			      "%u/%u: wrong CNT_CLK_CFG1", cases[i].mclk, cases[i].fs);
	}
}

ZTEST_F(tac5112, test_configure_controller_mode_fixed_cclk_frequency)
{
	/* 13 MHz is not a multiple of 48 kHz, so the fixed-CCLK mode is used. */
	struct audio_codec_cfg cfg = controller_cfg(48000U, 13000000U);
	const struct device *dev = fixture->dev;

	zassert_ok(audio_codec_configure(dev, &cfg));

	zassert_equal((rd(dev, TAC5112_REG_CLK_CFG2) & TAC5112_CLK_SRC_SEL_MASK) >>
			      TAC5112_CLK_SRC_SEL_SHIFT,
		      TAC5112_PLL_CLK_SRC_FIXED_CCLK);
	zassert_equal((rd(dev, TAC5112_REG_CNT_CLK_CFG2) & TAC5112_CCLK_FREQ_SEL_MASK) >>
			      TAC5112_CCLK_FREQ_SEL_SHIFT,
		      2U, "13 MHz is CCLK_FREQ_SEL 2");
	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG2) & TAC5112_PASI_CNT_CFG_BIT,
		      TAC5112_PASI_CNT_CFG_BIT);

	/* No integer CCLK/FSYNC ratio exists, so the registers keep auto-detect. */
	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG0) & TAC5112_CCLK_FS_RATIO_MSB_MASK, 0U);
	zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG1), 0U);
}

ZTEST_F(tac5112, test_configure_controller_mode_picks_rate_code_and_family_together)
{
	static const struct {
		uint32_t fs;
		uint8_t code;
		uint8_t fs_mode;
	} cases[] = {
		{48000U, 20U, 0U},
		{44100U, 20U, TAC5112_FS_MODE_44K1_BIT},
		{16000U, 28U, 0U},
		{14700U, 28U, TAC5112_FS_MODE_44K1_BIT},
		{96000U, 15U, 0U},
		{88200U, 15U, TAC5112_FS_MODE_44K1_BIT},
		{22050U, 25U, TAC5112_FS_MODE_44K1_BIT},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		/* 12 MHz is a fixed CCLK frequency but not a multiple of these rates. */
		struct audio_codec_cfg cfg = controller_cfg(cases[i].fs, 12000000U);
		const struct device *dev = fixture->dev;
		int ret = audio_codec_configure(dev, &cfg);

		zassert_ok(ret, "fs %u: configure returned %d", cases[i].fs, ret);
		zassert_equal((rd(dev, TAC5112_REG_CLK_CFG0) & TAC5112_PASI_SAMP_RATE_MASK) >>
				      TAC5112_PASI_SAMP_RATE_SHIFT,
			      cases[i].code, "fs %u: wrong PASI_SAMP_RATE", cases[i].fs);
		zassert_equal(rd(dev, TAC5112_REG_CNT_CLK_CFG2) & TAC5112_FS_MODE_44K1_BIT,
			      cases[i].fs_mode, "fs %u: wrong FS_MODE", cases[i].fs);
	}
}

ZTEST_F(tac5112, test_configure_controller_mode_rejects_rates_it_cannot_generate)
{
	/* These share a code and family with a supported rate but are different
	 * rates: 45 kHz would be generated as 44.1 kHz or 48 kHz, 15 kHz as
	 * 14.7 kHz or 16 kHz, and so on.
	 */
	static const uint32_t rates[] = {45000U, 15000U, 47000U, 44000U, 16500U, 30000U};

	for (size_t i = 0; i < ARRAY_SIZE(rates); i++) {
		struct audio_codec_cfg cfg = controller_cfg(rates[i], 13000000U);

		zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL,
			      "%u Hz must be rejected", rates[i]);
	}
}

ZTEST_F(tac5112, test_configure_controller_mode_rejects_bad_requests)
{
	struct audio_codec_cfg cfg;

	/* Mixed clock directions: PASI_CNT_CFG is a single bit. */
	cfg = controller_cfg(48000U, 12288000U);
	cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_TARGET;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	cfg = controller_cfg(48000U, 12288000U);
	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	/* No CCLK frequency supplied. */
	cfg = controller_cfg(48000U, 0U);
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	/* Above the 36.864 MHz CCLK limit. */
	cfg = controller_cfg(48000U, 49152000U);
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	/* Neither a multiple of fs nor a CCLK_FREQ_SEL frequency. */
	cfg = controller_cfg(48000U, 13500000U);
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	cfg = controller_cfg(4150U, 12000000U);
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);
}

ZTEST_F(tac5112, test_configure_rejects_lsb_first_data_order)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();
	uint8_t before;

	zassert_ok(audio_codec_configure(fixture->dev, &cfg));
	before = rd(fixture->dev, TAC5112_REG_PASI_CFG0);

	cfg.dai_cfg.i2s.format |= I2S_FMT_DATA_ORDER_LSB;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);

	/* Rejected before the codec is reset or reprogrammed. */
	zassert_equal(rd(fixture->dev, TAC5112_REG_PASI_CFG0), before);

	/* MSB-first, the codec's only order, is still accepted. */
	cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S | I2S_FMT_DATA_ORDER_MSB;
	zassert_ok(audio_codec_configure(fixture->dev, &cfg));
}

ZTEST_F(tac5112, test_configure_rejects_unsupported_word_size)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();

	cfg.dai_cfg.i2s.word_size = 0U;
	zassert_equal(audio_codec_configure(fixture->dev, &cfg), -EINVAL);
}

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

ZTEST_F(tac5112, test_output_volume_bounds_checking)
{
	audio_property_value_t val;
	uint8_t before;
	uint8_t after;

	configure_dut(fixture->dev);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1A_CFG0, &before));

	val.vol = TAC5112_DAC_VOL_HALF_DB_MIN - 1;
	zassert_equal(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					       AUDIO_CHANNEL_FRONT_LEFT, val),
		      -EINVAL);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1A_CFG0, &after));
	zassert_equal(before, after, "rejected volume write must not touch hardware");

	val.vol = TAC5112_DAC_VOL_HALF_DB_MAX + 1;
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
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1B_CFG0, &after));
	zassert_equal(after, TAC5112_DAC_DVOL_0DB);

	/* The extreme ends of the range must be accepted. */
	val.vol = (int)TAC5112_DAC_VOL_HALF_DB_MIN;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_FRONT_LEFT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH1A_CFG0, &after));
	zassert_equal(after, TAC5112_DAC_DVOL_MIN);

	val.vol = (int)TAC5112_DAC_VOL_HALF_DB_MAX;
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

	val.vol = TEST_TAC5112_DVOL_DB;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DAC_DVOL_0DB + TEST_TAC5112_DVOL_DB);

	val.mute = true;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_MUTE,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DVOL_MUTE);

	val.mute = false;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_MUTE,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DAC_DVOL_0DB + TEST_TAC5112_DVOL_DB,
		      "unmute must restore the cached volume");
}

ZTEST_F(tac5112, test_failed_mute_write_does_not_commit_mute_state)
{
	audio_property_value_t val;
	uint8_t reg;

	configure_dut(fixture->dev);

	/* Second DAC register of the right channel NACKs: mute fails part way. */
	tac5112_emul_fail_writes(EMUL_DT_GET(TAC5112_EMUL_NODE), TAC5112_REG_DAC_CH2B_CFG0, true);

	val.mute = true;
	zassert_not_equal(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_MUTE,
						   AUDIO_CHANNEL_FRONT_RIGHT, val),
			  0, "mute must report the failed write");

	/* The already-written first register is rolled back. */
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DAC_DVOL_0DB, "partial mute not rolled back, got 0x%02x", reg);

	tac5112_emul_fail_writes(EMUL_DT_GET(TAC5112_EMUL_NODE), TAC5112_REG_DAC_CH2B_CFG0, false);

	/* The channel was never muted, so a volume update must reach hardware. */
	val.vol = TEST_TAC5112_DVOL_DB;
	zassert_ok(audio_codec_set_property(fixture->dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DAC_DVOL_0DB + TEST_TAC5112_DVOL_DB);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_DAC_CH2B_CFG0, &reg));
	zassert_equal(reg, TAC5112_DAC_DVOL_0DB + TEST_TAC5112_DVOL_DB);
}

ZTEST_F(tac5112, test_input_volume_uses_adc_range)
{
	audio_property_value_t val;
	uint8_t reg;

	configure_dut(fixture->dev);

	val.vol = (int)TAC5112_ADC_VOL_HALF_DB_MAX + 1;
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
	zassert_equal(audio_codec_set_property(fixture->dev, TEST_AUDIO_PROPERTY_NOT_SUPPORTED,
					       AUDIO_CHANNEL_ALL, val),
		      -ENOTSUP);
}

ZTEST_F(tac5112, test_set_property_rejects_null_dev)
{
	const struct audio_codec_driver_api *api = DEVICE_API_GET(audio_codec, fixture->dev);
	audio_property_value_t val = {.vol = 0};

	zassert_equal(api->set_property(NULL, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL, val),
		      -EINVAL);
}

ZTEST_F(tac5112, test_apply_properties_is_a_no_op_success)
{
	zassert_ok(audio_codec_apply_properties(fixture->dev));
}

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
		.dmul = TEST_TAC5112_CUSTOM_PLL,
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
	zassert_equal(val & TAC5112_PLL_DMUL_MSB_MASK,
		      (TEST_TAC5112_CUSTOM_PLL >> 8) & TAC5112_PLL_DMUL_MSB_MASK);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG17, &val));
	zassert_equal(val, (uint8_t)(TEST_TAC5112_CUSTOM_PLL & 0xFFU));

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

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG15, &after));
	zassert_equal(before, after);
}

static void trim_setup_near_dmul_max(const struct device *dev)
{
	struct tac5112_pll_config pll = {
		.auto_config = false,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
		.pdiv = 1U,
		.jmul = 8U,
		.dmul = 9983U, /* 0x26FF: a +1e6 ppb trim clamps to 9999 = 0x270F */
		.ndiv = 1U,
		.mdiv = 1U,
	};

	configure_dut(dev);
	zassert_ok(tac5112_configure_pll(dev, &pll));
}

static uint16_t read_dmul(const struct device *dev)
{
	uint8_t cfg16;
	uint8_t cfg17;

	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_CLK_CFG16, &cfg16));
	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_CLK_CFG17, &cfg17));

	return (uint16_t)(((cfg16 & TAC5112_PLL_DMUL_MSB_MASK) << 8) | cfg17);
}

static void trim_setup_dmul(const struct device *dev, uint16_t dmul)
{
	struct tac5112_pll_config pll = {
		.auto_config = false,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
		.pdiv = 1U,
		.jmul = 8U,
		.dmul = dmul,
		.ndiv = 1U,
		.mdiv = 1U,
	};

	configure_dut(dev);
	zassert_ok(tac5112_configure_pll(dev, &pll));
}

ZTEST_F(tac5112, test_trim_pll_failed_second_write_restores_old_multiplier)
{
	const struct emul *emul = EMUL_DT_GET(TAC5112_EMUL_NODE);

	trim_setup_near_dmul_max(fixture->dev);
	zassert_equal(read_dmul(fixture->dev), 9983U);

	/* 0x26FF -> 0x270F goes LSB first, so the MSB write is the second. */
	tac5112_emul_fail_writes(emul, TAC5112_REG_CLK_CFG16, true);
	zassert_not_equal(tac5112_trim_pll(fixture->dev, 1000000), 0);
	tac5112_emul_clear_failures(emul);

	zassert_equal(read_dmul(fixture->dev), 9983U, "old multiplier not restored");

	/* Trimming remains available after a clean rollback. */
	zassert_ok(tac5112_trim_pll(fixture->dev, 1000000));
	zassert_equal(read_dmul(fixture->dev), TAC5112_PLL_DMUL_MAX);
}

ZTEST_F(tac5112, test_trim_pll_failed_rollback_disables_trimming)
{
	const struct emul *emul = EMUL_DT_GET(TAC5112_EMUL_NODE);

	trim_setup_near_dmul_max(fixture->dev);

	tac5112_emul_fail_writes_after(emul, TAC5112_REG_CLK_CFG17, 1U);
	tac5112_emul_fail_writes(emul, TAC5112_REG_CLK_CFG16, true);
	zassert_not_equal(tac5112_trim_pll(fixture->dev, 1000000), 0);
	tac5112_emul_clear_failures(emul);

	/* DMUL is unknown, so further trims are refused until reconfiguration. */
	zassert_equal(tac5112_trim_pll(fixture->dev, 0), -ENOTSUP);
}

ZTEST_F(tac5112, test_trim_pll_never_passes_through_an_invalid_multiplier)
{
	const struct emul *emul = EMUL_DT_GET(TAC5112_EMUL_NODE);

	trim_setup_near_dmul_max(fixture->dev);
	tac5112_emul_fail_writes_after(emul, TAC5112_REG_CLK_CFG17, 1U);
	tac5112_emul_fail_writes(emul, TAC5112_REG_CLK_CFG16, true);
	zassert_not_equal(tac5112_trim_pll(fixture->dev, 1000000), 0);
	tac5112_emul_clear_failures(emul);
	zassert_true(read_dmul(fixture->dev) <= TAC5112_PLL_DMUL_MAX,
		     "intermediate DMUL %u above the valid maximum", read_dmul(fixture->dev));

	trim_setup_dmul(fixture->dev, 9984U);
	tac5112_emul_fail_writes_after(emul, TAC5112_REG_CLK_CFG16, 1U);
	tac5112_emul_fail_writes(emul, TAC5112_REG_CLK_CFG17, true);
	zassert_not_equal(tac5112_trim_pll(fixture->dev, -11111), 0);
	tac5112_emul_clear_failures(emul);
	zassert_true(read_dmul(fixture->dev) <= TAC5112_PLL_DMUL_MAX,
		     "intermediate DMUL %u above the valid maximum", read_dmul(fixture->dev));

	/* Successful updates in both directions end on the requested values. */
	trim_setup_dmul(fixture->dev, 9984U);
	zassert_ok(tac5112_trim_pll(fixture->dev, -11111));
	zassert_equal(read_dmul(fixture->dev), 9983U);

	trim_setup_near_dmul_max(fixture->dev);
	zassert_ok(tac5112_trim_pll(fixture->dev, 1000000));
	zassert_equal(read_dmul(fixture->dev), TAC5112_PLL_DMUL_MAX);
}

ZTEST_F(tac5112, test_custom_pll_is_disabled_while_dividers_are_written)
{
	const struct emul *emul = EMUL_DT_GET(TAC5112_EMUL_NODE);
	const struct device *dev = fixture->dev;
	struct tac5112_pll_config pll = {
		.auto_config = false,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
		.pdiv = 1U,
		.jmul = 8U,
		.dmul = 9984U, /* 0x2700: MSB first from 0x26FF passes through 0x27FF */
		.ndiv = 1U,
		.mdiv = 1U,
	};
	uint8_t val;

	/* Start from 9983 (0x26FF) with the PLL running. */
	trim_setup_near_dmul_max(dev);
	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_CLK_CFG2, &val));
	zassert_equal(val & TAC5112_PLL_DIS_BIT, 0U, "PLL should be enabled after a custom config");

	/* The MSB write lands, the LSB write fails: the multiplier is left at the
	 * invalid 0x27FF, so the PLL must not be left running on it.
	 */
	tac5112_emul_fail_writes(emul, TAC5112_REG_CLK_CFG17, true);
	zassert_not_equal(tac5112_configure_pll(dev, &pll), 0);
	tac5112_emul_clear_failures(emul);

	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_CLK_CFG2, &val));
	zassert_equal(val & TAC5112_PLL_DIS_BIT, TAC5112_PLL_DIS_BIT,
		      "PLL must stay disabled after a failed divider write");
	zassert_equal(tac5112_trim_pll(dev, 0), -ENOTSUP, "failed config must not look active");

	/* A complete configuration enables the PLL again, with the new value. */
	zassert_ok(tac5112_configure_pll(dev, &pll));
	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_CLK_CFG2, &val));
	zassert_equal(val & TAC5112_PLL_DIS_BIT, 0U, "PLL not re-enabled after success");
	zassert_equal(read_dmul(dev), 9984U);
}

ZTEST_F(tac5112, test_auto_pll_stays_disabled_until_leaving_custom_mode_succeeds)
{
	const struct emul *emul = EMUL_DT_GET(TAC5112_EMUL_NODE);
	const struct device *dev = fixture->dev;
	struct tac5112_pll_config custom = {
		.auto_config = false,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
		.pdiv = 1U,
		.jmul = 8U,
		.dmul = 9984U,
		.ndiv = 1U,
		.mdiv = 1U,
	};
	struct tac5112_pll_config auto_pll = {
		.auto_config = true,
		.fractional_allowed = true,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
	};
	uint8_t val;

	/* Leave the codec with the invalid 0x27FF multiplier and the PLL disabled
	 * (see test_custom_pll_is_disabled_while_dividers_are_written).
	 */
	trim_setup_near_dmul_max(dev);
	tac5112_emul_fail_writes(emul, TAC5112_REG_CLK_CFG17, true);
	zassert_not_equal(tac5112_configure_pll(dev, &custom), 0);
	tac5112_emul_clear_failures(emul);

	/* Switching to auto mode fails when CUSTOM_CLK_CFG cannot be cleared: the
	 * PLL must not have been re-enabled on the invalid multiplier.
	 */
	tac5112_emul_fail_writes(emul, TAC5112_REG_CLK_CFG0, true);
	zassert_not_equal(tac5112_configure_pll(dev, &auto_pll), 0);
	tac5112_emul_clear_failures(emul);

	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_CLK_CFG2, &val));
	zassert_equal(val & TAC5112_PLL_DIS_BIT, TAC5112_PLL_DIS_BIT,
		      "PLL re-enabled although leaving custom mode failed");

	/* Once the mode change succeeds the PLL is enabled again. */
	zassert_ok(tac5112_configure_pll(dev, &auto_pll));
	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_CLK_CFG2, &val));
	zassert_equal(val & TAC5112_PLL_DIS_BIT, 0U, "PLL not enabled after auto mode succeeded");
	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_CLK_CFG0, &val));
	zassert_equal(val & TAC5112_CUSTOM_CLK_CFG_BIT, 0U);
}

ZTEST_F(tac5112, test_pll_auto_mode_rejects_internal_oscillator)
{
	struct tac5112_pll_config pll = {
		.auto_config = true,
		.clk_src = TAC5112_PLL_CLK_SRC_INTERNAL_OSC,
	};
	uint8_t before;
	uint8_t after;

	configure_dut(fixture->dev);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG0, &before));

	zassert_equal(tac5112_configure_pll(fixture->dev, &pll), -EINVAL);

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG0, &after));
	zassert_equal(before, after, "rejected config must not touch hardware");
}

ZTEST_F(tac5112, test_pll_rejects_null_pointers)
{
	struct tac5112_pll_config pll = {.auto_config = true};

	zassert_equal(tac5112_configure_pll(NULL, &pll), -EINVAL);
	zassert_equal(tac5112_configure_pll(fixture->dev, NULL), -EINVAL);
}

ZTEST(tac5112, test_dmul_trim)
{
	/* Zero offset leaves the baseline DMUL unchanged. */
	zassert_equal(tac5112_dmul_trim(8U, TEST_TAC5112_CUSTOM_PLL, 0), TEST_TAC5112_CUSTOM_PLL);

	/* +100 ppm at JMUL=8: dDMUL = 85000 * 1e5 / 1e9 = 8.5, rounds to 9. */
	zassert_equal(tac5112_dmul_trim(8U, TEST_TAC5112_CUSTOM_PLL, 100000), 5009U);
	/* Symmetric in the negative direction. */
	zassert_equal(tac5112_dmul_trim(8U, TEST_TAC5112_CUSTOM_PLL, -100000), 4991U);

	/* Corrections beyond the DMUL register span saturate, not wrap. */
	zassert_equal(tac5112_dmul_trim(8U, 9990U, 1000000), TAC5112_PLL_DMUL_MAX);
	zassert_equal(tac5112_dmul_trim(8U, 5U, -1000000), 0U);

	zassert_equal(tac5112_dmul_trim(8U, TEST_TAC5112_CUSTOM_PLL, 1), TEST_TAC5112_CUSTOM_PLL);
}

ZTEST_F(tac5112, test_trim_pll_runtime)
{
	struct tac5112_pll_config pll = {
		.auto_config = false,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
		.pdiv = 1U,
		.jmul = 384U, /* > 255, so CLK_CFG16 also carries the JMUL MSB */
		.dmul = TEST_TAC5112_CUSTOM_PLL,
		.ndiv = 1U,
		.mdiv = 1U,
	};
	struct tac5112_pll_config auto_pll = {
		.auto_config = true,
		.clk_src = TAC5112_PLL_CLK_SRC_PASI_BCLK,
	};
	uint8_t cfg16_before;
	uint8_t cfg16_after;
	uint8_t val;
	uint16_t expect;

	configure_dut(fixture->dev);

	/* NULL dev is rejected before the custom-mode check. */
	zassert_equal(tac5112_trim_pll(NULL, 0), -EINVAL);

	/* A custom PLL records the baseline and enables trimming. */
	zassert_ok(tac5112_configure_pll(fixture->dev, &pll));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG16, &cfg16_before));

	/* Trim up: the DMUL numerator becomes tac5112_dmul_trim(384, 5000, +100000). */
	expect = tac5112_dmul_trim(384U, TEST_TAC5112_CUSTOM_PLL, 100000);
	zassert_ok(tac5112_trim_pll(fixture->dev, 100000));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG17, &val));
	zassert_equal(val, (uint8_t)(expect & 0xFFU));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG16, &cfg16_after));
	zassert_equal(cfg16_after & TAC5112_PLL_DMUL_MSB_MASK,
		      (expect >> 8) & TAC5112_PLL_DMUL_MSB_MASK);
	zassert_equal(cfg16_after & TAC5112_PLL_JMUL_MSB_BIT,
		      cfg16_before & TAC5112_PLL_JMUL_MSB_BIT,
		      "trim must not disturb the JMUL MSB bit");

	zassert_ok(tac5112_trim_pll(fixture->dev, 0));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG17, &val));
	zassert_equal(val, (uint8_t)(TEST_TAC5112_CUSTOM_PLL & 0xFFU));

	zassert_ok(tac5112_configure_pll(fixture->dev, &auto_pll));
	zassert_equal(tac5112_trim_pll(fixture->dev, 0), -ENOTSUP);
}

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

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &page0_val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_INT_MASK0, &page1_val));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG15, &page3_val));
	zassert_equal(page0_val, TEST_TAC5112_REG_CH_EN_RESET_VAL, "CH_EN reset default");
	zassert_equal(page1_val, TEST_TAC5112_REG_INT_MASK0_RESET_VAL, "INT_MASK0 reset default");
	zassert_equal(page3_val, TEST_TAC5112_REG_CLK_CFG15_RESET_VAL,
		      "CLK_CFG15 (PDIV) reset default");

	zassert_ok(
		tac5112_reg_write(fixture->dev, TAC5112_REG_CLK_CFG15, TEST_TAC5112_REG_CLK_CFG15));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &page0_val));
	zassert_equal(page0_val,
		      TAC5112_CH_EN_IN_CH1_BIT | TAC5112_CH_EN_IN_CH2_BIT |
			      TAC5112_CH_EN_OUT_CH1_BIT | TAC5112_CH_EN_OUT_CH2_BIT,
		      "page 0 register must be unaffected by a page 3 write");

	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CLK_CFG15, &page3_val));
	zassert_equal(page3_val, TEST_TAC5112_REG_CLK_CFG15);
}

ZTEST(tac5112, test_dvol_conversion_dac_range)
{
	uint8_t dvol;

	zassert_ok(tac5112_dvol_from_half_db(true, TAC5112_DAC_VOL_HALF_DB_MIN, &dvol));
	zassert_equal(dvol, TAC5112_DAC_DVOL_MIN);

	zassert_ok(tac5112_dvol_from_half_db(true, 0, &dvol));
	zassert_equal(dvol, TAC5112_DAC_DVOL_0DB);

	zassert_ok(tac5112_dvol_from_half_db(true, TAC5112_DAC_VOL_HALF_DB_MAX, &dvol));
	zassert_equal(dvol, TAC5112_DAC_DVOL_MAX);

	zassert_equal(tac5112_dvol_from_half_db(true, TAC5112_DAC_VOL_HALF_DB_MIN - 1, &dvol),
		      -EINVAL);
	zassert_equal(tac5112_dvol_from_half_db(true, TAC5112_DAC_VOL_HALF_DB_MAX + 1, &dvol),
		      -EINVAL);
}

ZTEST(tac5112, test_dvol_conversion_adc_range)
{
	uint8_t dvol;

	zassert_ok(tac5112_dvol_from_half_db(false, TAC5112_ADC_VOL_HALF_DB_MIN, &dvol));
	zassert_equal(dvol, TAC5112_ADC_DVOL_MIN);

	zassert_ok(tac5112_dvol_from_half_db(false, TAC5112_ADC_VOL_HALF_DB_MAX, &dvol));
	zassert_equal(dvol, TAC5112_ADC_DVOL_MAX);

	zassert_equal(tac5112_dvol_from_half_db(false, TAC5112_ADC_VOL_HALF_DB_MAX + 1, &dvol),
		      -EINVAL);
}

ZTEST(tac5112, test_dvol_conversion_rejects_null_output)
{
	zassert_equal(tac5112_dvol_from_half_db(true, 0, NULL), -EINVAL);
}

ZTEST(tac5112, test_dvol_conversion_does_not_truncate_before_bounds_check)
{
	uint8_t dvol = TEST_TAC5112_DVOL;

	zassert_equal(tac5112_dvol_from_half_db(true, TEST_TAC5112_DVOL_CEILING_ABOVE, &dvol),
		      -EINVAL);
	zassert_equal(dvol, TEST_TAC5112_DVOL,
		      "rejected conversion must not touch "
		      "the output parameter");
}

ZTEST(tac5112, test_ch_en_from_masks)
{
	/* Stereo record + playback (the default) maps to CH1/CH2 in each nibble. */
	zassert_equal(tac5112_ch_en_from_masks(0x3U, 0x3U),
		      TAC5112_CH_EN_IN_CH1_BIT | TAC5112_CH_EN_IN_CH2_BIT |
			      TAC5112_CH_EN_OUT_CH1_BIT | TAC5112_CH_EN_OUT_CH2_BIT);

	/* Individual channels map to their documented bit positions. */
	zassert_equal(tac5112_ch_en_from_masks(0x1U, 0x0U), TAC5112_CH_EN_IN_CH1_BIT);
	zassert_equal(tac5112_ch_en_from_masks(0x2U, 0x0U), TAC5112_CH_EN_IN_CH2_BIT);
	zassert_equal(tac5112_ch_en_from_masks(0x0U, 0x1U), TAC5112_CH_EN_OUT_CH1_BIT);
	zassert_equal(tac5112_ch_en_from_masks(0x0U, 0x2U), TAC5112_CH_EN_OUT_CH2_BIT);

	/* Only two physical channels exist: higher mask bits are ignored. */
	zassert_equal(tac5112_ch_en_from_masks(0xCU, 0xCU), 0x00U);
	zassert_equal(tac5112_ch_en_from_masks(0x0U, 0x0U), 0x00U);
}

ZTEST_F(tac5112, test_set_channels_runtime)
{
	struct audio_codec_cfg cfg = valid_codec_cfg();
	uint8_t val;

	zassert_ok(audio_codec_configure(fixture->dev, &cfg));

	/* Re-select both record channels and no playback channels. */
	zassert_ok(tac5112_set_channels(fixture->dev, 0x3U, 0x0U));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &val));
	zassert_equal(val, TAC5112_CH_EN_IN_CH1_BIT | TAC5112_CH_EN_IN_CH2_BIT);
	/* Inputs enabled -> ADC and MICBIAS powered. */
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PWR_CFG, &val));
	zassert_equal(val & (TAC5112_PWR_CFG_ADC_PDZ_BIT | TAC5112_PWR_CFG_MICBIAS_PDZ_BIT),
		      TAC5112_PWR_CFG_ADC_PDZ_BIT | TAC5112_PWR_CFG_MICBIAS_PDZ_BIT);

	/* Switch to playback-only: outputs enabled, record path powered down. */
	zassert_ok(tac5112_set_channels(fixture->dev, 0x0U, 0x3U));
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &val));
	zassert_equal(val, TAC5112_CH_EN_OUT_CH1_BIT | TAC5112_CH_EN_OUT_CH2_BIT);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_PWR_CFG, &val));
	zassert_equal(val & (TAC5112_PWR_CFG_ADC_PDZ_BIT | TAC5112_PWR_CFG_MICBIAS_PDZ_BIT), 0U);

	/* Out-of-range masks are rejected without changing CH_EN. */
	zassert_equal(tac5112_set_channels(fixture->dev, 0x4U, 0x0U), -EINVAL);
	zassert_equal(tac5112_set_channels(fixture->dev, 0x0U, 0x8U), -EINVAL);
	zassert_equal(tac5112_set_channels(fixture->dev, 0x10U, 0x0U), -EINVAL);
	zassert_equal(tac5112_set_channels(fixture->dev, 0x0U, 0xFFU), -EINVAL);
	zassert_equal(tac5112_set_channels(NULL, 0x1U, 0x1U), -EINVAL);
	zassert_ok(tac5112_reg_read(fixture->dev, TAC5112_REG_CH_EN, &val));
	zassert_equal(val, TAC5112_CH_EN_OUT_CH1_BIT | TAC5112_CH_EN_OUT_CH2_BIT,
		      "rejected set_channels must not change CH_EN");
}

/* --- Asynchronous fault path (IRQ -> work -> error callback) ------------ */

static const struct emul *fault_emul(void)
{
	return EMUL_DT_GET(TAC5112_EMUL_NODE);
}

static int fault_irq_asserted(void)
{
	static const struct gpio_dt_spec irq = GPIO_DT_SPEC_GET(TAC5112_EMUL_NODE, fault_gpios);

	return gpio_pin_get_dt(&irq);
}

/* Mask the output fault interrupts (they are unmasked out of reset). */
static void fault_irqs_mask(const struct device *dev)
{
	zassert_ok(tac5112_reg_write(dev, TAC5112_REG_INT_MASK4, 0xFFU));
}

static void fault_arm(const struct device *dev)
{
	configure_dut(dev);
	zassert_ok(audio_codec_register_error_callback(dev, fault_cb));
	zassert_equal(fault_irq_asserted(), 0, "IRQ line asserted with no fault pending");
}

static void expect_single_callback(const struct device *dev, uint32_t errors)
{
	fault_work_drain(dev);

	zassert_equal(fault_log.count, 1U, "expected 1 callback, got %u", fault_log.count);
	zassert_equal(fault_log.last_errors, errors, "errors 0x%x, expected 0x%x",
		      fault_log.last_errors, errors);
	zassert_equal(fault_log.last_dev, dev, "callback got the wrong device");
}

static void fault_inject_and_expect(const struct device *dev, struct tac5112_reg reg, uint8_t bits,
				    uint32_t errors)
{
	memset(&fault_log, 0, sizeof(fault_log));
	fault_arm(dev);

	zassert_ok(tac5112_emul_inject_latch(fault_emul(), reg, bits));
	expect_single_callback(dev, errors);

	zassert_ok(tac5112_reg_write(dev, TAC5112_REG_SW_RESET, 1));
}

ZTEST_F(tac5112, test_fault_overcurrent_maps_to_callback)
{
	static const struct tac5112_reg regs[] = {TAC5112_REG_OUT_CH1_LTCH,
						  TAC5112_REG_OUT_CH2_LTCH};
	static const uint8_t bits[] = {TAC5112_OUT_CH_SC_OUTP_BIT, TAC5112_OUT_CH_SC_OUTM_BIT};

	for (size_t r = 0; r < ARRAY_SIZE(regs); r++) {
		for (size_t b = 0; b < ARRAY_SIZE(bits); b++) {
			fault_inject_and_expect(fixture->dev, regs[r], bits[b],
						AUDIO_CODEC_ERROR_OVERCURRENT);
		}
	}
}

ZTEST_F(tac5112, test_fault_virtual_ground_maps_to_dc)
{
	static const struct tac5112_reg regs[] = {TAC5112_REG_OUT_CH1_LTCH,
						  TAC5112_REG_OUT_CH2_LTCH};
	static const uint8_t bits[] = {TAC5112_OUT_CH_VG_FAULT_P_BIT,
				       TAC5112_OUT_CH_VG_FAULT_M_BIT};

	for (size_t r = 0; r < ARRAY_SIZE(regs); r++) {
		for (size_t b = 0; b < ARRAY_SIZE(bits); b++) {
			fault_inject_and_expect(fixture->dev, regs[r], bits[b],
						AUDIO_CODEC_ERROR_DC);
		}
	}
}

ZTEST_F(tac5112, test_fault_brownout_maps_to_undervoltage)
{
	fault_arm(fixture->dev);

	tac5112_emul_set_brownout(fault_emul(), true);
	expect_single_callback(fixture->dev, AUDIO_CODEC_ERROR_UNDERVOLTAGE);
}

ZTEST_F(tac5112, test_fault_combined_errors_reported_in_one_callback)
{
	fault_arm(fixture->dev);

	k_sched_lock();
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH2_LTCH,
					     TAC5112_OUT_CH_VG_FAULT_M_BIT));
	tac5112_emul_set_brownout(fault_emul(), true);
	k_sched_unlock();

	expect_single_callback(fixture->dev, AUDIO_CODEC_ERROR_OVERCURRENT | AUDIO_CODEC_ERROR_DC |
						     AUDIO_CODEC_ERROR_UNDERVOLTAGE);
}

ZTEST_F(tac5112, test_fault_latches_clear_on_read)
{
	const struct tac5112_reg reg = TAC5112_REG_OUT_CH1_LTCH;
	const uint8_t bits = TAC5112_OUT_CH_SC_OUTP_BIT | TAC5112_OUT_CH_VG_FAULT_P_BIT;
	uint8_t val;

	fault_irqs_mask(fixture->dev);
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), reg, bits));

	zassert_ok(tac5112_reg_read(fixture->dev, reg, &val));
	zassert_equal(val, bits, "latch bits not visible on first read, got 0x%02x", val);

	zassert_ok(tac5112_reg_read(fixture->dev, reg, &val));
	zassert_equal(val, 0U, "latch must clear on read, got 0x%02x", val);
}

ZTEST_F(tac5112, test_clear_errors_clears_all_latches)
{
	/* Interrupts masked, so the latches are populated but no IRQ consumes them. */
	fault_irqs_mask(fixture->dev);
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_INT_LTCH0, 0x81U));
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTM_BIT));
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH2_LTCH,
					     TAC5112_OUT_CH_VG_FAULT_P_BIT));

	zassert_not_equal(tac5112_emul_latch_peek(fault_emul(), TAC5112_REG_INT_LTCH0), 0U);
	zassert_not_equal(tac5112_emul_latch_peek(fault_emul(), TAC5112_REG_OUT_CH1_LTCH), 0U);
	zassert_not_equal(tac5112_emul_latch_peek(fault_emul(), TAC5112_REG_OUT_CH2_LTCH), 0U);

	zassert_ok(audio_codec_clear_errors(fixture->dev));

	zassert_equal(tac5112_emul_latch_peek(fault_emul(), TAC5112_REG_INT_LTCH0), 0U);
	zassert_equal(tac5112_emul_latch_peek(fault_emul(), TAC5112_REG_OUT_CH1_LTCH), 0U);
	zassert_equal(tac5112_emul_latch_peek(fault_emul(), TAC5112_REG_OUT_CH2_LTCH), 0U);
}

ZTEST_F(tac5112, test_clear_errors_releases_irq_line)
{
	fault_arm(fixture->dev);

	/* Hold the scheduler so the work item cannot consume the latch first. */
	k_sched_lock();
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));
	zassert_equal(fault_irq_asserted(), 1, "unmasked latched fault must assert the IRQ");

	zassert_ok(audio_codec_clear_errors(fixture->dev));
	zassert_equal(fault_irq_asserted(), 0, "clear_errors() must release the IRQ line");
	k_sched_unlock();

	/* The queued work item finds nothing left to report. */
	fault_work_drain(fixture->dev);
	zassert_equal(fault_log.count, 0U, "callback fired for an already-cleared fault");
}

ZTEST_F(tac5112, test_fault_irq_releases_after_handling_and_rearms)
{
	fault_arm(fixture->dev);

	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));
	expect_single_callback(fixture->dev, AUDIO_CODEC_ERROR_OVERCURRENT);

	/* The handler's read cleared the latch, which de-asserts the line... */
	zassert_equal(tac5112_emul_latch_peek(fault_emul(), TAC5112_REG_OUT_CH1_LTCH), 0U);
	zassert_equal(fault_irq_asserted(), 0, "IRQ still asserted after latch was read");

	/* ...so a new fault produces a fresh edge and a second callback. */
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH2_LTCH,
					     TAC5112_OUT_CH_VG_FAULT_M_BIT));
	fault_work_drain(fixture->dev);

	zassert_equal(fault_log.count, 2U, "second fault did not re-trigger the callback");
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_DC);
}

ZTEST_F(tac5112, test_fault_masked_sources_do_not_interrupt)
{
	/* Registered, but the fault interrupts are masked. */
	fault_irqs_mask(fixture->dev);
	zassert_ok(audio_codec_register_error_callback(fixture->dev, fault_cb));

	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));
	fault_work_drain(fixture->dev);

	zassert_equal(fault_log.count, 0U, "masked fault raised a callback");
	zassert_equal(fault_irq_asserted(), 0, "masked fault asserted the IRQ line");
}

ZTEST_F(tac5112, test_fault_without_callback_is_consumed_silently)
{
	configure_dut(fixture->dev);
	zassert_ok(audio_codec_register_error_callback(fixture->dev, NULL));

	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));
	fault_work_drain(fixture->dev);

	zassert_equal(fault_log.count, 0U);
	zassert_equal(tac5112_emul_latch_peek(fault_emul(), TAC5112_REG_OUT_CH1_LTCH), 0U,
		      "handler must still read (clear) the latch with no callback registered");
	zassert_equal(fault_irq_asserted(), 0);
}

ZTEST_F(tac5112, test_fault_read_error_is_retried_without_losing_bits)
{
	fault_arm(fixture->dev);

	tac5112_emul_fail_reads(fault_emul(), TAC5112_REG_OUT_CH2_LTCH, 1U);
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));

	expect_single_callback(fixture->dev, AUDIO_CODEC_ERROR_OVERCURRENT);
}

ZTEST_F(tac5112, test_fault_read_error_reports_partial_result_after_fast_retries)
{
	fault_arm(fixture->dev);

	tac5112_emul_fail_reads(fault_emul(), TAC5112_REG_OUT_CH2_LTCH, 0xFFU);
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_VG_FAULT_P_BIT));

	/* The bits that could be read are reported once the fast retries run out. */
	for (int i = 0; (i < 10) && (fault_log.count == 0U); i++) {
		fault_work_drain(fixture->dev);
	}

	zassert_equal(fault_log.count, 1U, "expected the partial result to be reported");
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_DC);
}

ZTEST_F(tac5112, test_fault_unreadable_latch_is_recovered_by_slow_polling)
{
	fault_arm(fixture->dev);

	tac5112_emul_fail_reads(fault_emul(), TAC5112_REG_OUT_CH2_LTCH,
				1U + TAC5112_FAULT_RETRY_MAX);
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH2_LTCH,
					     TAC5112_OUT_CH_SC_OUTM_BIT));

	for (int i = 0; (i < 10) && (fault_log.count == 0U); i++) {
		fault_work_drain(fixture->dev);
	}

	zassert_equal(fault_log.count, 1U, "fault was lost after the fast retries");
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
	zassert_equal(tac5112_emul_latch_peek(fault_emul(), TAC5112_REG_OUT_CH2_LTCH), 0U);
	zassert_equal(fault_irq_asserted(), 0, "IRQ must be released once the latch is read");
}

static void park_retry_after_first_attempt(struct tac5112_data *data)
{
	bool seen = false;

	for (int i = 0; (i < 1000) && !seen; i++) {
		zassert_ok(k_mutex_lock(&data->lock, K_FOREVER));
		seen = (data->fault_retries != 0U);
		if (!seen) {
			k_mutex_unlock(&data->lock);
			k_sleep(K_USEC(100));
		}
	}

	zassert_true(seen, "first worker attempt did not complete");
	zassert_equal(data->fault_retries, 1U);
	zassert_equal(data->fault_pending, AUDIO_CODEC_ERROR_OVERCURRENT,
		      "precondition: partial fault not collected");
}

ZTEST_F(tac5112, test_clear_errors_discards_partially_collected_fault)
{
	struct tac5112_data *data = fixture->dev->data;

	fault_arm(fixture->dev);

	/* CH1 is consumed, CH2's read fails once, so a retry is queued. */
	data->fault_retry_ms = TEST_TAC5112_PARKED_RETRY_MS;
	tac5112_emul_fail_reads(fault_emul(), TAC5112_REG_OUT_CH2_LTCH, 1U);
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));
	park_retry_after_first_attempt(data);

	zassert_ok(audio_codec_clear_errors(fixture->dev));
	zassert_equal(data->fault_pending, 0U);
	k_mutex_unlock(&data->lock);

	fault_work_drain(fixture->dev);
	zassert_equal(fault_log.count, 0U, "stale fault reported after clear_errors()");
}

ZTEST_F(tac5112, test_configure_discards_partially_collected_fault)
{
	struct tac5112_data *data = fixture->dev->data;

	fault_arm(fixture->dev);

	data->fault_retry_ms = TEST_TAC5112_PARKED_RETRY_MS;
	tac5112_emul_fail_reads(fault_emul(), TAC5112_REG_OUT_CH2_LTCH, 1U);
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));
	park_retry_after_first_attempt(data);

	configure_dut(fixture->dev);
	zassert_equal(data->fault_pending, 0U);
	zassert_equal(data->fault_retries, 0U);
	k_mutex_unlock(&data->lock);

	fault_work_drain(fixture->dev);
	zassert_equal(fault_log.count, 0U, "stale fault reported after configure()");
}

ZTEST_F(tac5112, test_fault_after_brownout_is_reported_without_new_irq_edge)
{
	fault_arm(fixture->dev);

	tac5112_emul_set_brownout(fault_emul(), true);
	expect_single_callback(fixture->dev, AUDIO_CODEC_ERROR_UNDERVOLTAGE);
	zassert_equal(fault_irq_asserted(), 1, "live brown-out keeps the IRQ asserted");

	/* The line is already asserted, so this fault produces no new edge. */
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));
	fault_work_drain(fixture->dev);

	zassert_equal(fault_log.count, 2U, "output fault missed while the line stayed asserted");
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_OVERCURRENT,
		      "unchanged brown-out must not be reported again, got 0x%x",
		      fault_log.last_errors);

	/* Once it clears, polling stops and a new brown-out is reported again. */
	tac5112_emul_set_brownout(fault_emul(), false);
	fault_work_drain(fixture->dev);
	tac5112_emul_set_brownout(fault_emul(), true);
	fault_work_drain(fixture->dev);

	zassert_equal(fault_log.count, 3U);
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_UNDERVOLTAGE);
}

ZTEST_F(tac5112, test_fault_after_brownout_and_clear_errors_is_still_reported)
{
	fault_arm(fixture->dev);

	tac5112_emul_set_brownout(fault_emul(), true);
	expect_single_callback(fixture->dev, AUDIO_CODEC_ERROR_UNDERVOLTAGE);

	/* clear_errors() must not stop the recovery polling while the line is up. */
	zassert_ok(audio_codec_clear_errors(fixture->dev));
	zassert_equal(fault_irq_asserted(), 1);

	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH2_LTCH,
					     TAC5112_OUT_CH_VG_FAULT_M_BIT));
	fault_work_drain(fixture->dev);

	zassert_equal(fault_log.count, 2U, "output fault missed after clear_errors()");
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_DC,
		      "unchanged brown-out must not be reported again, got 0x%x",
		      fault_log.last_errors);
}

ZTEST_F(tac5112, test_brownout_reasserted_before_worker_sees_it_clear_is_reported_again)
{
	fault_arm(fixture->dev);

	tac5112_emul_set_brownout(fault_emul(), true);
	expect_single_callback(fixture->dev, AUDIO_CODEC_ERROR_UNDERVOLTAGE);

	k_sched_lock();
	tac5112_emul_set_brownout(fault_emul(), false);
	tac5112_emul_set_brownout(fault_emul(), true);
	k_sched_unlock();

	fault_work_drain(fixture->dev);

	zassert_equal(fault_log.count, 2U, "second brown-out occurrence was suppressed");
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_UNDERVOLTAGE);
}

ZTEST_F(tac5112, test_brownout_raised_during_fault_collection_is_reported_once)
{
	fault_arm(fixture->dev);

	/* While the worker handles an output fault, a brown-out asserts the IRQ
	 * after the latches were read but before the live status is. That edge
	 * is covered by this run's status read, so the follow-up run it queues
	 * must not report the same brown-out again.
	 */
	tac5112_emul_brownout_after_read(fault_emul(), TAC5112_REG_OUT_CH2_LTCH);
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));

	expect_single_callback(fixture->dev,
			       AUDIO_CODEC_ERROR_OVERCURRENT | AUDIO_CODEC_ERROR_UNDERVOLTAGE);
}

ZTEST_F(tac5112, test_failed_rollback_marks_channel_for_resync)
{
	const struct emul *emul = EMUL_DT_GET(TAC5112_EMUL_NODE);
	const struct device *dev = fixture->dev;
	audio_property_value_t val;
	uint8_t reg;

	configure_dut(dev);

	val.mute = true;
	zassert_ok(audio_codec_set_property(dev, AUDIO_PROPERTY_OUTPUT_MUTE,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));

	/* Unmute: A is written, B fails, and restoring A fails too. */
	tac5112_emul_fail_writes_after(emul, TAC5112_REG_DAC_CH2A_CFG0, 1U);
	tac5112_emul_fail_writes(emul, TAC5112_REG_DAC_CH2B_CFG0, true);
	val.mute = false;
	zassert_not_equal(audio_codec_set_property(dev, AUDIO_PROPERTY_OUTPUT_MUTE,
						   AUDIO_CHANNEL_FRONT_RIGHT, val),
			  0);
	tac5112_emul_clear_failures(emul);

	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_not_equal(reg, TAC5112_DVOL_MUTE, "precondition: A was left unmuted");

	/* The cache still says muted. A volume update must resynchronise the
	 * codec to the muted state instead of returning from the cache alone.
	 */
	val.vol = TEST_TAC5112_DVOL_DB;
	zassert_ok(audio_codec_set_property(dev, AUDIO_PROPERTY_OUTPUT_VOLUME,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));
	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DVOL_MUTE, "A not resynchronised to muted");
	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_DAC_CH2B_CFG0, &reg));
	zassert_equal(reg, TAC5112_DVOL_MUTE, "B not resynchronised to muted");

	/* Unmute restores the volume requested while muted, on both registers. */
	val.mute = false;
	zassert_ok(audio_codec_set_property(dev, AUDIO_PROPERTY_OUTPUT_MUTE,
					    AUDIO_CHANNEL_FRONT_RIGHT, val));
	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_DAC_CH2A_CFG0, &reg));
	zassert_equal(reg, TAC5112_DAC_DVOL_0DB + TEST_TAC5112_DVOL_DB);
	zassert_ok(tac5112_reg_read(dev, TAC5112_REG_DAC_CH2B_CFG0, &reg));
	zassert_equal(reg, TAC5112_DAC_DVOL_0DB + TEST_TAC5112_DVOL_DB);
}

ZTEST_F(tac5112, test_brownout_raised_while_status_is_sampled_is_reported_once)
{
	fault_arm(fixture->dev);

	/* The brown-out asserts the IRQ after the worker has decided to read the
	 * live status but before the I2C read samples it: the sample sees the
	 * status set, and the edge it raised must not count as a second
	 * occurrence on the follow-up run.
	 */
	tac5112_emul_brownout_before_read(fault_emul(), TAC5112_REG_AVDD_IOVDD_STS);
	zassert_ok(tac5112_emul_inject_latch(fault_emul(), TAC5112_REG_OUT_CH1_LTCH,
					     TAC5112_OUT_CH_SC_OUTP_BIT));

	expect_single_callback(fixture->dev,
			       AUDIO_CODEC_ERROR_OVERCURRENT | AUDIO_CODEC_ERROR_UNDERVOLTAGE);
}

ZTEST_F(tac5112, test_second_brownout_after_a_reported_one_is_not_lost_during_status_read)
{
	fault_arm(fixture->dev);

	tac5112_emul_set_brownout(fault_emul(), true);
	expect_single_callback(fixture->dev, AUDIO_CODEC_ERROR_UNDERVOLTAGE);

	/* The brown-out ends and returns while the next poll reads the status:
	 * the sample shows it set, which is the same level as the reported
	 * occurrence, but the IRQ saw a clear and a new assertion. That is a
	 * second occurrence and must be reported by the follow-up run.
	 */
	tac5112_emul_brownout_cycle_before_read(fault_emul(), TAC5112_REG_AVDD_IOVDD_STS);
	fault_work_drain(fixture->dev);

	zassert_equal(fault_log.count, 2U, "second brown-out occurrence was lost");
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_UNDERVOLTAGE);

	/* And it is reported once, not on every further poll. */
	fault_work_drain(fixture->dev);
	zassert_equal(fault_log.count, 2U, "unchanged brown-out reported again");
}

ZTEST_F(tac5112, test_brownout_cycling_after_the_status_sample_is_not_lost)
{
	fault_arm(fixture->dev);

	/* The status is sampled set, then the brown-out ends and returns before
	 * the worker acts on the sample. The extra assertion must not be
	 * acknowledged against the earlier sample: the status is read again, so
	 * the condition that is present is reported once and nothing is left
	 * unaccounted for.
	 */
	tac5112_emul_brownout_cycle_after_read(fault_emul(), TAC5112_REG_AVDD_IOVDD_STS);
	tac5112_emul_set_brownout(fault_emul(), true);
	expect_single_callback(fixture->dev, AUDIO_CODEC_ERROR_UNDERVOLTAGE);

	/* No pending assertion remains, so further polls stay quiet... */
	fault_work_drain(fixture->dev);
	fault_work_drain(fixture->dev);
	zassert_equal(fault_log.count, 1U, "unchanged brown-out reported again");

	/* ...and a genuinely separate later occurrence is still reported. */
	tac5112_emul_set_brownout(fault_emul(), false);
	fault_work_drain(fixture->dev);
	tac5112_emul_set_brownout(fault_emul(), true);
	fault_work_drain(fixture->dev);
	zassert_equal(fault_log.count, 2U, "later brown-out occurrence was lost");
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_UNDERVOLTAGE);
}

ZTEST_F(tac5112, test_brownout_sampled_then_cleared_by_a_new_fault_is_still_reported)
{
	fault_arm(fixture->dev);

	/* The first status sample sees an active brown-out. It then clears and an
	 * output fault asserts a new IRQ edge before the worker checks the edge
	 * count, so the status is sampled again and reads clear. The brown-out
	 * that was seen must still be reported, and the new fault follows.
	 */
	tac5112_emul_clear_brownout_and_fault_after_read(fault_emul(), TAC5112_REG_AVDD_IOVDD_STS,
							 TAC5112_REG_OUT_CH1_LTCH,
							 TAC5112_OUT_CH_SC_OUTP_BIT);
	tac5112_emul_set_brownout(fault_emul(), true);
	fault_work_drain(fixture->dev);
	fault_work_drain(fixture->dev);

	zassert_equal(fault_log.all_errors,
		      AUDIO_CODEC_ERROR_UNDERVOLTAGE | AUDIO_CODEC_ERROR_OVERCURRENT,
		      "got 0x%x: a sampled brown-out or the new fault was lost",
		      fault_log.all_errors);
	zassert_equal(fault_log.count, 2U);
	zassert_equal(fault_log.last_errors, AUDIO_CODEC_ERROR_OVERCURRENT);
}
