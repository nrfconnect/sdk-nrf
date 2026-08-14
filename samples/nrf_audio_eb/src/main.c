/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Exercises whatever the shield brings up on the current carrier. Sections that
 * a given board's overlay does not provide are skipped at compile time, so the
 * same source builds on:
 *   - nRF54LM20 DK       : TDM audio, I2C codec, PDM, SD card, RGB LED, line-in
 *   - nRF5340 Audio DK (P10): I2S audio, I2C codec (+ reset/irq); codec only
 *   - nRF5340 DK (P3/P4)    : I2S audio, I2C codec (+ reset/irq); codec only
 *
 * It verifies wiring/bring-up, not audio quality. With no TAC5112 driver the
 * audio step only proves the serial port clocks out; audible output needs codec
 * register setup over I2C.
 */

#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/fs/fs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf_audio_eb, CONFIG_NRF_AUDIO_EB_LOG_LEVEL);

#if !DT_NODE_EXISTS(DT_ALIAS(tac5112_i2s)) || !DT_NODE_EXISTS(DT_NODELABEL(tac5112))
#error "Build with -DSHIELD=nrf_audio_eb (shield devicetree not found)."
#endif

#define CODEC_NODE DT_NODELABEL(tac5112)

#if DT_NODE_EXISTS(DT_NODELABEL(led_rgb_red))
static const struct gpio_dt_spec led_r = GPIO_DT_SPEC_GET(DT_NODELABEL(led_rgb_red), gpios);
static const struct gpio_dt_spec led_g = GPIO_DT_SPEC_GET(DT_NODELABEL(led_rgb_green), gpios);
static const struct gpio_dt_spec led_b = GPIO_DT_SPEC_GET(DT_NODELABEL(led_rgb_blue), gpios);
static const char * const names[] = {"red", "green", "blue"};

static void test_leds(void)
{
	const struct gpio_dt_spec *leds[] = {&led_r, &led_g, &led_b};

	LOG_INF("[LED]  RGB blink");

	for (int i = 0; i < 3; i++) {
		if (!gpio_is_ready_dt(leds[i])) {
			LOG_WRN("  %-5s : port not ready", names[i]);
			continue;
		}

		gpio_pin_configure_dt(leds[i], GPIO_OUTPUT_INACTIVE);
		LOG_INF("  %-5s : on", names[i]);
		gpio_pin_set_dt(leds[i], 1);
		k_msleep(500);
		gpio_pin_set_dt(leds[i], 0);
	}
}
#else
static void test_leds(void)
{
	LOG_WRN("[LED]  not on this board -- skipped");
}
#endif /* DT_NODE_EXISTS(DT_ALIAS(led_rgb_red)) */

#if DT_NODE_EXISTS(DT_NODELABEL(line_in_detect))
static const struct gpio_dt_spec line_in = GPIO_DT_SPEC_GET(DT_NODELABEL(line_in_detect), gpios);

static void test_line_in(void)
{
	int v;

	LOG_INF("[LINE] line-in-detect");

	if (!gpio_is_ready_dt(&line_in)) {
		LOG_ERR("  port not ready");
		return;
	}

	gpio_pin_configure_dt(&line_in, GPIO_INPUT);
	v = gpio_pin_get_dt(&line_in);

	LOG_INF("  logical level = %d (%s)", v, v ? "asserted" : "deasserted");
}
#else
static void test_line_in(void)
{
	LOG_WRN("[LINE] not on this board -- skipped");
}
#endif /* DT_NODE_EXISTS(DT_ALIAS(led_rgb_red)) */

#if DT_ON_BUS(CODEC_NODE, i2c)
static const struct i2c_dt_spec codec_i2c = I2C_DT_SPEC_GET(CODEC_NODE);
#if DT_NODE_HAS_PROP(CODEC_NODE, reset_gpios)
static const struct gpio_dt_spec codec_reset = GPIO_DT_SPEC_GET(CODEC_NODE, reset_gpios);
#endif

