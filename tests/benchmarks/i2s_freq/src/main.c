/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/iterable_sections.h>

#define I2S_DEV_NODE DT_ALIAS(i2s_node0)

#define NUM_BLOCKS 20
#define SAMPLE_NO  64

/* The data_l represent a sine wave */
static int16_t data_l[SAMPLE_NO] = {
	3211,	6392,	9511,	12539,	15446,	18204,	20787,	23169,	25329,	27244,	28897,
	30272,	31356,	32137,	32609,	32767,	32609,	32137,	31356,	30272,	28897,	27244,
	25329,	23169,	20787,	18204,	15446,	12539,	9511,	6392,	3211,	0,	-3212,
	-6393,	-9512,	-12540, -15447, -18205, -20788, -23170, -25330, -27245, -28898, -30273,
	-31357, -32138, -32610, -32767, -32610, -32138, -31357, -30273, -28898, -27245, -25330,
	-23170, -20788, -18205, -15447, -12540, -9512,	-6393,	-3212,	-1,
};

/* The data_r represent a sine wave shifted by 90 deg to data_l sine wave */
static int16_t data_r[SAMPLE_NO] = {
	32609,	32137,	31356,	30272,	28897,	27244,	25329,	23169,	20787,	18204,	15446,
	12539,	9511,	6392,	3211,	0,	-3212,	-6393,	-9512,	-12540, -15447, -18205,
	-20788, -23170, -25330, -27245, -28898, -30273, -31357, -32138, -32610, -32767, -32610,
	-32138, -31357, -30273, -28898, -27245, -25330, -23170, -20788, -18205, -15447, -12540,
	-9512,	-6393,	-3212,	-1,	3211,	6392,	9511,	12539,	15446,	18204,	20787,
	23169,	25329,	27244,	28897,	30272,	31356,	32137,	32609,	32767,
};

#define BLOCK_SIZE (2 * sizeof(data_l))

#ifdef CONFIG_NOCACHE_MEMORY
#define MEM_SLAB_CACHE_ATTR __nocache
#else
#define MEM_SLAB_CACHE_ATTR
#endif /* CONFIG_NOCACHE_MEMORY */

/*
 * NUM_BLOCKS is the number of blocks used by the test. Some of the drivers,
 * e.g. i2s_mcux_flexcomm, permanently keep ownership of a few RX buffers. Add a few more
 * RX blocks to satisfy this requirement
 */

char MEM_SLAB_CACHE_ATTR
	__aligned(WB_UP(32)) _k_mem_slab_buf_rx_0_mem_slab[(NUM_BLOCKS + 2) * WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab,
			rx_0_mem_slab) = Z_MEM_SLAB_INITIALIZER(rx_0_mem_slab,
								_k_mem_slab_buf_rx_0_mem_slab,
								WB_UP(BLOCK_SIZE), NUM_BLOCKS + 2);

char MEM_SLAB_CACHE_ATTR
	__aligned(WB_UP(32)) _k_mem_slab_buf_tx_0_mem_slab[(NUM_BLOCKS)*WB_UP(BLOCK_SIZE)];
STRUCT_SECTION_ITERABLE(k_mem_slab,
			tx_0_mem_slab) = Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab,
								_k_mem_slab_buf_tx_0_mem_slab,
								WB_UP(BLOCK_SIZE), NUM_BLOCKS);

static const struct device *dev_i2s;

static const struct device *const timer_dev = DEVICE_DT_GET(DT_NODELABEL(timer_dev));
static const struct gpio_dt_spec frame_clk_sense_gpio =
	GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), test_gpios, {0});
#define NUMBER_OF_FRAMES_FOR_MEASURE_FREQUENCY (2 * SAMPLE_NO - 8)
static struct gpio_callback gpio_cb;
static volatile uint32_t edge_counter, elapsed_ticks;
static K_SEM_DEFINE(measurement_done, 0, 1);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static void fill_buf(int16_t *tx_block, int att)
{
	for (int i = 0; i < SAMPLE_NO; i++) {
		tx_block[2 * i] = data_l[i] >> att;
		tx_block[2 * i + 1] = data_r[i] >> att;
	}
}

