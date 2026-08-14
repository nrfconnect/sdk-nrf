/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Sample application for the Texas Instruments TAC5112 stereo audio codec,
 * driven through the standard Zephyr audio codec interface
 * (<zephyr/audio/codec.h>). It targets a Nordic nRF54LM20 DK controlling
 * the codec over I2C; the codec is selected from the devicetree by
 * compatible, so the same source runs unmodified against the software I2C
 * mock on native_sim.
 *
 * The sample walks the codec control surface the driver implements:
 *   1. register a fault callback,
 *   2. configure the audio serial interface (I2S target, 48 kHz, stereo,
 *      16-bit) -- this also brings the codec out of reset/sleep and runs
 *      the recommended automatic PLL/clock configuration,
 *   3. set the output volume and unmute,
 *   4. power up the DAC (start_output),
 *   5. optionally stream a 1 kHz test tone over I2S (see TONE section),
 *   6. demonstrate a volume ramp and a mute/unmute cycle,
 *   7. power down (stop_output).
 *
 * Only the public codec API is used -- the private driver header
 * (tac5112.h) is intentionally NOT included.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tac5112_sample, LOG_LEVEL_INF);

#if !DT_HAS_COMPAT_STATUS_OKAY(ti_tac5112)
#error "No enabled ti,tac5112 node in the devicetree. " \
		"Add the board overlay (see boards/ and README.rst)."
#endif

/* Pick the (single) enabled TAC5112 straight from the devicetree. */
#define CODEC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(ti_tac5112)

/* The codec/driver expresses volume in 0.5 dB steps (val.vol == dB * 2). */
#define HALF_DB(db) ((int)((db) * 2))

/* Audio serial interface parameters shared by the codec and (optionally)
 * the I2S peripheral feeding it.
 */
#define AUDIO_SAMPLE_RATE_HZ 48000U
#define AUDIO_WORD_BITS_16   16U
#define AUDIO_WORD_BITS_24   24U
#define AUDIO_CHANNELS	     2U

static const struct device *const codec = DEVICE_DT_GET(CODEC_NODE);

/*
 * ---------------------------------------------------------------------
 * Fault handling
 * ---------------------------------------------------------------------
 */
static void codec_fault_handler(const struct device *dev, uint32_t errors)
{
	/* Runs from the driver's workqueue context when the codec asserts its
	 * fault/IRQ line. On the nRF54LM20 DK that line is the codec's GPIO1,
	 * wired via EXP D2 -> P3.02 (fault-gpios in the overlay); the driver
	 * enables the output short-circuit / virtual-ground fault interrupts
	 * in audio_codec_configure().
	 */
	LOG_ERR("codec reported fault(s): 0x%08x", errors);

	if (errors & AUDIO_CODEC_ERROR_OVERCURRENT) {
		LOG_ERR("  - output over-current / short circuit");
	}

	if (errors & AUDIO_CODEC_ERROR_DC) {
		LOG_ERR("  - output DC / virtual-ground fault");
	}

	if (errors & AUDIO_CODEC_ERROR_UNDERVOLTAGE) {
		LOG_ERR("  - supply under-voltage / brown-out");
	}

	/* Acknowledge the fault: read-clears the codec's latched fault
	 * registers so it de-asserts the IRQ line and can flag the next event.
	 * Safe to call here -- the driver invokes this callback without holding
	 * its internal lock.
	 */
	(void)audio_codec_clear_errors(dev);
}

/*
 * ---------------------------------------------------------------------
 * Codec setup
 * ---------------------------------------------------------------------
 */
static int codec_setup(void)
{
	struct audio_codec_cfg cfg = {0};
	int ret;

	/* ASI target mode: the host (nRF I2S or the mock) drives BCLK/FSYNC. */
	cfg.mclk_freq = 0U;
	cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	cfg.dai_cfg.i2s.word_size = AUDIO_WORD_BITS_16;
	cfg.dai_cfg.i2s.channels = AUDIO_CHANNELS;
	cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	cfg.dai_cfg.i2s.frame_clk_freq = AUDIO_SAMPLE_RATE_HZ;

	ret = audio_codec_configure(codec, &cfg);
	if (ret != 0) {
		LOG_ERR("audio_codec_configure() failed (%d)", ret);
		return ret;
	}

	LOG_INF("codec configured: I2S target, %u Hz, %u-bit, %u ch", AUDIO_SAMPLE_RATE_HZ,
		AUDIO_WORD_BITS_16, AUDIO_CHANNELS);
	return 0;
}