static void test_codec(void)
{
	int ret;
	uint8_t reg = 0x00, val = 0xff;

	LOG_INF("[I2C]  TAC5112 probe @0x%02x", codec_i2c.addr);

	if (!device_is_ready(codec_i2c.bus)) {
		LOG_ERR("  bus not ready");
		return;
	}

#if DT_NODE_HAS_PROP(CODEC_NODE, reset_gpios)
	if (gpio_is_ready_dt(&codec_reset)) {
		gpio_pin_configure_dt(&codec_reset, GPIO_OUTPUT_ACTIVE);
		k_msleep(1);
		gpio_pin_set_dt(&codec_reset, 0);
		k_msleep(2);
	}
#endif

	ret = i2c_write_read_dt(&codec_i2c, &reg, 1, &val, 1);
	if (ret == 0) {
		LOG_INF("  ACK: codec present (page reg = 0x%02x)", val);
	} else {
		LOG_ERR("  no ACK (ret=%d): check ADDR strap / power / wiring", ret);
	}
}
#elif DT_ON_BUS(CODEC_NODE, spi)
static const struct spi_dt_spec codec_spi =
	SPI_DT_SPEC_GET(CODEC_NODE, SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA, 0);
#if DT_NODE_HAS_PROP(CODEC_NODE, reset_gpios)
static const struct gpio_dt_spec codec_reset = GPIO_DT_SPEC_GET(CODEC_NODE, reset_gpios);
#endif

static void test_codec(void)
{
	int ret;
	uint8_t tx[2] = {(0 << 1) | 0x01, 0x00};
	uint8_t rx[2] = {0};
	const struct spi_buf txb = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf rxb = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set txs = {.buffers = &txb, .count = 1};
	const struct spi_buf_set rxs = {.buffers = &rxb, .count = 1};

	LOG_INF("[SPI]  TAC5112 probe (SPI mode 1)");
	if (!spi_is_ready_dt(&codec_spi)) {
		LOG_ERR("  bus not ready");
		return;
	}

#if DT_NODE_HAS_PROP(CODEC_NODE, reset_gpios)
	if (gpio_is_ready_dt(&codec_reset)) {
		gpio_pin_configure_dt(&codec_reset, GPIO_OUTPUT_ACTIVE);
		k_msleep(1);
		gpio_pin_set_dt(&codec_reset, 0);
		k_msleep(2);
	}
#endif

	ret = spi_transceive_dt(&codec_spi, &txs, &rxs);
	if (ret == 0) {
		LOG_INF("  SPI ok: PAGE reg = 0x%02x (expect 0x00 after reset)", rx[1]);
	} else {
		LOG_ERR("  SPI transceive failed (ret=%d)", ret);
	}
}
#else
static void test_codec(void)
{
	LOG_WRN("[CODEC] unknown control bus -- skipped");
}
#endif /* DT_ON_BUS(CODEC_NODE, i2c/spi)) */

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_sdmmc_disk)
#if defined(CONFIG_FAT_FILESYSTEM_ELM)

#include <ff.h>

#if defined(CONFIG_DISK_DRIVER_MMC)
#define DISK_DRIVE_NAME "SD2"
#else
#define DISK_DRIVE_NAME "SD"
#endif

#define DISK_MOUNT_PT "/" DISK_DRIVE_NAME ":"

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t sd_mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = DISK_MOUNT_PT,
};

#elif defined(CONFIG_FILE_SYSTEM_EXT2)

#include <zephyr/fs/ext2.h>

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT	"/ext"

static struct fs_mount_t sd_mp = {
	.type = FS_EXT2,
	.flags = FS_MOUNT_FLAG_NO_FORMAT,
	.storage_dev = (void *)DISK_DRIVE_NAME,
	.mnt_point = "/ext",
};
#endif

