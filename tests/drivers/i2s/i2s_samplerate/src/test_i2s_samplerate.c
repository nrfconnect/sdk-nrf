/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>

#include <zephyr/drivers/gpio.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>

#define I2S_DEV_NODE DT_ALIAS(i2s_node0)

#define NUM_BLOCKS 4
#define TIMEOUT	   200

#ifdef CONFIG_NOCACHE_MEMORY
#define MEM_SLAB_CACHE_ATTR __nocache
#else
#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

#define SAMPLE_NO 64

/* Random data with no meaning. */
static int16_t data[SAMPLE_NO] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x10, 0x20, 0x40, 0x80, 0xAA, 0x55, 0x55, 0xAA,
	0xfe, 0xef, 0xfe, 0xee, 0x00, 0x01, 0x02, 0x04,
	0x0e, 0x12, 0x01, 0x80, 0x81, 0x18, 0xAA, 0x55,
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x10, 0x20, 0x40, 0x80, 0xAA, 0x55, 0x55, 0xAA,
	0xfe, 0xef, 0xfe, 0xee, 0x00, 0x01, 0x02, 0x04,
	0x0e, 0x12, 0x01, 0x80, 0x81, 0x18, 0xAA, 0x55,
};

#define BLOCK_SIZE (sizeof(data))

/*
 * NUM_BLOCKS is the number of blocks used by the test. Some of the drivers,
 * permanently keep ownership of a few RX buffers. Add a two more
 * RX blocks to satisfy this requirement
 */
char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_rx_0_mem_slab[(NUM_BLOCKS + 2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, rx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(rx_0_mem_slab, _k_mem_slab_buf_rx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS + 2);

char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

static const struct device *dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE);

#if defined(CONFIG_DT_HAS_NORDIC_NRF_TDM_ENABLED)
volatile NRF_TDM_Type *p_reg = (NRF_TDM_Type *)DT_REG_ADDR(I2S_DEV_NODE);
#else
volatile NRF_I2S_Type *p_reg = (NRF_I2S_Type *)DT_REG_ADDR(I2S_DEV_NODE);
#endif

static const struct i2s_config default_i2s_cfg = {
	.word_size = 16U,
	.channels = 2U,
	.format = I2S_FMT_DATA_FORMAT_I2S,
	.frame_clk_freq = 44100U,
	.block_size = BLOCK_SIZE,
	.timeout = TIMEOUT,
	.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER,
	.mem_slab = &tx_0_mem_slab,
};

/* Variables used to count edges on I2S_LRCK_M line. */
#define CLOCK_INPUT_PIN NRF_DT_GPIOS_TO_PSEL(DT_PATH(zephyr_user), test_gpios)

#if defined(NRF_TIMER0)
static nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(NRF_TIMER0);
#elif defined(NRF_TIMER00)
static nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(NRF_TIMER00);
#elif defined(NRF_TIMER130)
static nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(NRF_TIMER130);
#else
#error "No timer instance found"
#endif

static bool i2s_enabled;

static void capture_callback(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	nrfx_timer_capture(&timer_instance, NRF_TIMER_CC_CHANNEL0);
	i2s_enabled = false;
}

K_TIMER_DEFINE(capture_timer, capture_callback, NULL);

static void fill_buf(uint16_t *tx_block)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[i] = data[i];
	}
}

