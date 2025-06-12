/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define DEVICE_DT_GET_AND_COMMA(node_id) DEVICE_DT_GET(node_id),
/* Generate a list of devices for all instances of the "compat" */
#define DEVS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_AND_COMMA)

static const struct device *const devices[] = {
#ifdef CONFIG_I2S_NRF_TDM
	DEVS_FOR_DT_COMPAT(nordic_nrf_tdm)
#endif
#ifdef CONFIG_I2S_NRFX
	DEVS_FOR_DT_COMPAT(nordic_nrf_i2s)
#endif
};

typedef void (*test_func_t)(const struct device *dev);
typedef bool (*capability_func_t)(const struct device *dev);

#define WORD_SIZE 16U
#define NUMBER_OF_CHANNELS 2
#define FRAME_CLK_FREQ 44100

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

/* Fill in TX buffer with test samples. */
static void fill_buf(int16_t *tx_block)
{
	for (int i = 0; i < WORDS_COUNT; i++) {
		tx_block[i] = data[i];
	}
}

static void setup_instance(const struct device *dev)
{
	/* Left for future test expansion. */
}

static void tear_down_instance(const struct device *dev)
{
	/* Left for future test expansion. */
}

static void test_all_instances(test_func_t func, capability_func_t capability_check)
{
	int devices_skipped = 0;

	zassert_true(ARRAY_SIZE(devices) > 0, "No device found");
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		setup_instance(devices[i]);
		TC_PRINT("\nInstance %u: ", i + 1);
		if ((capability_check == NULL) ||
		     capability_check(devices[i])) {
			TC_PRINT("Testing %s\n", devices[i]->name);
			func(devices[i]);
		} else {
			TC_PRINT("Skipped for %s\n", devices[i]->name);
			devices_skipped++;
		}
		tear_down_instance(devices[i]);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
	if (devices_skipped == ARRAY_SIZE(devices)) {
		ztest_test_skip();
	}
}


/**
 * Test if instance can initiate data transmission.
 */
static void test_transmit_any_data_instance(const struct device *dev)
{
	struct i2s_config i2s_cfg;
	void *rx_block[1];
	void *tx_block;
	size_t rx_size;
	int ret;

	i2s_cfg.word_size = WORD_SIZE;
	i2s_cfg.channels = NUMBER_OF_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER
				| I2S_OPT_BIT_CLK_MASTER | I2S_OPT_BIT_CLK_GATED;

	i2s_cfg.mem_slab = &tx_0_mem_slab;
	ret = i2s_configure(dev, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(0, ret, "%s: Failed to configure I2S TX stream", dev->name);

	i2s_cfg.mem_slab = &rx_0_mem_slab;
	ret = i2s_configure(dev, I2S_DIR_RX, &i2s_cfg);
	zassert_equal(0, ret, "%s: Failed to configure I2S RX stream", dev->name);

	/* Prefill TX queue */
	ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
	zassert_equal(ret, 0);
	fill_buf((uint16_t *)tx_block);

	ret = i2s_write(dev, tx_block, BLOCK_SIZE);
	zassert_equal(ret, 0);

	TC_PRINT("TX starts\n");

	ret = i2s_trigger(dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = i2s_read(dev, &rx_block[0], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	TC_PRINT("RX done\n");

	/* Test doesn't check received data. */
	k_mem_slab_free(&rx_0_mem_slab, rx_block[0]);
}

static bool test_transmit_any_data_capable(const struct device *dev)
{
	return true;
}

ZTEST(i2s_instances, test_transmit_any_data)
{
	test_all_instances(test_transmit_any_data_instance, test_transmit_any_data_capable);
}

static void *suite_setup(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devices); i++) {
		zassert_true(device_is_ready(devices[i]),
			     "Device %s is not ready", devices[i]->name);
		k_object_access_grant(devices[i], k_current_get());
	}

	return NULL;
}

ZTEST_SUITE(i2s_instances, NULL, suite_setup, NULL, NULL, NULL);
