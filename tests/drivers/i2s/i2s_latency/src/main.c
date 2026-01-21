/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/counter.h>

#define SAMPLE_WIDTH		       CONFIG_I2S_SAMPLE_WIDTH
#define WORDS_COUNT		       16
#define TIMEOUT_MS		       2000
#define NUMBER_OF_BLOCKS	       1
#define NUMBER_OF_CHANNELS	       1
#define TEST_TIMER_COUNT_TIME_LIMIT_MS 500
#define MEASUREMENT_REPEATS	       10
#define MAX_TOLERANCE		       2.5

const struct device *const i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s_dut));
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));

#if (SAMPLE_WIDTH == 8)
#define SLAB_ALIGN 2
static const int8_t test_data[WORDS_COUNT] = {1,  9,  2,  10, 4,  12, 8,   16,
					      16, 24, 32, 40, 64, 72, 128, 136};
#elif (SAMPLE_WIDTH == 16)
#define SLAB_ALIGN 32
static const int16_t test_data[WORDS_COUNT] = {1,  9,  2,  10, 4,  12, 8,   16,
					       16, 24, 32, 40, 64, 72, 128, 136};
#else
#define SLAB_ALIGN 32
static const int32_t test_data[WORDS_COUNT] = {1,  9,  2,  10, 4,  12, 8,   16,
					       16, 24, 32, 40, 64, 72, 128, 136};
#endif

#define BLOCK_SIZE (sizeof(test_data))

#ifdef CONFIG_NOCACHE_MEMORY
#define MEM_SLAB_CACHE_ATTR __nocache
#else
#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(
	SLAB_ALIGN)) _k_mem_slab_buf_rx_mem_slab[(NUMBER_OF_BLOCKS + 2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab, rx_mem_slab) = Z_MEM_SLAB_INITIALIZER(
	rx_mem_slab, _k_mem_slab_buf_rx_mem_slab, WB_UP(BLOCK_SIZE), NUMBER_OF_BLOCKS + 2);

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(
	SLAB_ALIGN)) _k_mem_slab_buf_tx_mem_slab[(NUMBER_OF_BLOCKS)*WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab,
			tx_mem_slab) = Z_MEM_SLAB_INITIALIZER(tx_mem_slab,
							      _k_mem_slab_buf_tx_mem_slab,
							      WB_UP(BLOCK_SIZE), NUMBER_OF_BLOCKS);

void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms)
{
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)count_time_ms * 1000);
	counter_cfg.user_data = &counter_cfg;
}

#if (SAMPLE_WIDTH == 8)
static void fill_tx_buffer(int8_t *tx_block)
#elif (SAMPLE_WIDTH == 16)
static void fill_tx_buffer(int16_t *tx_block)
#else
static void fill_tx_buffer(int32_t *tx_block)
#endif
{
	for (int i = 0; i < WORDS_COUNT; i++) {
		tx_block[i] = test_data[i];
	}
}