static int configure_stream(const struct device *dev, enum i2s_dir dir, struct i2s_config *i2s_cfg)
{
	int ret;

	if (dir == I2S_DIR_TX) {
		/* Configure the Transmit port as Master */
		i2s_cfg->options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	} else if (dir == I2S_DIR_RX) {
		/* Configure the Receive port as Slave */
		i2s_cfg->options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
	} else { /* dir == I2S_DIR_BOTH */
		i2s_cfg->options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_cfg->mem_slab = &tx_0_mem_slab;
		ret = i2s_configure(dev, I2S_DIR_TX, i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S TX stream (%d)\n", ret);
			return ret;
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_cfg->mem_slab = &rx_0_mem_slab;
		ret = i2s_configure(dev, I2S_DIR_RX, i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S RX stream (%d)\n", ret);
			return ret;
		}
	}

	return TC_PASS;
}

static void i2s_dir_both_transfer_long(struct i2s_config *i2s_cfg, uint32_t expected_divider,
				       uint32_t expected_edges, uint32_t delta)
{
	void *rx_block[NUM_BLOCKS];
	void *tx_block[NUM_BLOCKS];
	size_t rx_size;
	int tx_idx;
	int rx_idx = 0;
	int ret;

	/* Configure I2S Dir Both transfer. */
	ret = configure_stream(dev_i2s, I2S_DIR_BOTH, i2s_cfg);
	zassert_equal(ret, TC_PASS);

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx], K_FOREVER);
		zassert_equal(ret, 0);
		fill_buf((uint16_t *)tx_block[tx_idx]);
	}

	/* Prefill TX queue */
	ret = i2s_write(dev_i2s, tx_block[0], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_write(dev_i2s, tx_block[1], BLOCK_SIZE);
	zassert_equal(ret, 0);

	/* Clear clock cycles counter */
	nrfx_timer_clear(&timer_instance);

	tx_idx = 2;

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	/* When kerel timer expires:
	 *  - NRFX timer counter value is stored;
	 *  - i2s_enabled is set to false.
	 */
	i2s_enabled = true;
	k_timer_start(&capture_timer, K_SECONDS(1), K_NO_WAIT);

	while (i2s_enabled) {
		ret = i2s_write(dev_i2s, tx_block[tx_idx % NUM_BLOCKS], BLOCK_SIZE);
		zassert_equal(ret, 0);
		tx_idx++;

		ret = i2s_read(dev_i2s, &rx_block[rx_idx % NUM_BLOCKS], &rx_size);
		zassert_equal(ret, 0, "Got unexpected %d", ret);

		k_mem_slab_free(&rx_0_mem_slab, rx_block[rx_idx % NUM_BLOCKS]);
		rx_idx++;
	}

	/* All data written, drain TX/RX queues. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = 0;
	while (ret == 0) {
		ret = i2s_read(dev_i2s, &rx_block[rx_idx % NUM_BLOCKS], &rx_size);
		if (ret == 0) {
			k_mem_slab_free(&rx_0_mem_slab, rx_block[rx_idx % NUM_BLOCKS]);
			rx_idx++;
		}
	}

	TC_PRINT("%d TX blocks sent\n", tx_idx);
	TC_PRINT("%d RX blocks received\n", rx_idx);

	/* Get number of I2S LRCK rising edges. */
	uint32_t pulses = nrfx_timer_capture_get(&timer_instance, NRF_TIMER_CC_CHANNEL0);

	TC_PRINT("LRCK edges: %u\n", pulses);
#if defined(CONFIG_DT_HAS_NORDIC_NRF_TDM_ENABLED)
	TC_PRINT("SCK divider is %u\n", p_reg->CONFIG.SCK.DIV);
	zassert_equal(p_reg->CONFIG.SCK.DIV, expected_divider);
#else
	TC_PRINT("MCK divider is %u\n", p_reg->CONFIG.MCKFREQ);
	zassert_equal(p_reg->CONFIG.MCKFREQ, expected_divider);
#endif

	zassert_within(pulses, expected_edges, delta, "got %u while expected is %u with delta %u",
		       pulses, expected_edges, delta);
}

ZTEST(i2s_samplerate, test_dir_both_at_08000_sps)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_8000_SKIP);

	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.frame_clk_freq = 8000;

	i2s_dir_both_transfer_long(&i2s_cfg, CONFIG_I2S_TEST_8000_DIVIDER,
				   CONFIG_I2S_TEST_8000_EXPECTED, CONFIG_I2S_TEST_8000_TOLERANCE);
}

ZTEST(i2s_samplerate, test_dir_both_at_16000_sps)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_16000_SKIP);

	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.frame_clk_freq = 16000;

	i2s_dir_both_transfer_long(&i2s_cfg, CONFIG_I2S_TEST_16000_DIVIDER,
				   CONFIG_I2S_TEST_16000_EXPECTED, CONFIG_I2S_TEST_16000_TOLERANCE);
}

ZTEST(i2s_samplerate, test_dir_both_at_32000_sps)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_32000_SKIP);

	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.frame_clk_freq = 32000;

	i2s_dir_both_transfer_long(&i2s_cfg, CONFIG_I2S_TEST_32000_DIVIDER,
				   CONFIG_I2S_TEST_32000_EXPECTED, CONFIG_I2S_TEST_32000_TOLERANCE);
}

ZTEST(i2s_samplerate, test_dir_both_at_44100_sps)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_44100_SKIP);

	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.frame_clk_freq = 44100;

	i2s_dir_both_transfer_long(&i2s_cfg, CONFIG_I2S_TEST_44100_DIVIDER,
				   CONFIG_I2S_TEST_44100_EXPECTED, CONFIG_I2S_TEST_44100_TOLERANCE);
}

ZTEST(i2s_samplerate, test_dir_both_at_48000_sps)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_48000_SKIP);

	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.frame_clk_freq = 48000;

	i2s_dir_both_transfer_long(&i2s_cfg, CONFIG_I2S_TEST_48000_DIVIDER,
				   CONFIG_I2S_TEST_48000_EXPECTED, CONFIG_I2S_TEST_48000_TOLERANCE);
}

ZTEST(i2s_samplerate, test_dir_both_at_88200_sps)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_88200_SKIP);

	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.frame_clk_freq = 88200;

	i2s_dir_both_transfer_long(&i2s_cfg, CONFIG_I2S_TEST_88200_DIVIDER,
				   CONFIG_I2S_TEST_88200_EXPECTED, CONFIG_I2S_TEST_88200_TOLERANCE);
}

