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

#define I2S_DEV_NODE DT_ALIAS(i2s_node0)

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

static volatile uint16_t count_mclk, count_lrck;
static struct gpio_callback mclk_cb_data, lrck_cb_data;
static const struct gpio_dt_spec gpio_mclk_spec =
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), gpios, 0);
static const struct gpio_dt_spec gpio_lrck_spec =
	GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), gpios, 1);

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

/* ISR that couns falling edges on the MCLK signal. */
static void gpio_mclk_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(gpio_mclk_spec.pin)) {
		count_mclk++;
	}
}

/* ISR that couns falling edges on the LRCK signal. */
static void gpio_lrck_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(gpio_lrck_spec.pin)) {
		count_lrck++;
	}
}

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
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER
				| I2S_OPT_BIT_CLK_MASTER | I2S_OPT_BIT_CLK_GATED;
	} else if (dir == I2S_DIR_RX) {
		/* Configure the Receive port as Slave */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE
				| I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_BIT_CLK_GATED;
	} else { /* dir == I2S_DIR_BOTH */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER
				| I2S_OPT_BIT_CLK_MASTER | I2S_OPT_BIT_CLK_GATED;
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

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX START trigger failed\n");

	/* No master on the other side, this will timeout. */
	ret = i2s_read(dev_i2s, &rx_block[0], &rx_size);

	ret = i2s_trigger(dev_i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "RX STOP trigger failed");

	k_msleep(2);

	/* Verify number of falling edges on MCLK */
	uint32_t expected = FRAME_CLK_FREQ * NUMBER_OF_CHANNELS * WORD_SIZE * (TIMEOUT / 1000);

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

	TC_PRINT("TX starts\n");

	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "TX START trigger failed\n");

	/* All data written, drain TX queue and stop stream. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_STOP);
	zassert_equal(ret, 0, "TX STOP trigger failed");

	/* Wait until I2S finishes TX. */
	k_msleep(1000);

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

	/* Configure GPIO used to count MCLK edges. */
	ret = gpio_is_ready_dt(&gpio_mclk_spec);
	zassert_equal(ret, true, "gpio_mclk_spec not ready (ret %d)", ret);
	ret = gpio_pin_configure_dt(&gpio_mclk_spec, GPIO_INPUT);
	zassert_equal(ret, 0, "failed to configure input pin gpio_mclk_spec (err %d)", ret);
	ret = gpio_pin_interrupt_configure_dt(&gpio_mclk_spec, GPIO_INT_EDGE_FALLING);
	zassert_equal(ret, 0, "failed to configure gpio_mclk_spec interrupt (err %d)", ret);
	gpio_init_callback(&mclk_cb_data, gpio_mclk_isr, BIT(gpio_mclk_spec.pin));
	gpio_add_callback(gpio_mclk_spec.port, &mclk_cb_data);

	/* Configure GPIO used to count LRCK edges. */
	ret = gpio_is_ready_dt(&gpio_lrck_spec);
	zassert_equal(ret, true, "gpio_lrck_spec not ready (ret %d)", ret);
	ret = gpio_pin_configure_dt(&gpio_lrck_spec, GPIO_INPUT);
	zassert_equal(ret, 0, "failed to configure input pin gpio_lrck_spec (err %d)", ret);
	ret = gpio_pin_interrupt_configure_dt(&gpio_lrck_spec, GPIO_INT_EDGE_FALLING);
	zassert_equal(ret, 0, "failed to configure gpio_lrck_spec interrupt (err %d)", ret);
	gpio_init_callback(&lrck_cb_data, gpio_lrck_isr, BIT(gpio_lrck_spec.pin));
	gpio_add_callback(gpio_lrck_spec.port, &lrck_cb_data);

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