#if (SAMPLE_WIDTH == 8)
static int verify_rx_buffer(int8_t *rx_block)
#elif (SAMPLE_WIDTH == 16)
static int verify_rx_buffer(int16_t *rx_block)
#else
static int verify_rx_buffer(int32_t *rx_block)
#endif
{
	int last_word = WORDS_COUNT;

/* Find offset. */
#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
	static ZTEST_DMEM int offset = -1;

	if (offset < 0) {
		do {
			++offset;
			if (offset > CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET) {
				TC_PRINT("Allowed data offset exceeded\n");
				return -TC_FAIL;
			}
		} while (rx_block[NUMBER_OF_CHANNELS * offset] != test_data[0]);

		TC_PRINT("Using data offset: %d\n", offset);
	}

	rx_block += NUMBER_OF_CHANNELS * offset;
	last_word -= NUMBER_OF_CHANNELS * offset;
#endif

	for (int i = 0; i < last_word; i++) {
		if (rx_block[i] != test_data[i]) {
			TC_PRINT("Error: data mismatch at position %d, expected %d, actual %d\n", i,
				 test_data[i], rx_block[i]);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

static void configure_i2s(const struct device *dev, uint32_t frame_clk_freq)
{
	int ret;
	struct i2s_config i2s_cfg;

	i2s_cfg.word_size = SAMPLE_WIDTH;
	i2s_cfg.channels = NUMBER_OF_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = frame_clk_freq;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT_MS;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;

	i2s_cfg.mem_slab = &tx_mem_slab;
	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	zassert_ok(ret, "Failed to configure I2S TX stream: %d", ret);

	i2s_cfg.mem_slab = &rx_mem_slab;
	ret = i2s_configure(i2s_dev, I2S_DIR_RX, &i2s_cfg);
	zassert_ok(ret, "Failed to configure I2S RX stream: %d", ret);
}

uint32_t calculate_theoretical_transsmison_time_us(uint8_t words_count, uint32_t frame_clk_freq)
{
	return (uint32_t)(1000000 * (double)words_count / (double)frame_clk_freq);
}

static void test_i2s_transmission_latency(const struct device *dev, uint32_t frame_clk_freq)
{
	int ret;
	void *rx_block[NUMBER_OF_BLOCKS];
	void *tx_block;
	size_t rx_size;
	uint32_t tst_timer_value;
	uint64_t timer_value_us[MEASUREMENT_REPEATS];
	uint64_t average_timer_value_us = 0;
	uint32_t theoretical_transmission_time_us;
	uint32_t maximal_allowed_transmission_time_us;

	TC_PRINT("Number of samples: %u\n", WORDS_COUNT);

	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	/* Configure I2S Dir Both transfer. */
	configure_i2s(dev, frame_clk_freq);

	for (uint32_t repeat_counter = 0; repeat_counter < MEASUREMENT_REPEATS; repeat_counter++) {

		/* Prefill TX queue */
		ret = k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_FOREVER);
		zassert_equal(ret, 0, "TX mem slab allocation failed");

#if (SAMPLE_WIDTH == 8)
		fill_tx_buffer((uint8_t *)tx_block);
#elif (SAMPLE_WIDTH == 16)
		fill_tx_buffer((uint16_t *)tx_block);
#else
		fill_tx_buffer((uint32_t *)tx_block);
#endif
		ret = i2s_write(i2s_dev, tx_block, BLOCK_SIZE);
		zassert_equal(ret, 0, "I2S write failed");

		counter_reset(tst_timer_dev);
		counter_start(tst_timer_dev);
		ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
		zassert_equal(ret, 0, "RX/TX START trigger failed\n");
		/* All data written, drain TX queue and stop both streams. */
		ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
		zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");
		ret = i2s_read(i2s_dev, rx_block, &rx_size);
		counter_get_value(tst_timer_dev, &tst_timer_value);
		counter_stop(tst_timer_dev);

		zassert_equal(ret, 0, "I2S read failed");
		zassert_equal(rx_size, BLOCK_SIZE);

		/* Verify received data */
#if (SAMPLE_WIDTH == 8)
		ret = verify_rx_buffer((uint8_t *)rx_block[0]);
#elif (SAMPLE_WIDTH == 16)
		ret = verify_rx_buffer((uint16_t *)rx_block[0]);
#else
		ret = verify_rx_buffer((uint32_t *)rx_block[0]);
#endif
		zassert_equal(ret, 0, "TX data does not match RX data");
		k_mem_slab_free(&rx_mem_slab, rx_block[0]);

		timer_value_us[repeat_counter] =
			counter_ticks_to_us(tst_timer_dev, tst_timer_value);
		average_timer_value_us += timer_value_us[repeat_counter];
	}

	average_timer_value_us /= MEASUREMENT_REPEATS;

	theoretical_transmission_time_us =
		calculate_theoretical_transsmison_time_us(WORDS_COUNT, frame_clk_freq);
	maximal_allowed_transmission_time_us =
		(uint32_t)(MAX_TOLERANCE * (double)theoretical_transmission_time_us);
	TC_PRINT("Calculated transmission time (for frame clk = %uHz, sample width = %ubit) [us]: "
		 "%u\n",
		 frame_clk_freq, SAMPLE_WIDTH, theoretical_transmission_time_us);
	TC_PRINT("Measured transmission time (for frame clk = %uHz, sample width = %ubit) [us]: "
		 "%llu\n",
		 frame_clk_freq, SAMPLE_WIDTH, average_timer_value_us);
	TC_PRINT("Measured - claculated time delta (for frame clk = %uHz, sample width = %ubit) "
		 "[us]: %lld\n",
		 frame_clk_freq, SAMPLE_WIDTH,
		 average_timer_value_us - theoretical_transmission_time_us);
	TC_PRINT("Maximal allowed transmission time (for frame clk = %uHz, sample width = %ubit) "
		 "[us]: %u\n",
		 frame_clk_freq, SAMPLE_WIDTH, maximal_allowed_transmission_time_us);

	zassert_true(average_timer_value_us < maximal_allowed_transmission_time_us,
		     "Measured call latency is over the specified limit");
}

void *test_setup(void)
{
	zassert_true(device_is_ready(i2s_dev), "I2S device is not ready");
	return NULL;
}

ZTEST(i2s_latency, test_i2s_transmission_call_latency)
{
	test_i2s_transmission_latency(i2s_dev, 4000);
	test_i2s_transmission_latency(i2s_dev, 8000);
	test_i2s_transmission_latency(i2s_dev, 44100);
	test_i2s_transmission_latency(i2s_dev, 48000);
}

ZTEST_SUITE(i2s_latency, NULL, test_setup, NULL, NULL, NULL);