static int codec_set_volume(int half_db)
{
	audio_property_value_t val;
	int ret;

	val.vol = half_db;
	ret = audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_VOLUME, AUDIO_CHANNEL_ALL, val);
	if (ret != 0) {
		LOG_ERR("set output volume %d (0.5 dB) failed (%d)", half_db, ret);
		return ret;
	}

	return audio_codec_apply_properties(codec);
}

static int codec_set_mute(bool mute)
{
	audio_property_value_t val;
	int ret;

	val.mute = mute;
	ret = audio_codec_set_property(codec, AUDIO_PROPERTY_OUTPUT_MUTE, AUDIO_CHANNEL_ALL, val);
	if (ret != 0) {
		LOG_ERR("%s failed (%d)", mute ? "mute" : "unmute", ret);
		return ret;
	}

	return audio_codec_apply_properties(codec);
}

/*
 * ---------------------------------------------------------------------
 * Optional: stream a 1 kHz test tone over I2S.
 *
 * Enabled only when an "tac5112-i2s" devicetree alias points at the I2S
 * peripheral wired to the codec (BCLK/FSYNC/DIN). The nRF I2S acts as the
 * clock controller (master); the codec is the ASI target, matching
 * codec_setup() above. If the alias is absent this whole section compiles
 * out and the sample runs the control-only demo.
 * ---------------------------------------------------------------------
 */
#if DT_NODE_EXISTS(DT_ALIAS(tac5112_i2s))

#include <math.h>

#define I2S_TX_NODE	  DT_ALIAS(tac5112_i2s)
#define TONE_FREQ_HZ	  1000U
#define SAMPLES_PER_BLOCK (AUDIO_SAMPLE_RATE_HZ / 100U) /* 10 ms of frames */
#define BLOCK_BYTES_16	  (SAMPLES_PER_BLOCK * AUDIO_CHANNELS * (AUDIO_WORD_BITS_16 / 8U))
#define BLOCK_BYTES_24	  (SAMPLES_PER_BLOCK * AUDIO_CHANNELS * (AUDIO_WORD_BITS_24 / 8U))
#define BLOCK_COUNT	  8U
#define TONE_BLOCKS	  100U /* ~1 second */
#define PRIME_BLOCKS	  3U

K_MEM_SLAB_DEFINE_STATIC(tone_slab, BLOCK_BYTES_16, BLOCK_COUNT, 4);
K_MEM_SLAB_DEFINE_STATIC(loopback_slab, BLOCK_BYTES_24, BLOCK_COUNT, 4);

static int16_t tone_block[SAMPLES_PER_BLOCK * AUDIO_CHANNELS];

static void tone_block_init(void)
{
	for (uint32_t i = 0; i < SAMPLES_PER_BLOCK; i++) {
		double t = (double)i / (double)AUDIO_SAMPLE_RATE_HZ;
		int16_t s =
			(int16_t)(0.25 * 32767.0 * sin(2.0 * 3.14159265358979 * TONE_FREQ_HZ * t));

		tone_block[2U * i] = s;	     /* left  */
		tone_block[2U * i + 1U] = s; /* right */
	}
}

static void fill_tone_block(int16_t *buf)
{
	for (uint32_t i = 0; i < SAMPLES_PER_BLOCK * AUDIO_CHANNELS; i++) {
		buf[i] = tone_block[i];
	}
}

