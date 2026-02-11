/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#include <zephyr/drivers/i2s.h>

const struct device *const i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s_dut));
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));

#define SLAB_ALIGN 32
static const int16_t test_data[WORDS_COUNT] = {1,  9,  2,  10, 4,  12, 8,   16,
					       16, 24, 32, 40, 64, 72, 128, 136};

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

static void configure_i2s(const struct device *dev, uint32_t frame_clk_freq)
{
	int ret = 0;
	struct i2s_config i2s_cfg;

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = NUMBER_OF_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = frame_clk_freq;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT_MS;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;

	i2s_cfg.mem_slab = &tx_mem_slab;
	ret += i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	if (ret != 0) {
		printk("Failed to configure I2S TX stream: %d\n", ret);
	}

	i2s_cfg.mem_slab = &rx_mem_slab;
	ret += i2s_configure(i2s_dev, I2S_DIR_RX, &i2s_cfg);
	if (ret != 0) {
		printk("Failed to configure I2S RX stream: %d\n", ret);
	}

	__ASSERT(ret == 0, "I2S configuration failed: %d\n", ret);
}

int main(void)
{

	int ret;
	uint32_t tst_timer_value;
	uint64_t timer_value_us;
	int64_t uptime;
	void *tx_block;

	dk_leds_init();

	/* Zephyr boot done */
	dk_set_led_on(DK_LED1);

	uptime = k_uptime_get();
	printk("Uptime (after boot) [ms]: %llu\n", uptime);

	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	/* Time measurement start */
	counter_reset(tst_timer_dev);
	counter_start(tst_timer_dev);

	/* Configure I2S */
	configure_i2s(i2s_dev, SAMPLE_RATE);

	/* Alloc memory and fill TX buffer */
	ret = k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_FOREVER);
	__ASSERT(ret == 0, "Failed to allocate memory for TDM: %d\n", ret);
	fill_tx_buffer((uint16_t *)tx_block);

	/* Write data to TX queue */
	ret = i2s_write(i2s_dev, tx_block, BLOCK_SIZE);
	__ASSERT(ret == 0, "Failed to write to TX queue: %d\n", ret);

	/* Trigger */
	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
	__ASSERT(ret == 0, "RX/TX START trigger failed: %d\n", ret);

	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	__ASSERT(ret == 0, "RX/TX DRAIN trigger failed: %d\n", ret);

	/* Time measurement stop */
	counter_get_value(tst_timer_dev, &tst_timer_value);
	counter_stop(tst_timer_dev);

	/* I2S ready */
	dk_set_led_on(DK_LED2);
	printk("TDM ready\n");
	uptime = k_uptime_get();

	timer_value_us = counter_ticks_to_us(tst_timer_dev, tst_timer_value);

	printk("[Zephyr] TDM, sample width: 16, words count: %d, block size: %u, blocks: %d, channels: %d, "
	       "sample rate [Hz]: %ld\n",
	       WORDS_COUNT, BLOCK_SIZE, NUMBER_OF_BLOCKS, NUMBER_OF_CHANNELS, SAMPLE_RATE);
	printk("Time from boot to TDM started [us]: %llu\n", timer_value_us);
	printk("Uptime [ms]: %llu\n", uptime);

	return 0;
}
