/*
 * Copyright (c) 2017 comsuisse AG
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>

#define I2S_DEV_NODE DT_ALIAS(i2s_node0)

#define BLOCK_SIZE			CONFIG_I2S_TEST_BUFFER_SIZE

#if defined(CONFIG_COVERAGE)
#define NUM_BLOCKS 2
#else
#define NUM_BLOCKS 4
#endif
#define TIMEOUT 2000

#ifdef CONFIG_NOCACHE_MEMORY
	#define MEM_SLAB_CACHE_ATTR __nocache
#else
	#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

/* Random data with no meaning. */
static uint8_t data[] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	0x10, 0x20, 0x40, 0x80, 0xAA, 0x55, 0x55, 0xAA,
	0xfe, 0xef, 0xfe, 0xee, 0x00, 0x01, 0x02, 0x04,
	0x0e, 0x12, 0x01, 0x80, 0x81, 0x18, 0xAA, 0x55,
};

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

static const struct device *dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE);

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

#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
/* Data offset may differ from test to test if
 * number of audio channels or word size has changed.
 */
static int offset;
#endif

static void fill_buf(uint8_t *tx_block, uint8_t val)
{
	for (int i = 0; i < BLOCK_SIZE; i++) {
		tx_block[i] = data[i] + val;
	}
}

static int verify_buf(uint8_t *rx_block, uint8_t val, uint32_t offset_in_bytes)
{
	int sample_no = BLOCK_SIZE;
	bool data_match = true;

#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
	/* Offset -1 means that offset has to be detected. */
	if (offset < 0) {
		do {
			++offset;
			if (offset > offset_in_bytes) {
				TC_PRINT("Allowed data offset exceeded\n");
				return -TC_FAIL;
			}
		} while (rx_block[offset] != data[0] + val);

		TC_PRINT("Using data offset: %d bytes\n", offset);
	}

	rx_block += offset;
	sample_no -= offset;
#else
	ARG_UNUSED(offset_in_bytes);
#endif

	for (int i = 0; i < sample_no; i++) {
		if (rx_block[i] != (uint8_t) (data[i] + val)) {
			data_match = false;
			break;
		}
	}

	/* Workaround for issue resulting from limited performance when buffer is small.
	 * When new write buffer is not delivered on time, previous buffer is retransmitted.
	 *
	 * (In the test, each buffer is contens of `data` with each byte incresed by `val`.
	 * `val` is equal to TX block number = 0, 1, 2,...)
	 */
	if (!data_match) {
		TC_PRINT("%u RX block: Compare with data sent earlier (workaround).\n", val);
		data_match = true;
		for (int i = 0; i < sample_no; i++) {
			if (rx_block[i] != (uint8_t) (data[i] + val - 1)) {
				data_match = false;
				break;
			}
		}
	}

	if (!data_match) {
		TC_PRINT("Index: Expected | Received\n");
		for (int i = 0; i < sample_no; i++) {
			TC_PRINT("%u: 0x%02X | 0x%02X\n",
				i, (uint8_t) (data[i] + val), rx_block[i]);
		}
		return -TC_FAIL;
	}

	return TC_PASS;
}

static int configure_stream(const struct device *dev, enum i2s_dir dir,
	struct i2s_config *i2s_cfg)
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

static void i2s_dir_both_transfer_long(struct i2s_config *i2s_cfg, uint32_t offset_in_bytes)
{
	void *rx_block[NUM_BLOCKS];
	void *tx_block[NUM_BLOCKS];
	size_t rx_size;
	int tx_idx;
	int rx_idx = 0;
	int num_verified;
	int ret;

	/* Configure I2S Dir Both transfer. */
	ret = configure_stream(dev_i2s, I2S_DIR_BOTH, i2s_cfg);
#if defined(CONFIG_I2S_TEST_BLOCK_SIZE_8_UNSUPPORTED) && (CONFIG_I2S_TEST_BUFFER_SIZE == 8)
	zassert_equal(ret, -EINVAL, "Unexpected result %d", ret);
	TC_PRINT("No communication check due to unsupported buffer size.\n");
	return;
#else
	zassert_equal(ret, TC_PASS);
#endif

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx], K_FOREVER);
		zassert_equal(ret, 0);
		fill_buf((uint8_t *)tx_block[tx_idx], tx_idx);
	}

	tx_idx = 0;

	/* Prefill TX queue */
	ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	while (tx_idx < NUM_BLOCKS) {
		ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
		zassert_equal(ret, 0);

		ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
		zassert_equal(ret, 0, "Got unexpected %d", ret);
		zassert_equal(rx_size, BLOCK_SIZE);
	}

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	TC_PRINT("%d TX blocks sent\n", tx_idx);
	TC_PRINT("%d RX blocks received\n", rx_idx);

	/* Verify received data */
	num_verified = 0;
	for (rx_idx = 0; rx_idx < NUM_BLOCKS; rx_idx++) {
		ret = verify_buf((uint8_t *)rx_block[rx_idx], rx_idx, offset_in_bytes);
		if (ret != 0) {
			TC_PRINT("Validation failed for %d RX block\n", rx_idx);
		} else {
			num_verified++;
		}
		k_mem_slab_free(&rx_0_mem_slab, rx_block[rx_idx]);
	}
	zassert_equal(num_verified, NUM_BLOCKS, "Invalid RX blocks received");
}