static int verify_buf(int16_t *rx_block, int att)
{
	int sample_no = SAMPLE_NO;

#if (CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET > 0)
	static ZTEST_DMEM int offset = -1;

	if (offset < 0) {
		do {
			++offset;
			if (offset > CONFIG_I2S_TEST_ALLOWED_DATA_OFFSET) {
				TC_PRINT("Allowed data offset exceeded\n");
				return -TC_FAIL;
			}
		} while (rx_block[2 * offset] != data_l[0] >> att);

		TC_PRINT("Using data offset: %d\n", offset);
	}

	rx_block += 2 * offset;
	sample_no -= offset;
#endif

	for (int i = 0; i < sample_no; i++) {
		if (rx_block[2 * i] != data_l[i] >> att) {
			TC_PRINT("Error: att %d: data_l mismatch at position "
				 "%d, expected %d, actual %d\n",
				 att, i, data_l[i] >> att, rx_block[2 * i]);
			return -TC_FAIL;
		}
		if (rx_block[2 * i + 1] != data_r[i] >> att) {
			TC_PRINT("Error: att %d: data_r mismatch at position "
				 "%d, expected %d, actual %d\n",
				 att, i, data_r[i] >> att, rx_block[2 * i + 1]);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

#define TIMEOUT 2000

static int configure_stream(const struct device *dev_i2s, enum i2s_dir dir, uint32_t frame_clk_freq)
{
	int ret;
	struct i2s_config i2s_cfg;

	i2s_cfg.word_size = 16U;
	i2s_cfg.channels = 2U;
	i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
	i2s_cfg.frame_clk_freq = frame_clk_freq;
	i2s_cfg.block_size = BLOCK_SIZE;
	i2s_cfg.timeout = TIMEOUT;

	if (dir == I2S_DIR_TX) {
		/* Configure the Transmit port as Master */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	} else if (dir == I2S_DIR_RX) {
		/* Configure the Receive port as Slave */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_SLAVE | I2S_OPT_BIT_CLK_SLAVE;
	} else { /* dir == I2S_DIR_BOTH */
		i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
	}

	if (dir == I2S_DIR_TX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &tx_0_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S TX stream (%d)\n", ret);
			return -TC_FAIL;
		}
	}

	if (dir == I2S_DIR_RX || dir == I2S_DIR_BOTH) {
		i2s_cfg.mem_slab = &rx_0_mem_slab;
		ret = i2s_configure(dev_i2s, I2S_DIR_RX, &i2s_cfg);
		if (ret < 0) {
			TC_PRINT("Failed to configure I2S RX stream (%d)\n", ret);
			return -TC_FAIL;
		}
	}

	return TC_PASS;
}

void start_frame_clock_measurement(void)
{
	edge_counter = 0;
	elapsed_ticks = 0;

	if (IS_ENABLED(CONFIG_I2S_TEST_FRAME_CLOCK_MEASUREMENT_GPIO_POLLING)) {
		while (gpio_pin_get_dt(&frame_clk_sense_gpio) == 1) {
		}
		while (gpio_pin_get_dt(&frame_clk_sense_gpio) == 0) {
		}
		gpio_pin_set_dt(&led, 1);
		counter_start(timer_dev);
		while (edge_counter < NUMBER_OF_FRAMES_FOR_MEASURE_FREQUENCY) {
			while (gpio_pin_get_dt(&frame_clk_sense_gpio) == 1) {
			}
			while (gpio_pin_get_dt(&frame_clk_sense_gpio) == 0) {
			}
			edge_counter++;
		}
		counter_get_value(timer_dev, (uint32_t *)&elapsed_ticks);
		gpio_pin_set_dt(&led, 0);
		counter_stop(timer_dev);
	} else {
		gpio_add_callback_dt(&frame_clk_sense_gpio, &gpio_cb);
	}
}

void frame_clock_edge_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (edge_counter == 0) {
		counter_start(timer_dev);
		gpio_pin_set_dt(&led, 1);
	}

	if (++edge_counter > NUMBER_OF_FRAMES_FOR_MEASURE_FREQUENCY) {
		counter_get_value(timer_dev, (uint32_t *)&elapsed_ticks);
		gpio_pin_set_dt(&led, 0);
		gpio_remove_callback_dt(&frame_clk_sense_gpio, &gpio_cb);
		counter_stop(timer_dev);
		k_sem_give(&measurement_done);
	}
}

void verify_frame_clock_frequency(uint32_t expected_freq)
{
	uint32_t measured_freq;
	uint32_t deviation;
	uint32_t elapsed_us;

	if (!IS_ENABLED(CONFIG_I2S_TEST_FRAME_CLOCK_MEASUREMENT_GPIO_POLLING)) {
		zassert_ok(k_sem_take(&measurement_done, K_SECONDS(1)),
			   "Frame clock frequency measurement timeout");
	}

	elapsed_us = counter_ticks_to_us(timer_dev, elapsed_ticks);
	measured_freq = NUMBER_OF_FRAMES_FOR_MEASURE_FREQUENCY * 1000000 / elapsed_us;
	deviation =
		(uint32_t)(((uint64_t)(expected_freq)*CONFIG_I2S_TEST_FRAME_CLOCK_TOLERANCE_PPM) /
			   1000000);

	TC_PRINT("%d frames captured in %d[us] --> measured frequency: %d[Hz]\n",
		 NUMBER_OF_FRAMES_FOR_MEASURE_FREQUENCY, elapsed_us, measured_freq);
	zassert_between_inclusive(
		measured_freq, expected_freq - deviation, expected_freq + deviation,
		"Measured frame clock frequency out of tolerance (allowed deviation: %d)",
		deviation);
}

static void i2s_transfer_short(uint32_t frame_clk_freq)
{
	void *rx_block[3];
	void *tx_block;
	size_t rx_size;
	int ret;

	/* Configure I2S Dir Both transfer. */
	ret = configure_stream(dev_i2s, I2S_DIR_BOTH, frame_clk_freq);
	zassert_equal(ret, TC_PASS);

	/* Prefill TX queue */
	for (int i = 0; i < 3; i++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block, K_FOREVER);
		zassert_equal(ret, 0);
		fill_buf((uint16_t *)tx_block, i);

		ret = i2s_write(dev_i2s, tx_block, BLOCK_SIZE);
		zassert_equal(ret, 0);
	}

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	start_frame_clock_measurement();

	ret = i2s_read(dev_i2s, &rx_block[0], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	ret = i2s_read(dev_i2s, &rx_block[1], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	ret = i2s_read(dev_i2s, &rx_block[2], &rx_size);
	zassert_equal(ret, 0);
	zassert_equal(rx_size, BLOCK_SIZE);

	/* Verify received data */
	ret = verify_buf((uint16_t *)rx_block[0], 0);
	zassert_equal(ret, 0);
	k_mem_slab_free(&rx_0_mem_slab, rx_block[0]);

	ret = verify_buf((uint16_t *)rx_block[1], 1);
	zassert_equal(ret, 0);
	k_mem_slab_free(&rx_0_mem_slab, rx_block[1]);

	ret = verify_buf((uint16_t *)rx_block[2], 2);
	zassert_equal(ret, 0);
	k_mem_slab_free(&rx_0_mem_slab, rx_block[2]);

	verify_frame_clock_frequency(frame_clk_freq);
}

/** @brief Short I2S transfer using I2S_DIR_BOTH and sample rate of 8000.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a short sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(i2s_freq, test_i2s_transfer_short_08000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_SKIP_SAMPLERATE_8000);

	i2s_transfer_short(8000);
}

/** @brief Short I2S transfer using I2S_DIR_BOTH and sample rate of 16000.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a short sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(i2s_freq, test_i2s_transfer_short_16000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_SKIP_SAMPLERATE_16000);

	i2s_transfer_short(16000);
}

/** @brief Short I2S transfer using I2S_DIR_BOTH and sample rate of 32000.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a short sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(i2s_freq, test_i2s_transfer_short_32000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_SKIP_SAMPLERATE_32000);

	i2s_transfer_short(32000);
}

/** @brief Short I2S transfer using I2S_DIR_BOTH and sample rate of 44100.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a short sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(i2s_freq, test_i2s_transfer_short_44100)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_SKIP_SAMPLERATE_44100);

	i2s_transfer_short(44100);
}

/** @brief Short I2S transfer using I2S_DIR_BOTH and sample rate of 48000.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a short sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(i2s_freq, test_i2s_transfer_short_48000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_SKIP_SAMPLERATE_48000);

	i2s_transfer_short(48000);
}

/** @brief Short I2S transfer using I2S_DIR_BOTH and sample rate of 88200.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a short sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(i2s_freq, test_i2s_transfer_short_88200)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_SKIP_SAMPLERATE_88200);

	i2s_transfer_short(88200);
}

/** @brief Short I2S transfer using I2S_DIR_BOTH and sample rate of 96000.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a short sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(i2s_freq, test_i2s_transfer_short_96000)
{
	Z_TEST_SKIP_IFDEF(CONFIG_I2S_TEST_SKIP_SAMPLERATE_96000);

	i2s_transfer_short(96000);
}

/** @brief Long I2S transfer using I2S_DIR_BOTH.
 *
 * - START trigger starts both the transmission and reception.
 * - Sending / receiving a long sequence of data returns success.
 * - DRAIN trigger empties the transmit queue and stops both streams.
 */
ZTEST(i2s_freq, test_i2s_transfer_long_44100)
{
	void *rx_block[NUM_BLOCKS];
	void *tx_block[NUM_BLOCKS];
	size_t rx_size;
	int tx_idx;
	int rx_idx = 0;
	int num_verified;
	int ret;
	uint32_t frame_clk_freq = 44100;

	/* Configure I2S Dir Both transfer. */
	ret = configure_stream(dev_i2s, I2S_DIR_BOTH, frame_clk_freq);
	zassert_equal(ret, TC_PASS);

	/* Prepare TX data blocks */
	for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
		ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx], K_FOREVER);
		zassert_equal(ret, 0);
		fill_buf((uint16_t *)tx_block[tx_idx], tx_idx % 3);
	}

	tx_idx = 0;

	/* Prefill TX queue */
	ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
	zassert_equal(ret, 0);

	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START);
	zassert_equal(ret, 0, "RX/TX START trigger failed\n");

	for (; tx_idx < NUM_BLOCKS;) {
		ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
		zassert_equal(ret, 0);

		ret = i2s_read(dev_i2s, &rx_block[rx_idx++], &rx_size);
		zassert_equal(ret, 0);
		zassert_equal(rx_size, BLOCK_SIZE);
	}

	/* All data written, drain TX queue and stop both streams. */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DRAIN);
	zassert_equal(ret, 0, "RX/TX DRAIN trigger failed");

	start_frame_clock_measurement();

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
		ret = verify_buf((uint16_t *)rx_block[rx_idx], rx_idx % 3);
		if (ret != 0) {
			TC_PRINT("%d RX block invalid\n", rx_idx);
		} else {
			num_verified++;
		}
		k_mem_slab_free(&rx_0_mem_slab, rx_block[rx_idx]);
	}
	zassert_equal(num_verified, NUM_BLOCKS, "Invalid RX blocks received");

	verify_frame_clock_frequency(frame_clk_freq);
}

