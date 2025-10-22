/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2s, LOG_LEVEL_INF);

#include <zephyr/drivers/i2s.h>
#include "common.h"

#if DT_NODE_HAS_STATUS(DT_ALIAS(i2s_node0), okay)

static const struct device *dev_i2s = DEVICE_DT_GET_OR_NULL(DT_ALIAS(i2s_node0));

#define SAMPLE_NO	32
#define FRAME_CLK_FREQ	8000
#define TIMEOUT		2000
#define NUM_RX_BLOCKS	4
#define NUM_TX_BLOCKS	4

/* The data_l represent a sine wave */
static int16_t data_l[SAMPLE_NO] = {
	  6392,  12539,  18204,  23169,  27244,  30272,  32137,  32767,  32137,
	 30272,  27244,  23169,  18204,  12539,   6392,      0,  -6393, -12540,
	-18205, -23170, -27245, -30273, -32138, -32767, -32138, -30273, -27245,
	-23170, -18205, -12540,  -6393,     -1,
};

/* The data_r represent a sine wave with double the frequency of data_l */
static int16_t data_r[SAMPLE_NO] = {
	 12539,  23169,  30272,  32767,  30272,  23169,  12539,      0, -12540,
	-23170, -30273, -32767, -30273, -23170, -12540,     -1,  12539,  23169,
	 30272,  32767,  30272,  23169,  12539,      0, -12540, -23170, -30273,
	-32767, -30273, -23170, -12540,     -1,
};

#define BLOCK_SIZE (2 * sizeof(data_l))

K_MEM_SLAB_DEFINE(rx_mem_slab, BLOCK_SIZE, NUM_RX_BLOCKS, 32);
K_MEM_SLAB_DEFINE(tx_mem_slab, BLOCK_SIZE, NUM_TX_BLOCKS, 32);


static void fill_buf(int16_t *tx_block, int att)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = data_l[i] >> att;
		tx_block[2 * i + 1] = data_r[i] >> att;
	}
}

static int configure_stream(const struct device *dev_i2s, enum i2s_dir dir)
{
	int ret;
	struct i2s_config i2s_cfg;

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = FRAME_CLK_FREQ;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT;

	if (dir == I2S_DIR_TX) {
		/* Configure the Transmit port as Master */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER
				| I2S_OPT_BIT_CLK_MASTER;
	} else if (dir == I2S_DIR_RX) {
		/* Configure the Receive port as Slave */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE
				| I2S_OPT_BIT_CLK_SLAVE;
	} else { /* dir == I2S_DIR_BOTH */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER
				| I2S_OPT_BIT_CLK_MASTER;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &tx_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
		if (ret < 0) {
			LOG_ERR("Failed to configure I2S TX stream (%d)", ret);
			return -1;
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &rx_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
		if (ret < 0) {
			LOG_ERR("Failed to configure I2S RX stream (%d)", ret);
			return -1;
		}
	}

	return 0;
}

static int tx_block_write(const struct device *dev_i2s, int att, int err)
{
	char tx_block[BLOCK_SIZE];
	int ret;

	fill_buf((uint16_t *)tx_block, att);
	ret = i2s_buf_write(dev_i2s, tx_block, BLOCK_SIZE);
	if (ret != err) {
		LOG_ERR("i2s_buf_write failed (%d)", ret);
		return ret;
	}

	return 0;
}

static int rx_block_read(const struct device *dev_i2s, int att)
{
	char rx_block[BLOCK_SIZE];	/* Data is received but left unalyzed */
	size_t rx_size;
	int ret;

	ret = i2s_buf_read(dev_i2s, rx_block, &rx_size);
	if (ret < 0 || rx_size != BLOCK_SIZE) {
		LOG_ERR("i2s_buf_read() failed (%d)", ret);
		return ret;
	}
	LOG_DBG("i2s_buf_read() received %d", rx_size);

	return 0;
}

/* I2S thread */
static void i2s_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;

	atomic_inc(&started_threads);

	if (!device_is_ready(dev_i2s)) {
		LOG_ERR("Device %s is not ready.", dev_i2s->name);
		atomic_inc(&completed_threads);
		return;
	}

	/* Configure I2S */
	ret = configure_stream(dev_i2s, I2S_DIR_BOTH);
	if (ret < 0) {
		atomic_inc(&completed_threads);
		return;
	}

	for (int i = 0; i < I2S_THREAD_COUNT_MAX; i++) {
		/* Drop??? previous communication */
		ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
		if (ret < 0) {
			LOG_ERR("i2s_trigger(I2S_TRIGGER_DROP) TX/RX failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}

		/* Prefill TX queue */
		ret = tx_block_write(dev_i2s, 0, 0);
		if (ret != 0) {
			LOG_ERR("tx_block_write(0) failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}
		LOG_DBG("TX 1 - OK");

		ret = tx_block_write(dev_i2s, 1, 0);
		if (ret != 0) {
			LOG_ERR("tx_block_write(1) failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}
		LOG_DBG("TX 2 - OK");

		ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
		if (ret < 0) {
			LOG_ERR("i2s_trigger(I2S_TRIGGER_START) TX/RX failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}

		ret = rx_block_read(dev_i2s, 0);
		if (ret < 0) {
			LOG_ERR("rx_block_read(0) failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}
		LOG_DBG("RX 1 - OK");

		ret = tx_block_write(dev_i2s, 2, 0);
		if (ret != 0) {
			LOG_ERR("tx_block_write(2) failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}
		LOG_DBG("TX 3 - OK");

		/* All data written, drain TX queue and stop both streams. */
		ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
		if (ret < 0) {
			LOG_ERR("i2s_trigger(I2S_TRIGGER_DRAIN) TX/RX failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}

		ret = rx_block_read(dev_i2s, 1);
		if (ret < 0) {
			LOG_ERR("rx_block_read(1) failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}
		LOG_DBG("RX 2 - OK");

		ret = rx_block_read(dev_i2s, 2);
		if (ret < 0) {
			LOG_ERR("rx_block_read(2) failed (%d)", ret);
			atomic_inc(&completed_threads);
			return;
		}
		LOG_DBG("RX 3 - OK");

		LOG_INF("I2S data was sent");
		k_msleep(I2S_THREAD_SLEEP);
	}

	LOG_INF("I2S thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_i2s_id, I2S_THREAD_STACKSIZE, i2s_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(I2S_THREAD_PRIORITY), 0, 0);

#else
#pragma message("I2S thread skipped due to missing node in the DTS")
#endif