static void test_sdcard(void)
{
	int ret;
	struct fs_statvfs st;
	struct fs_dir_t dir;

	LOG_INF("[SD]   mount %s (SPI mode, 1-bit)", sd_mp.mnt_point);

	ret = fs_mount(&sd_mp);
	if (ret) {
		LOG_ERR("  fs_mount failed (ret = %d): card inserted/formatted?", ret);
		return;
	}

	if (fs_statvfs(sd_mp.mnt_point, &st) == 0) {
		unsigned long total_kb =
			(unsigned long)((uint64_t)st.f_blocks * st.f_frsize / 1024);
		unsigned long free_kb = (unsigned long)((uint64_t)st.f_bfree * st.f_frsize / 1024);

		LOG_INF("  mounted: %lu KB total, %lu KB free", total_kb, free_kb);
	}

	fs_dir_t_init(&dir);

	if (fs_opendir(&dir, sd_mp.mnt_point) == 0) {
		struct fs_dirent ent;
		int n = 0;

		while (fs_readdir(&dir, &ent) == 0 && ent.name[0] != '\0') {
			LOG_INF("   %s %s (%zu)", ent.type == FS_DIR_ENTRY_DIR ? "[D]" : "[F]",
				ent.name, ent.size);
			n++;
		}

		LOG_INF("  %d root entries", n);

		fs_closedir(&dir);
	}

	fs_unmount(&sd_mp);
}
#else
static void test_sdcard(void)
{
	LOG_WRN("[SD]   not on this board -- skipped");
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(zephyr_sdmmc_disk) */

#if DT_NODE_EXISTS(DT_ALIAS(dmic0))
#define PDM_RATE	16000
#define PDM_BLOCK_MS	100
#define PDM_BLOCK_SIZE	((PDM_RATE * 2 * PDM_BLOCK_MS) / 1000)
#define PDM_BLOCK_COUNT 4

K_MEM_SLAB_DEFINE_STATIC(pdm_slab, PDM_BLOCK_SIZE, PDM_BLOCK_COUNT, 4);
static const struct device *const dmic = DEVICE_DT_GET(DT_ALIAS(dmic0));

static void test_pdm(void)
{
	int ret;
	void *buf;
	size_t size;
	struct pcm_stream_cfg stream = {
		.pcm_width = 16,
		.pcm_rate = PDM_RATE,
		.block_size = PDM_BLOCK_SIZE,
		.mem_slab = &pdm_slab,
	};
	struct dmic_cfg cfg = {
		.io = {
				.min_pdm_clk_freq = 1000000,
				.max_pdm_clk_freq = 3500000,
				.min_pdm_clk_dc = 40,
				.max_pdm_clk_dc = 60,
			},
		.streams = &stream,
		.channel = {
				.req_num_streams = 1,
				.req_num_chan = 1,
				.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT),
			},
	};

	LOG_INF("[PDM]  microphone capture");
	if (!device_is_ready(dmic)) {
		LOG_ERR("  device not ready");
		return;
	}

	ret = dmic_configure(dmic, &cfg);
	if (ret) {
		LOG_ERR("  configure failed (ret=%d)", ret);
		return;
	}

	ret = dmic_trigger(dmic, DMIC_TRIGGER_START);
	if (ret) {
		LOG_ERR("  start failed (ret=%d)", ret);
		return;
	}

	ret = dmic_read(dmic, 0, &buf, &size, 1000);
	if (ret == 0) {
		int16_t *s = buf;
		size_t n = size / sizeof(int16_t);
		int32_t peak = 0;

		for (size_t i = 0; i < n; i++) {
			int32_t a = s[i] < 0 ? -s[i] : s[i];

			if (a > peak) {
				peak = a;
			}
		}

		LOG_INF("  captured %zu bytes, peak amplitude = %d", size, peak);
		k_mem_slab_free(&pdm_slab, buf);
	} else {
		LOG_WRN("  read failed (ret=%d)", ret);
	}

	dmic_trigger(dmic, DMIC_TRIGGER_STOP);
}
#else
static void test_pdm(void)
{
	LOG_WRN("[PDM]  not on this board -- skipped");
}
#endif /* DT_NODE_EXISTS(DT_ALIAS(dmic0)) */

#if DT_NODE_EXISTS(DT_ALIAS(tac5112_i2s))
#define AUDIO_WORD_BITS	     16
#define AUDIO_SAMPLE_RATE_HZ 48000
#define AUDIO_CHANNELS	     2
#define I2S_SAMPLES	     128
#define I2S_BLOCK_SIZE	     (I2S_SAMPLES * AUDIO_CHANNELS * sizeof(int16_t))
#define I2S_BLOCK_COUNT	     4
#define I2S_TIMEOUT	     1000

K_MEM_SLAB_DEFINE_STATIC(i2s_slab, I2S_BLOCK_SIZE, I2S_BLOCK_COUNT, 4);
static const struct device *const codec = DEVICE_DT_GET(CODEC_NODE);

#define HALF_DB(db)	  ((int)((db) * 2))
#define I2S_TX_NODE	  DT_ALIAS(tac5112_i2s)
#define TONE_FREQ_HZ	  1000U
#define SAMPLES_PER_BLOCK (AUDIO_SAMPLE_RATE_HZ / 100U)
#define BLOCK_BYTES	  (SAMPLES_PER_BLOCK * AUDIO_CHANNELS * (AUDIO_WORD_BITS / 8U))
#define BLOCK_COUNT	  8U
#define TONE_BLOCKS	  100U
#define PRIME_BLOCKS	  3U

