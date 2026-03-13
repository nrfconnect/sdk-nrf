/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>

#define WORDS_COUNT		       11
#define TIMEOUT_MS		       2000
#define NUMBER_OF_BLOCKS	       1
#define NUMBER_OF_CHANNELS	       1

const struct device *const i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s_dut));

#define SLAB_ALIGN 2
static const int8_t test_data[WORDS_COUNT];

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

void *test_setup(void)
{
	zassert_true(device_is_ready(i2s_dev), "I2S device is not ready");
	return NULL;
}

/*
 * HMPAN-46
 * PAN conditions:
 * If the buffer size is not divisible by 4 the first bytes received will
 * be lost each time the DMA restarts.
 * The number of bytes lost is equal
 * to the difference when rounding the size up.
 */
ZTEST(i2s_pan, test_hmpan_46_workaround)
{
	int ret;
	struct i2s_config i2s_cfg;

	i2s_cfg.word_size = 8U;
	i2s_cfg.channels = NUMBER_OF_CHANNELS;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = 44100;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT_MS;
	i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;

	i2s_cfg.mem_slab = &tx_mem_slab;
	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Driver should detect unsupported data align\n");
}

ZTEST_SUITE(i2s_pan, NULL, test_setup, NULL, NULL, NULL);