static void *test_i2s_freq_configure(void)
{
	int ret;

	/* Configure I2S Dir Both transfer. */
	dev_i2s = DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE);
	zassert_not_null(dev_i2s, "receive device not found");
	zassert(device_is_ready(dev_i2s), "receive device not ready");

	ret = configure_stream(dev_i2s, I2S_DIR_BOTH, 44100);
	zassert_equal(ret, TC_PASS);

	zassert_true(device_is_ready(frame_clk_sense_gpio.port));
	ret = gpio_pin_configure_dt(&frame_clk_sense_gpio, GPIO_INPUT);
	zassert_ok(ret, "Configure frame clock sense (input GPIO) failed");
	if (!IS_ENABLED(CONFIG_I2S_TEST_FRAME_CLOCK_MEASUREMENT_GPIO_POLLING)) {
		ret = gpio_pin_interrupt_configure_dt(&frame_clk_sense_gpio, GPIO_INT_EDGE_RISING);
		zassert_ok(ret, "Configure interrupt for frame clock sense failed");
		gpio_init_callback(&gpio_cb, frame_clock_edge_callback,
				   BIT(frame_clk_sense_gpio.pin));
	}
	zassert_true(device_is_ready(timer_dev));
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	return 0;
}

ZTEST_SUITE(i2s_freq, NULL, test_i2s_freq_configure, NULL, NULL, NULL);