ZTEST(i2s_samplerate, test_dir_both_at_96000_sps)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_96000_SKIP);

	struct i2s_config i2s_cfg = default_i2s_cfg;

	i2s_cfg.frame_clk_freq = 96000;

	i2s_dir_both_transfer_long(&i2s_cfg, CONFIG_I2S_TEST_96000_DIVIDER,
				   CONFIG_I2S_TEST_96000_EXPECTED, CONFIG_I2S_TEST_96000_TOLERANCE);
}

static void *suite_setup(void)
{
	int ret;

	TC_PRINT("I2S samplerate test on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("Testing I2S device %s\n", dev_i2s->name);
	TC_PRINT("I2S device address is %p\n", p_reg);
	TC_PRINT("Target values are:\n");
	TC_PRINT(" -  8000 Sps: %u +/- %u\n", CONFIG_I2S_TEST_8000_EXPECTED,
		 CONFIG_I2S_TEST_8000_TOLERANCE);
	TC_PRINT(" - 16000 Sps: %u +/- %u\n", CONFIG_I2S_TEST_16000_EXPECTED,
		 CONFIG_I2S_TEST_16000_TOLERANCE);
	TC_PRINT(" - 32000 Sps: %u +/- %u\n", CONFIG_I2S_TEST_32000_EXPECTED,
		 CONFIG_I2S_TEST_32000_TOLERANCE);
	TC_PRINT(" - 44100 Sps: %u +/- %u\n", CONFIG_I2S_TEST_44100_EXPECTED,
		 CONFIG_I2S_TEST_44100_TOLERANCE);
	TC_PRINT(" - 48000 Sps: %u +/- %u\n", CONFIG_I2S_TEST_48000_EXPECTED,
		 CONFIG_I2S_TEST_48000_TOLERANCE);
	TC_PRINT(" - 88200 Sps: %u +/- %u\n", CONFIG_I2S_TEST_88200_EXPECTED,
		 CONFIG_I2S_TEST_88200_TOLERANCE);
	TC_PRINT(" - 96000 Sps: %u +/- %u\n", CONFIG_I2S_TEST_96000_EXPECTED,
		 CONFIG_I2S_TEST_96000_TOLERANCE);
	TC_PRINT("===================================================================\n");

	/* Check I2S Device. */
	zassert_not_null(dev_i2s, "I2S device not found");
	zassert(device_is_ready(dev_i2s), "I2S device not ready");

	/* Configure GPIOTE. */
	uint8_t gpiote_channel;
	nrfx_gpiote_t gpiote_instance =
		GPIOTE_NRFX_INST_BY_NODE(NRF_DT_GPIOTE_NODE(DT_PATH(zephyr_user), test_gpios));

	ret = nrfx_gpiote_channel_alloc(&gpiote_instance, &gpiote_channel);
	zassert_true(ret == 0, "GPIOTE channel allocation failed, return code = %d", ret);

	nrfx_gpiote_trigger_config_t trigger_cfg = {
		.p_in_channel = &gpiote_channel,
		.trigger = NRFX_GPIOTE_TRIGGER_LOTOHI,
	};

	nrf_gpio_pin_pull_t pull_cfg = NRFX_GPIOTE_DEFAULT_PULL_CONFIG;

	nrfx_gpiote_input_pin_config_t gpiote_cfg = {
		.p_pull_config = &pull_cfg,
		.p_trigger_config = &trigger_cfg,
	};

	ret = nrfx_gpiote_input_configure(&gpiote_instance, CLOCK_INPUT_PIN, &gpiote_cfg);
	zassert_true(ret == 0, "GPIOTE input configuration failed, return code = %d", ret);

	nrfx_gpiote_trigger_enable(&gpiote_instance, CLOCK_INPUT_PIN, false);

	/* Configure Timer. */
	nrfx_timer_config_t timer_config =
		NRFX_TIMER_DEFAULT_CONFIG(NRFX_TIMER_BASE_FREQUENCY_GET(&timer_instance));
	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	timer_config.mode = NRF_TIMER_MODE_COUNTER;

	ret = nrfx_timer_init(&timer_instance, &timer_config, NULL);
	zassert_true(ret == 0, "nrfx_timer_init() failed, ret = 0x%08X", ret);

	nrfx_timer_enable(&timer_instance);

	/* Configure GPPI from GPIOTE to Timer. */
	nrfx_gppi_handle_t gppi_handle;
	uint32_t eep = nrfx_gpiote_in_event_address_get(&gpiote_instance, CLOCK_INPUT_PIN);
	uint32_t tep = nrfx_timer_task_address_get(&timer_instance, NRF_TIMER_TASK_COUNT);

	ret = nrfx_gppi_conn_alloc(eep, tep, &gppi_handle);
	zassert_equal(ret, 0, "GPPI channel allocation failed, return code = %d", ret);
	nrfx_gppi_conn_enable(gppi_handle);

	return 0;
}

ZTEST_SUITE(i2s_samplerate, NULL, suite_setup, NULL, NULL, NULL);
