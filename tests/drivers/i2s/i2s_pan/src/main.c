/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/ztest.h>

#define WORDS_COUNT		       CONFIG_TEST_WORDS_COUNT
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

#if (CONFIG_TEST_WORDS_COUNT % 4) > 0
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
	i2s_cfg.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER;

	i2s_cfg.mem_slab = &tx_mem_slab;
	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, -EINVAL, "Driver should detect unsupported data align\n");
}

#else

static void print_data(char *header, uint8_t *data)
{
	TC_PRINT("%s:\n", header);
	for (int i = 0; i < WORDS_COUNT; i++) {
		TC_PRINT("[%u] = %u, ", i, data[i]);
	}
	TC_PRINT("\n");
}

static void fill_buf(uint8_t *tx_block, uint8_t val)
{
	for (int i = 0; i < BLOCK_SIZE; i++) {
		tx_block[i] = i + val;
	}
}

static int verify_buf(int8_t *rx_block, uint8_t val)
{
	int sample_no = BLOCK_SIZE;

#if (CONFIG_TEST_I2S_ALLOWED_DATA_OFFSET > 0)
	static ZTEST_DMEM int offset = -1;

	if (offset < 0) {
		do {
			++offset;
			if (offset > CONFIG_TEST_I2S_ALLOWED_DATA_OFFSET) {
				TC_PRINT("verify_buf: Allowed data offset exceeded\n");
				return -TC_FAIL;
			}
		} while (rx_block[offset] != val);

		TC_PRINT("verify_buf: Using data offset: %d\n", offset);
	}

	rx_block += offset;
	sample_no -= offset;
#endif

	for (int i = 0; i < sample_no; i++) {
		if (rx_block[i] != i + val) {
			TC_PRINT("verify_buf: TX[%d] = %d doesn't match RX[%d] = %d\n",
				 i, i + val, i, rx_block[i]);
			return -TC_FAIL;
		}
	}

	TC_PRINT("verify_buf: RX data match with TX data\n");
	return TC_PASS;
}

/*
 * HMPAN-146
 * PAN conditions:
 * 1. TDM.CONFIG.RXTXEN = 0x0 (Duplex) or 0x1 (RX) AND
 * 2. TDM.PSEL.SDIN[31] = 0x1 (SDIN disconnected)
 * Observed:
 * Received data is valid, even though they should not be.
 * SDIN is always regarded as connected.
 */
ZTEST(i2s_pan, test_hmpan_146_not_observed)
{
	int ret;
	struct i2s_config i2s_cfg = {
		.word_size = 8U,
		.channels = NUMBER_OF_CHANNELS,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.frame_clk_freq = 44100U,
		.block_size = BLOCK_SIZE,
		.timeout = TIMEOUT_MS,
		.options = I2S_OPT_FRAME_CLK_CONTROLLER | I2S_OPT_BIT_CLK_CONTROLLER,
	};
	const uint8_t offest_1 = 11;
	const uint8_t offest_2 = 22;
	void *tx_block;
	void *rx_block;
	size_t rx_size;
	NRF_TDM_Type *p_reg = (NRF_TDM_Type *)DT_REG_ADDR(DT_NODELABEL(i2s_dut));

	/* Configure I2S in Duplex mode (I2S_DIR_BOTH). */
	i2s_cfg.mem_slab = &tx_mem_slab,
	ret = i2s_configure(i2s_dev, I2S_DIR_TX, &i2s_cfg);
	zassert_equal(ret, TC_PASS);
	i2s_cfg.mem_slab = &rx_mem_slab;
	ret = i2s_configure(i2s_dev, I2S_DIR_RX, &i2s_cfg);
	zassert_equal(ret, TC_PASS);

	/* Check behavior when PAN conditions are NOT meet. */

	/* Prepare TX data blocks. */
	ret = k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_FOREVER);
	zassert_equal(ret, 0);
	fill_buf(tx_block, offest_1);
	print_data("TX data", tx_block);

	/* Prefill TX queue. */
	ret = i2s_write(i2s_dev, tx_block, BLOCK_SIZE);
	zassert_equal(ret, 0);

	/* Send data. */
	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");
	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	/* Read data. */
	ret = i2s_read(i2s_dev, &rx_block, &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);
	print_data("RX data", rx_block);

	/* Check that RX contains TX data. */
	TC_PRINT("RX data shall be same as TX data\n");
	ret = verify_buf(rx_block, offest_1);
	zassert_equal(ret, TC_PASS);

	k_mem_slab_free(&tx_mem_slab, tx_block);
	k_mem_slab_free(&rx_mem_slab, rx_block);

	/* >>> Disconnect SDIN (PAN condition) <<< */
	p_reg->PSEL.SDIN =
		p_reg->PSEL.SDIN | TDM_PSEL_SDIN_CONNECT_Disconnected << TDM_PSEL_SDIN_CONNECT_Pos;
	TC_PRINT(">> PAN conditions applied <<\n");

	/* Prepare TX data blocks. */
	ret = k_mem_slab_alloc(&tx_mem_slab, &tx_block, K_FOREVER);
	zassert_equal(ret, 0);
	fill_buf(tx_block, offest_2);
	print_data("TX data", tx_block);

	/* Prefill TX queue. */
	ret = i2s_write(i2s_dev, tx_block, BLOCK_SIZE);
	zassert_equal(ret, 0);

	/* Send data. */
	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");
	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	/* Read data. */
	ret = i2s_read(i2s_dev, &rx_block, &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);
	print_data("RX data", rx_block);

	/* Check that RX data is invalid (SDIN disconnected). */
	TC_PRINT("RX data shall be invalid\n");
	ret = verify_buf(rx_block, offest_2);
	zassert_equal(ret, -TC_FAIL);

	k_mem_slab_free(&tx_mem_slab, tx_block);
	k_mem_slab_free(&rx_mem_slab, rx_block);
}
#endif

ZTEST_SUITE(i2s_pan, NULL, test_setup, NULL, NULL, NULL);