static int play_test_tone(void)
{
	const struct device *i2s_dev = DEVICE_DT_GET(I2S_TX_NODE);
	struct i2s_config i2s_cfg = {
		.word_size = AUDIO_WORD_BITS_16,
		.channels = AUDIO_CHANNELS,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER,
		.frame_clk_freq = AUDIO_SAMPLE_RATE_HZ,
		.mem_slab = &tone_slab,
		.block_size = BLOCK_BYTES_16,
		.timeout = 2000,
	};
	bool started = false;
	int ret;

	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("I2S device %s not ready", i2s_dev->name);
		return -ENODEV;
	}

	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("i2s_configure() failed (%d)", ret);
		return ret;
	}

	tone_block_init();
	LOG_INF("streaming %u Hz tone for ~1 s", TONE_FREQ_HZ);

	for (uint32_t b = 0; b < TONE_BLOCKS; b++) {
		void *block;

		ret = k_mem_slab_alloc(&tone_slab, &block, K_MSEC(200));
		if (ret != 0) {
			LOG_ERR("tone slab alloc failed (%d)", ret);
			return ret;
		}

		fill_tone_block((int16_t *)block);

		ret = i2s_write(i2s_dev, block, BLOCK_BYTES_16);
		if (ret != 0) {
			LOG_ERR("i2s_write() failed (%d)", ret);
			k_mem_slab_free(&tone_slab, block);
			return ret;
		}

		/* Start only once several blocks are queued, so the DMA always
		 * has data ready and never underruns into the ERROR state.
		 */
		if (!started && (b == PRIME_BLOCKS - 1U)) {
			ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
			if (ret != 0) {
				LOG_ERR("I2S start trigger failed (%d)", ret);
				return ret;
			}
			started = true;
		}
	}

	if (!started) {
		ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
		if (ret != 0) {
			LOG_ERR("I2S start trigger failed (%d)", ret);
			return ret;
		}
	}

	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	if (ret != 0) {
		LOG_ERR("I2S drain trigger failed (%d)", ret);
	}

	k_msleep((BLOCK_COUNT * 10U) + 50U); /* block period is 10 ms */

	return ret;
}

/* Duration of the ADC -> DAC loopback demo. */
#define LOOPBACK_SECONDS 10U
#define LOOPBACK_BLOCKS	 (LOOPBACK_SECONDS * 100U) /* 10 ms blocks */

/**
 * @brief Exercise output volume and mute on the *live* passthrough audio,
 * scheduled by block index (100 blocks = 1 s):
 *   0-2 s : 0 dB          4-6 s : ramp -40 -> 0 dB
 *   2-4 s : ramp 0->-40dB 6-8 s : muted        8-10 s : unmuted, 0 dB
 *
 * Writes to the codec over I2C only when the target actually changes, so it
 * never delays the TDM read/write cadence enough to under/overrun.
 */
static void loopback_demo_step(uint32_t b)
{
	static int last_vol_db = 999; /* impossible dB -> forces first write */
	static int last_mute = -1;
	int vol_db = 0;
	bool mute = false;

	if (b < 200U) { /* 0-2 s: full-volume passthrough */
		vol_db = 0;
	} else if (b < 400U) { /* 2-4 s: ramp down to -40 dB */
		vol_db = -((int)(b - 200U) * 40 / 200);
	} else if (b < 600U) { /* 4-6 s: ramp back up to 0 dB */
		vol_db = -40 + (int)(b - 400U) * 40 / 200;
	} else if (b < 800U) { /* 6-8 s: muted */
		mute = true;
	} else { /* 8-10 s: unmuted, 0 dB */
		vol_db = 0;
	}

	if ((int)mute != last_mute) {
		(void)codec_set_mute(mute);
		LOG_INF("loopback: %s", mute ? "mute" : "unmute");
		last_mute = mute;
	}

	if (!mute && (vol_db != last_vol_db)) {
		(void)codec_set_volume(HALF_DB(vol_db));
		if ((vol_db % 10) == 0) {
			LOG_INF("loopback: output volume %d dB", vol_db);
		}
		last_vol_db = vol_db;
	}
}

/**
 * @brief Full-duplex passthrough: capture from the codec ADC over the TDM
 * RX line (SDIN) and send it straight back to the codec DAC over the TDM
 * TX line (SDOUT), sharing one BCLK/FSYNC.
 *
 * Whatever analog signal is present on the codec inputs (line-in / mic on
 * the EB) is digitised, looped through the nRF, and played back on the
 * codec outputs. The codec record path is already powered by
 * codec_configure() (ADC_PDZ + input channels enabled), and the DAC by
 * audio_codec_start_output().
 */