static uint32_t check_test_configuration(struct i2s_config *i2s_cfg)
{
	/* Convert max allowed offset in audio samples to bytes of data. */
	uint32_t offset_in_bytes = CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET * i2s_cfg->channels
		* (i2s_cfg->word_size / 8);

	/* Skip test if allowed offset is same or larger than the buffer. */
	if (offset_in_bytes >= CONFIG_I2S_TEST_BUFFER_SIZE) {
		TC_PRINT("Allowed data offset is to big for this test.\n");
		ztest_test_skip();
	}

	return offset_in_bytes;
}

/*
 * 1 audio channel, 8 bit word
 */
ZTEST(i2s_buffer, test_1ch_08bit_at_08000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 8000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_08bit_at_16000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 16000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_08bit_at_32000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 32000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_08bit_at_44100)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 44100;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_08bit_at_48000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 48000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}


/*
 * 2 audio channels, 8 bit word
 */
ZTEST(i2s_buffer, test_2ch_08bit_at_08000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 8000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_08bit_at_16000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 16000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_08bit_at_32000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 32000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_08bit_at_44100)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 44100;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_08bit_at_48000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 8U;
	i2s_cfg.frame_clk_freq = 48000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}


/*
 * 1 audio channel, 16 bit word
 */
ZTEST(i2s_buffer, test_1ch_16bit_at_08000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 8000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_16bit_at_16000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 16000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_16bit_at_32000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 32000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_16bit_at_44100)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 44100;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_16bit_at_48000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 48000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}


/*
 * 2 audio channels, 16 bit word
 */
ZTEST(i2s_buffer, test_2ch_16bit_at_08000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 8000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_16bit_at_16000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 16000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_16bit_at_32000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 32000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_16bit_at_44100)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 44100;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_16bit_at_48000)
{
	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 16U;
	i2s_cfg.frame_clk_freq = 48000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}


/*
 * 1 audio channel, 32 bit word
 */
ZTEST(i2s_buffer, test_1ch_32bit_at_08000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 8000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_32bit_at_16000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 16000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_32bit_at_32000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 32000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_32bit_at_44100)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 44100;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_1ch_32bit_at_48000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 1U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 48000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}


/*
 * 2 audio channels, 32 bit word
 */
ZTEST(i2s_buffer, test_2ch_32bit_at_08000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 8000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_32bit_at_16000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 16000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_32bit_at_32000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 32000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_32bit_at_44100)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 44100;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}

ZTEST(i2s_buffer, test_2ch_32bit_at_48000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_WORD_SIZE_32_BIT_UNSUPPORTED);

	struct i2s_config i2s_cfg = default_i2s_cfg;
	uint32_t offset_in_bytes;

	i2s_cfg.channels = 2U;
	i2s_cfg.word_size = 32U;
	i2s_cfg.frame_clk_freq = 48000;

	offset_in_bytes = check_test_configuration(&i2s_cfg);

	i2s_dir_both_transfer_long(&i2s_cfg, offset_in_bytes);
}


static void *suite_setup(void)
{
	TC_PRINT("I2S buffer size test on %s\n", CONFIG_BOARD_TARGET);
	TC_PRINT("Testing I2S device %s\n", dev_i2s->name);
	TC_PRINT("BUFFER_SIZE = %d\n", CONFIG_I2S_TEST_BUFFER_SIZE);

	/* Check I2S Device. */
	zassert_not_null(dev_i2s, "I2S device not found");
	zassert(device_is_ready(dev_i2s), "I2S device not ready");

	TC_PRINT("===================================================================\n");

	return 0;
}

static void before(void *not_used)
{
	ARG_UNUSED(not_used);

#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
	/* Data offset may differ when test uses I2S
	 * with different configuration.
	 * Force offset callculation for every test.
	 */
	offset = -1;
#endif
}

ZTEST_SUITE(i2s_buffer, NULL, suite_setup, before, NULL, NULL);