K_MEM_SLAB_DEFINE_STATIC(tone_slab, BLOCK_BYTES, BLOCK_COUNT, 4);
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
		.word_size = AUDIO_WORD_BITS,
		.channels = AUDIO_CHANNELS,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER,
		.frame_clk_freq = AUDIO_SAMPLE_RATE_HZ,
		.mem_slab = &tone_slab,
		.block_size = BLOCK_BYTES,
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
	LOG_INF("  streaming %u Hz tone for ~1 s", TONE_FREQ_HZ);

	for (uint32_t b = 0; b < TONE_BLOCKS; b++) {
		void *block;

		ret = k_mem_slab_alloc(&tone_slab, &block, K_MSEC(200));
		if (ret != 0) {
			LOG_ERR("tone slab alloc failed (%d)", ret);
			return ret;
		}

		fill_tone_block((int16_t *)block);

		ret = i2s_write(i2s_dev, block, BLOCK_BYTES);
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

	/* Tone shorter than the prime count: start what we queued. */
	if (!started) {
		ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START);
		if (ret != 0) {
			LOG_ERR("I2S start trigger failed (%d)", ret);
			return ret;
		}
	}

	/* Let the queued blocks drain out of the peripheral. */
	ret = i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	if (ret != 0) {
		LOG_ERR("I2S drain trigger failed (%d)", ret);
	}

	/* DRAIN is asynchronous: the TDM stays in the STOPPING state until the
	 * last queued block finishes transmitting. Wait long enough for every
	 * in-flight block to play out and the peripheral to return to READY,
	 * so a following i2s_configure() (e.g. the loopback) isn't rejected
	 * with -EINVAL ("Cannot configure in state: 3").
	 */
	k_msleep((BLOCK_COUNT * 10U) + 50U); /* block period is 10 ms */

	return ret;
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

static void codec_fault_handler(const struct device *dev, uint32_t errors)
{
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

	(void)audio_codec_clear_errors(dev);
}

static void test_i2s(void)
{
	int ret;
	struct audio_codec_cfg cfg = {0};

	/* ASI target mode: the host (nRF I2S or the mock) drives BCLK/FSYNC. */
	cfg.mclk_freq = 0U;
	cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	cfg.dai_cfg.i2s.word_size = AUDIO_WORD_BITS;
	cfg.dai_cfg.i2s.channels = AUDIO_CHANNELS;
	cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	cfg.dai_cfg.i2s.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET;
	cfg.dai_cfg.i2s.frame_clk_freq = AUDIO_SAMPLE_RATE_HZ;

	LOG_INF("[I2S]  audio serial port init + TX");
	if (!device_is_ready(codec)) {
		LOG_ERR("  device not ready");
		return;
	}

	ret = audio_codec_register_error_callback(codec, codec_fault_handler);
	if (ret != 0) {
		LOG_WRN("register_error_callback() returned %d", ret);
	}

	ret = audio_codec_configure(codec, &cfg);
	if (ret != 0) {
		LOG_ERR("audio_codec_configure() failed (%d)", ret);
		return;
	}

	LOG_INF("  codec configured: I2S target, %u Hz, %u-bit, %u ch", AUDIO_SAMPLE_RATE_HZ,
		AUDIO_WORD_BITS, AUDIO_CHANNELS);

	/* Start at 0 dB, unmuted, then power up the DAC. */
	if (codec_set_volume(HALF_DB(0)) != 0 || codec_set_mute(false) != 0) {
		return;
	}

	audio_codec_start_output(codec);
	LOG_INF("  output enabled");

	ret = play_test_tone();
	if (ret != 0) {
		LOG_ERR("audio_codec_configure() failed (%d)", ret);
	}

	audio_codec_stop_output(codec);
	LOG_INF("  output disabled");

	(void)audio_codec_clear_errors(codec);
}
#else  /* no I2S alias */
static int test_i2s(void)
{
	LOG_INF("no tac5112-i2s alias in devicetree; skipping I2S test");
	return 0;
}
#endif /* DT_NODE_EXISTS(DT_ALIAS(tac5112_i2s)) */

int main(void)
{
	LOG_INF("========================================");
	LOG_INF(" nRF-Audio-EB shield smoke test");
	LOG_INF("========================================");

	test_leds();
	test_line_in();
	test_codec();
	test_pdm();
	test_i2s();
	test_sdcard();

	LOG_INF("===== smoke test complete =====");
	return 0;
}
