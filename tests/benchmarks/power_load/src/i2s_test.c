/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "i2s_test.h"

LOG_MODULE_REGISTER(i2s_test, LOG_LEVEL_INF);

const struct device *const i2s_dev = DEVICE_DT_GET(DT_NODELABEL(dut_i2s));

extern atomic_t started_threads;

static const int32_t test_data[WORDS_COUNT] = {1, 9,  2,  10, 4,  12, 8,   16,
					       6, 24, 32, 40, 64, 72, 128, 136};
void *rx_block[NUMBER_OF_BLOCKS];
void *tx_block;
size_t rx_size;

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

static void fill_tx_buffer(int16_t *tx_block)
{
	for (int i = 0; i < WORDS_COUNT; i++) {
		tx_block[i] = test_data[i];
	}
}

static int verify_rx_buffer(int16_t *rx_block)
{
	int last_word = WORDS_COUNT;

/* Find offset. */
#if (CONFIG_TEST_I2S_ALLOWED_DATA_OFFSET > 0)
	static int offset = -1;

	if (offset < 0) {
		do {
			++offset;
			if (offset > CONFIG_TEST_I2S_ALLOWED_DATA_OFFSET) {
				LOG_ERR("Allowed data offset exceeded\n");
				return -1;
			}
		} while (rx_block[NUMBER_OF_CHANNELS * offset] != test_data[0]);

		LOG_DBG("Using data offset: %d\n", offset);
	}

	rx_block += NUMBER_OF_CHANNELS * offset;
	last_word -= NUMBER_OF_CHANNELS * offset;
#endif

	for (int i = 0; i < last_word; i++) {
		if (rx_block[i] != test_data[i]) {
			LOG_ERR("Error: data mismatch at position %d, expected %d, actual %d\n", i,
				test_data[i], rx_block[i]);
			return -2;
		}
	}

	return 0;
}

static int setup(void)
{
	int ret;
	struct i2s_config i2s_cfg;

	ret = device_is_ready(i2s_dev);
	if (ret != 1) {
		LOG_ERR("I2S device is not ready: %d", ret);
		return -1;
	}

	i2s_cfg.word_size = SAMPLE_WIDTH;
	i2s_cfg.channels = NUMBER_OF_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ_HZ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT_MS;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;

	i2s_cfg.mem_slab = &tx_mem_slab;
	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);

	if (ret != 0) {
		LOG_ERR("Failed to configure I2S TX stream: %d", ret);
		return -1;
	}

	i2s_cfg.mem_slab = &rx_mem_slab;
	ret = i2s_configure(i2s_dev, I2S_DIR_RX, &i2s_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure I2S RX stream: %d", ret);
		return -2;
	}

	return 0;
}

static void i2s_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	ret = setup();
	if (ret != 0) {
		LOG_ERR("I2S setup failed: %d\n", ret);
		return;
	}

	atomic_inc(&started_threads);

	LOG_INF("I2S load thread started");
	while (1) {
		ret = k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("TX mem slab allocation failed: %d", ret);
			continue;
		}

		fill_tx_buffer((int16_t *)tx_block);

		ret = i2s_write(i2s_dev, tx_block, BLOCK_SIZE);
		if (ret != 0) {
			LOG_ERR("i2s_write failed: %d", ret);
		}

		ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
		if (ret != 0) {
			LOG_ERR("RX/TX START trigger failed: %d", ret);
		}

		ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
		if (ret != 0) {
			LOG_ERR("RX/TX DRAIN trigger failed: %d", ret);
		}

		ret = i2s_read(i2s_dev, rx_block, &rx_size);
		if (ret != 0) {
			LOG_ERR("i2s_read failed: %d", ret);
		}

		ret = verify_rx_buffer((int16_t *)rx_block[0]);
		if (ret != 0) {
			LOG_ERR("TX data does not match RX data: %d", ret);
		}

		k_mem_slab_free(&rx_mem_slab, rx_block[0]);

		k_msleep(I2S_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(i2s_load_thread, I2S_THREAD_STACKSIZE, i2s_load_thread_worker, NULL, NULL, NULL,
		K_PRIO_PREEMPT(I2S_THREAD_PRIORITY), 0, 0);