static int run_loopback(void)
{
	const struct device *i2s_dev = DEVICE_DT_GET(I2S_TX_NODE);
	struct i2s_config cfg = {
		.word_size = AUDIO_WORD_BITS_24,
		.channels = AUDIO_CHANNELS,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER,
		.frame_clk_freq = AUDIO_SAMPLE_RATE_HZ,
		.mem_slab = &loopback_slab,
		.block_size = BLOCK_BYTES_24,
		.timeout = 2000,
	};
	int ret;

	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("I2S device %s not ready", i2s_dev->name);
		return -ENODEV;
	}

	ret = i2s_configure(i2s_dev, I2S_DIR_BOTH, &cfg);
	if (ret != 0) {
		LOG_ERR("i2s_configure(BOTH) failed (%d)", ret);
		return ret;
	}

	for (uint32_t i = 0; i < PRIME_BLOCKS; i++) {
		void *block;

		ret = k_mem_slab_alloc(&loopback_slab, &block, K_MSEC(200));
		if (ret != 0) {
			LOG_ERR("loopback prime alloc failed (%d)", ret);
			return ret;
		}

		for (uint32_t j = 0; j < SAMPLES_PER_BLOCK * AUDIO_CHANNELS; j++) {
			((int16_t *)block)[j] = 0;
		}

		ret = i2s_write(i2s_dev, block, BLOCK_BYTES_16);
		if (ret != 0) {
			LOG_ERR("loopback prime write failed (%d)", ret);
			k_mem_slab_free(&tone_slab, block);
			return ret;
		}
	}

	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
	if (ret != 0) {
		LOG_ERR("loopback START failed (%d)", ret);
		return ret;
	}

	LOG_INF("ADC->DAC loopback for %u s (varying output volume + mute)", LOOPBACK_SECONDS);

	for (uint32_t b = 0; b < LOOPBACK_BLOCKS; b++) {
		void *block;
		size_t size;

		ret = i2s_read(i2s_dev, &block, &size);
		if (ret != 0) {
			LOG_ERR("i2s_read() failed (%d)", ret);
			break;
		}

		ret = i2s_write(i2s_dev, block, size);
		if (ret != 0) {
			LOG_ERR("i2s_write() failed (%d)", ret);
			k_mem_slab_free(&loopback_slab, block);
			break;
		}

		loopback_demo_step(b);
	}

	(void)i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);

	return ret;
}

#else /* no I2S alias */

static int play_test_tone(void)
{
	LOG_INF("no tac5112-i2s alias in devicetree; skipping tone playback "
		"(control-only demo)");
	return 0;
}

static int run_loopback(void)
{
	LOG_INF("no tac5112-i2s alias in devicetree; skipping ADC->DAC loopback");
	return 0;
}

#endif /* DT_NODE_EXISTS(DT_ALIAS(tac5112_i2s)) */

/*
 * ---------------------------------------------------------------------
 * Demo sequence
 * ---------------------------------------------------------------------
 */
int main(void)
{
	int ret;

	LOG_INF("TAC5112 audio codec sample");

	if (!device_is_ready(codec)) {
		LOG_ERR("codec device %s not ready", codec->name);
		return 0;
	}

	ret = audio_codec_register_error_callback(codec, codec_fault_handler);
	if (ret != 0) {
		LOG_WRN("register_error_callback() returned %d", ret);
	}

	if (codec_setup() != 0) {
		return 0;
	}

	if (codec_set_volume(HALF_DB(0)) != 0 || codec_set_mute(false) != 0) {
		return 0;
	}

	audio_codec_start_output(codec);
	LOG_INF("output enabled");

	(void)play_test_tone();

	(void)run_loopback();

#if !DT_NODE_EXISTS(DT_ALIAS(tac5112_i2s))
	LOG_INF("volume ramp down/up");
	for (int db = 0; db >= -20; db -= 2) {
		(void)codec_set_volume(HALF_DB(db));
		k_msleep(100);
	}

	for (int db = -20; db <= 0; db += 2) {
		(void)codec_set_volume(HALF_DB(db));
		k_msleep(100);
	}

	LOG_INF("mute");
	(void)codec_set_mute(true);
	k_msleep(500);

	LOG_INF("unmute");
	(void)codec_set_mute(false);
	k_msleep(500);
#endif

	audio_codec_stop_output(codec);
	LOG_INF("output disabled");

	(void)audio_codec_clear_errors(codec);

	LOG_INF("TAC5112 sample complete");
	return 0;
}
