/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/iterable_sections.h>

#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>

#define I2S_DEV_NODE DT_ALIAS(i2s_node0)
#define NRF_MCK_TIMER DT_REG_ADDR(DT_ALIAS(mck_timer))
#define NRF_LRCK_TIMER DT_REG_ADDR(DT_ALIAS(lrck_timer))

#define WORD_SIZE 16U
#define NUMBER_OF_CHANNELS CONFIG_TEST_NUMBER_OF_I2S_CHANNELS
#define FRAME_CLK_FREQ 1000

#define NUM_BLOCKS 2
#define TIMEOUT 1000

#define WORDS_COUNT 16
/* Each word has one bit set */
static const int16_t data[WORDS_COUNT] = {
	1, 9, 2, 10, 4, 12, 8, 16, 16, 24, 32, 40, 64, 72, 128, 136
};

#define BLOCK_SIZE (sizeof(data))

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

/*
 * NUM_BLOCKS is the number of blocks used by the test. Some of the drivers,
 * permanently keep ownership of a few RX buffers. Add a two more
 * RX blocks to satisfy this requirement
 */
static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_rx_0_mem_slab[(NUM_BLOCKS + 2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, rx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(rx_0_mem_slab, _k_mem_slab_buf_rx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS + 2);

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
	_k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
	Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
				WB_UP(BLOCK_SIZE), NUM_BLOCKS);

static volatile uint32_t count_mclk, count_lrck;

static const struct device *dev_i2s;

#if defined(CONFIG_COVERAGE)
#define EXPECTED_MCLK_SCALE 0.5
#else
#define EXPECTED_MCLK_SCALE 1.0
#endif

/* The falling edge of the frame sync signal (LRCK)
 * indicates the start of the PCM word.
 * An arbitrary number of data words can be sent in one frame.
 */
static uint32_t expected_lrck = (WORDS_COUNT / NUMBER_OF_CHANNELS) * EXPECTED_MCLK_SCALE;

/* There are MCLK_FREQ / FRAME_CLK_FREQ edges in one PCM sample.
 * There will be WORDS_COUNT / NUMBER_OF_CHANNELS PCM samples in total.
 */
#if defined(CONFIG_I2S_NRFX)
#define MCLK_FREQ (32000)
/* Due to lower performance some edges may be missed. */
static uint32_t expected_mclk = (MCLK_FREQ / FRAME_CLK_FREQ) * (WORDS_COUNT / NUMBER_OF_CHANNELS) *
				0.94 * EXPECTED_MCLK_SCALE;
#else
#define MCLK_FREQ (64000)
static uint32_t expected_mclk =
	(MCLK_FREQ / FRAME_CLK_FREQ) * (WORDS_COUNT / NUMBER_OF_CHANNELS) * EXPECTED_MCLK_SCALE;
#endif

static nrfx_timer_t mck_timer_instance = NRFX_TIMER_INSTANCE(NRF_MCK_TIMER);
static nrfx_timer_t lrck_timer_instance = NRFX_TIMER_INSTANCE(NRF_LRCK_TIMER);

#define MCK_INPUT_PIN NRF_DT_GPIOS_TO_PSEL_BY_IDX(DT_PATH(zephyr_user), gpios, 0)
#define LRCK_INPUT_PIN NRF_DT_GPIOS_TO_PSEL_BY_IDX(DT_PATH(zephyr_user), gpios, 1)

/* Fill in TX buffer with test samples. */
static void fill_buf(int16_t *tx_block)
{
	for (int i = 0; i < WORDS_COUNT; i++) {
		tx_block[i] = data[i];
	}
}

static int verify_buf(int16_t *rx_block)
{
	int last_word = WORDS_COUNT;

/* Find offset. */
#if (CONFIG_TEST_I2S_ALLOWED_DATA_OFFSET > 0)
	static ZTEST_DMEM int offset = -1;

	if (offset < 0) {
		do {
			++offset;
			if (offset > CONFIG_TEST_I2S_ALLOWED_DATA_OFFSET) {
				TC_PRINT("Allowed data offset exceeded\n");
				return -TC_FAIL;
			}
		} while (rx_block[NUMBER_OF_CHANNELS * offset] != data[0]);

		TC_PRINT("Using data offset: %d\n", offset);
	}

	rx_block += NUMBER_OF_CHANNELS * offset;
	last_word -= NUMBER_OF_CHANNELS * offset;
#endif

	/* Compare received data with sent values. */
	for (int i = 0; i < last_word; i++) {
		if (rx_block[i] != data[i]) {
			TC_PRINT("Error: data mismatch at position %d, expected %d, actual %d\n",
				 i, data[i], rx_block[i]);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

static int configure_stream(const struct device *dev_i2s, enum i2s_dir dir)
{
	int ret;
	struct i2s_config i2s_cfg;

	i2s_cfg.word_size = WORD_SIZE;
	i2s_cfg.channels = NUMBER_OF_CHANNELS;
#if defined(CONFIG_I2S_NRFX)
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
#else
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_PCM_SHORT;
#endif
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT;

	if (dir == I2S_DIR_TX) {
		/* Configure the Transmit port as Master */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_CONTROLLER
				| I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_BIT_CLK_GATED;
	} else if (dir == I2S_DIR_RX) {
		/* Configure the Receive port as Slave */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_TARGET
				| I2S_OPT_BIT_CLK_TARGET | I2S_OPT_BIT_CLK_GATED;
	} else { /* dir == I2S_DIR_BOTH */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_CONTROLLER
				| I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_BIT_CLK_GATED;
	}

	if (!IS_ENABLED(CONFIG_TEST_USE_GPIO_LOOPBACK)) {
		i2s_cfg.options |= I2S_OPT_LOOPBACK;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &tx_0_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S TX stream (%d)\n",
				 ret);
			return -TC_FAIL;
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &rx_0_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S RX stream (%d)\n",
				 ret);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}


/** @brief Check MCLK signal in short I2S transfer using I2S_DIR_BOTH.
 *
 * Verify that MCLK is generated when I2s is configured
 * to send and receive data.
 */
ZTEST(drivers_i2s_mclk, test_i2s_mclk_I2S_DIR_BOTH)
{
	void *rx_block[1];
	void *tx_block;
	size_t rx_size;
	int ret;

	/* Configure I2S Dir Both transfer. */
	ret = configure_stream(dev_i2s, I2S_DIR_BOTH);
	zassert_equal(ret, TC_PASS);

	/* Prefill TX queue */
	ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
	zassert_equal(ret, 0);
	fill_buf((uint16_t *)tx_block);

	ret = i2s_write(dev_i2s, tx_block, BLOCK_SIZE);
	zassert_equal(ret, 0);

	/* Clear clock cycles counters */
	nrfx_timer_clear(&mck_timer_instance);
	nrfx_timer_clear(&lrck_timer_instance);

	TC_PRINT("TX starts\n");

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = i2s_read(dev_i2s, &rx_block[0], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	TC_PRINT("RX done\n");

	/* Verify received data */
	ret = verify_buf((uint16_t *)rx_block[0]);
	zassert_equal(ret, 0);
	k_mem_slab_free(&rx_0_mem_slab, rx_block[0]);

	/* Get number of I2S MCLK rising edges. */
	nrfx_timer_capture(&mck_timer_instance, NRF_TIMER_CC_CHANNEL0);
	count_mclk = nrfx_timer_capture_get(&mck_timer_instance, NRF_TIMER_CC_CHANNEL0);

	/* Get number of I2S LRCLK rising edges. */
	nrfx_timer_capture(&lrck_timer_instance, NRF_TIMER_CC_CHANNEL0);
	count_lrck = nrfx_timer_capture_get(&lrck_timer_instance, NRF_TIMER_CC_CHANNEL0);

	/* Verify number of falling edges on LRCK. */
	zassert_true(count_lrck >= expected_lrck,
		"LRCK has %u falling edges, while at least %u is expected.",
		count_lrck, expected_lrck);

	/* Verify number of falling edges on MCLK. */
	zassert_true(count_mclk >= expected_mclk,
		"MCLK has %u falling edges, while at least %u is expected.",
		count_mclk, expected_mclk);
}

/** @brief Check MCLK signal in short I2S transfer using I2S_DIR_RX.
 *
 * Verify that MCLK is generated when I2s is configured to receive data.
 */
ZTEST(drivers_i2s_mclk, test_i2s_mclk_I2S_DIR_RX)
{
	void *rx_block[1];
	size_t rx_size;
	int ret;

	Z_TEST_SKIP_IFDEF(CONFIG_TEST_SKIP_I2S_DIR_RX);

	/* Configure I2S Dir RX transfer. */
	ret = configure_stream(dev_i2s, I2S_DIR_RX);
	zassert_equal(ret, TC_PASS);

	/* Clear clock cycles counters */
	nrfx_timer_clear(&mck_timer_instance);

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed\n");

	/* No master on the other side, this will timeout. */
	ret = i2s_read(dev_i2s, &rx_block[0], &rx_size);

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	k_msleep(2);

	/* Verify number of falling edges on MCLK */
	uint32_t expected = FRAME_CLK_FREQ * NUMBER_OF_CHANNELS * WORD_SIZE * (TIMEOUT / 1000);

	/* Get number of I2S MCLK rising edges. */
	nrfx_timer_capture(&mck_timer_instance, NRF_TIMER_CC_CHANNEL0);
	count_mclk = nrfx_timer_capture_get(&mck_timer_instance, NRF_TIMER_CC_CHANNEL0);

	zassert_true(count_mclk >= expected,
		"MCLK has %u falling edges, while at least %u is expected.",
		count_mclk, expected);

	/* Cleanup from Error state. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_DROP);
	zassert_equal(ret, 0, "RX DROP trigger failed");
}

/** @brief Check MCLK signal in short I2S transfer using I2S_DIR_TX.
 *
 * Verify that MCLK is generated when I2s is configured to send data.
 */
ZTEST(drivers_i2s_mclk, test_i2s_mclk_I2S_DIR_TX)
{
	void *tx_block;
	int ret;

	Z_TEST_SKIP_IFDEF(CONFIG_TEST_SKIP_I2S_DIR_TX);

	/* Configure I2S Dir TX transfer. */
	ret = configure_stream(dev_i2s, I2S_DIR_TX);
	zassert_equal(ret, TC_PASS);

	/* Prefill TX queue */
	ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
	zassert_equal(ret, 0);
	fill_buf((uint16_t *)tx_block);

	ret = i2s_write(dev_i2s, tx_block, BLOCK_SIZE);
	zassert_equal(ret, 0);

	/* Clear clock cycles counters */
	nrfx_timer_clear(&mck_timer_instance);
	nrfx_timer_clear(&lrck_timer_instance);

	TC_PRINT("TX starts\n");

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed\n");

	/* All data written, drain TX queue and stop stream. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "TX STOP trigger failed");

	/* Wait until I2S finishes TX. */
	k_msleep(1000);

	/* Get number of I2S MCLK rising edges. */
	nrfx_timer_capture(&mck_timer_instance, NRF_TIMER_CC_CHANNEL0);
	count_mclk = nrfx_timer_capture_get(&mck_timer_instance, NRF_TIMER_CC_CHANNEL0);

	/* Get number of I2S LRCLK rising edges. */
	nrfx_timer_capture(&lrck_timer_instance, NRF_TIMER_CC_CHANNEL0);
	count_lrck = nrfx_timer_capture_get(&lrck_timer_instance, NRF_TIMER_CC_CHANNEL0);

	/* Verify number of falling edges on LRCK. */
	zassert_true(count_lrck >= expected_lrck,
		"LRCK has %u falling edges, while at least %u is expected.",
		count_lrck, expected_lrck);

	/* Verify number of falling edges on MCLK. */
	zassert_true(count_mclk >= expected_mclk,
		"MCLK has %u falling edges, while at least %u is expected.",
		count_mclk, expected_mclk);
}

static void *suite_setup(void)
{
	int ret;

	/* Configure GPIOTE. */
	uint8_t gpiote_channel_mclk, gpiote_channel_lrclk;
	nrfx_gpiote_t gpiote_instance =
		GPIOTE_NRFX_INST_BY_NODE(NRF_DT_GPIOTE_NODE(DT_PATH(zephyr_user), gpios));

	nrfx_gpiote_trigger_config_t trigger_cfg = {
		.trigger = NRFX_GPIOTE_TRIGGER_LOTOHI,
	};

	nrf_gpio_pin_pull_t pull_cfg = NRFX_GPIOTE_DEFAULT_PULL_CONFIG;

	nrfx_gpiote_input_pin_config_t gpiote_cfg = {
		.p_pull_config = &pull_cfg,
		.p_trigger_config = &trigger_cfg,
	};

	ret = nrfx_gpiote_channel_alloc(&gpiote_instance, &gpiote_channel_mclk);
	zassert_true(ret == 0, "MCLK GPIOTE channel allocation failed, return code = %d", ret);
	trigger_cfg.p_in_channel = &gpiote_channel_mclk;
	ret = nrfx_gpiote_input_configure(&gpiote_instance, MCK_INPUT_PIN, &gpiote_cfg);
	zassert_true(ret == 0, "GPIOTE input configuration for MCK failed, return code = %d", ret);

	ret = nrfx_gpiote_channel_alloc(&gpiote_instance, &gpiote_channel_lrclk);
	zassert_true(ret == 0, "LRCK GPIOTE channel allocation failed, return code = %d", ret);
	trigger_cfg.p_in_channel = &gpiote_channel_lrclk;
	ret = nrfx_gpiote_input_configure(&gpiote_instance, LRCK_INPUT_PIN, &gpiote_cfg);
	zassert_true(ret == 0, "GPIOTE input configuration for LRCK failed, return code = %d", ret);

	nrfx_gpiote_trigger_enable(&gpiote_instance, MCK_INPUT_PIN, false);
	nrfx_gpiote_trigger_enable(&gpiote_instance, LRCK_INPUT_PIN, false);
	/* Configure Timer. */
	nrfx_timer_config_t timer_config =
		NRFX_TIMER_DEFAULT_CONFIG(NRFX_TIMER_BASE_FREQUENCY_GET(&mck_timer_instance));
	timer_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	timer_config.mode = NRF_TIMER_MODE_COUNTER;

	ret = nrfx_timer_init(&mck_timer_instance, &timer_config, NULL);
	zassert_true(ret == 0, "nrfx_timer_init() failed, ret = 0x%08X for mck_timer", ret);
	ret = nrfx_timer_init(&lrck_timer_instance, &timer_config, NULL);
	zassert_true(ret == 0, "nrfx_timer_init() failed, ret = 0x%08X for lrck_timer", ret);

	nrfx_timer_enable(&mck_timer_instance);
	nrfx_timer_enable(&lrck_timer_instance);

	/* Configure GPPI from GPIOTEs to Timers. */
	nrfx_gppi_handle_t mck_gppi_handle, lrck_gppi_handle;
	uint32_t mck_eep = nrfx_gpiote_in_event_address_get(&gpiote_instance, MCK_INPUT_PIN);
	uint32_t mck_tep = nrfx_timer_task_address_get(&mck_timer_instance, NRF_TIMER_TASK_COUNT);
	uint32_t lrck_eep = nrfx_gpiote_in_event_address_get(&gpiote_instance, LRCK_INPUT_PIN);
	uint32_t lrck_tep = nrfx_timer_task_address_get(&lrck_timer_instance, NRF_TIMER_TASK_COUNT);

	ret = nrfx_gppi_conn_alloc(mck_eep, mck_tep, &mck_gppi_handle);
	zassert_equal(ret, 0, "GPPI channel allocation for MCK failed, return code = %d", ret);
	nrfx_gppi_conn_enable(mck_gppi_handle);
	ret = nrfx_gppi_conn_alloc(lrck_eep, lrck_tep, &lrck_gppi_handle);
	zassert_equal(ret, 0, "GPPI channel allocation for LRCK failed, return code = %d", ret);
	nrfx_gppi_conn_enable(lrck_gppi_handle);

	/* Check I2S Device. */
	dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE);
	zassert_not_null(dev_i2s, "I2S device not found");
	zassert(device_is_ready(dev_i2s), "I2S device not ready");

	TC_PRINT("WORD_SIZE: %u\n", WORD_SIZE);
	TC_PRINT("NUMBER_OF_CHANNELS: %u\n", NUMBER_OF_CHANNELS);
	TC_PRINT("TEST_I2S_ALLOWED_DATA_OFFSET: %u\n", CONFIG_TEST_I2S_ALLOWED_DATA_OFFSET);
	TC_PRINT("LRCK: at least %d falling edges are expected\n", expected_lrck);
	TC_PRINT("MCLK: at least %d falling edges are expected\n", expected_mclk);
	TC_PRINT("===================================================================\n");

	return 0;
}

static void before(void *not_used)
{
	ARG_UNUSED(not_used);

	count_mclk = 0;
	count_lrck = 0;
}

static void after(void *not_used)
{
	ARG_UNUSED(not_used);

	TC_PRINT("MCLK: falling edges count = %d\n", count_mclk);
	TC_PRINT("LRCK: falling edges count = %d\n", count_lrck);
}

ZTEST_SUITE(drivers_i2s_mclk, NULL, suite_setup, before, after, NULL);
