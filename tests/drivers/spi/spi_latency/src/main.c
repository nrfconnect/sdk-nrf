/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/spi.h>
#include <nrfx_spim.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/drivers/counter.h>
#include <dk_buttons_and_leds.h>
#include <math.h>

#define TEST_TIMER_COUNT_TIME_LIMIT_MS 500
#define MEASUREMENT_REPEATS	       CONFIG_MEASUREMENT_REPEATS

#define SPI_MODE                                                                                   \
	(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB |              \
	 SPI_MODE_CPHA | SPI_MODE_CPOL)

#define MAX_TEST_BUFFER_SIZE 2000

static struct spi_dt_spec spim_spec = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi), SPI_MODE);
const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));

#define MEMORY_SECTION(node)                                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                        \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t tx_buffer[MAX_TEST_BUFFER_SIZE] MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi)));
static uint8_t rx_buffer[MAX_TEST_BUFFER_SIZE] MEMORY_SECTION(DT_BUS(DT_NODELABEL(dut_spi)));

static void *test_setup(void)
{
	zassert_true(spi_is_ready_dt(&spim_spec), "SPIM device is not ready");
	zassert_equal(dk_leds_init(), 0, "DK leds init failed");

	return NULL;
}

static void cleanup_buffers(void *nullp)
{
	memset(tx_buffer, 0xFF, MAX_TEST_BUFFER_SIZE);
	memset(rx_buffer, 0xFF, MAX_TEST_BUFFER_SIZE);
}

static void prepare_test_data(uint8_t *buffer, size_t buffer_size)
{
	for (size_t counter = 0; counter < buffer_size; counter++) {
		buffer[counter] = counter + 1;
	}
}

static void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms)
{
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)count_time_ms * 1000);
	counter_cfg.user_data = &counter_cfg;
}

static uint32_t calculate_theoretical_transsmison_time_us(size_t buffer_size,
							  uint32_t spim_frequency)
{
	double ratio = 1000000.0 / (double)spim_frequency;

	return (uint32_t)(ceil(ratio * (double)(buffer_size * 8 + NRF_SPIM_CSNDUR_DEFAULT)));
}

static void assess_measurement_result(uint64_t timer_value_us,
				      uint32_t theoretical_transmission_time_us, size_t buffer_size)
{
	uint64_t maximal_allowed_transmission_time_us;

	if (buffer_size == 1) {
		/* 5000% */
		maximal_allowed_transmission_time_us = (1 + 50) * theoretical_transmission_time_us;
	} else {
		/* 300% */
		maximal_allowed_transmission_time_us = (1 + 3) * theoretical_transmission_time_us;
	}
	zassert_true(timer_value_us < maximal_allowed_transmission_time_us,
		     "Measured transmission call latency is over the specified limit %llu > %llu",
		     timer_value_us, maximal_allowed_transmission_time_us);
}

static void test_spim_transmission_latency(size_t buffer_size)
{
	int err;
	uint32_t tst_timer_value;
	uint64_t timer_value_us[MEASUREMENT_REPEATS];
	uint64_t average_timer_value_us = 0;
	uint32_t theoretical_transmission_time_us;

	struct spi_buf tx_spi_buf = {.buf = tx_buffer, .len = buffer_size};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_buf, .count = 1};

	struct spi_buf rx_spi_buf = {.buf = rx_buffer, .len = buffer_size};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_buf, .count = 1};

	TC_PRINT("SPIM latency in test with buffer size: %u bytes and frequency: %uHz\n",
		 buffer_size, spim_spec.config.frequency);
	TC_PRINT("Measurement repeats: %u\n", MEASUREMENT_REPEATS);

	prepare_test_data(tx_buffer, buffer_size);
	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	theoretical_transmission_time_us =
		calculate_theoretical_transsmison_time_us(buffer_size, spim_spec.config.frequency);

	for (uint32_t repeat_counter = 0; repeat_counter < MEASUREMENT_REPEATS; repeat_counter++) {
		memset(rx_buffer, 0xFF, buffer_size);
		counter_reset(tst_timer_dev);
		dk_set_led_on(DK_LED1);
		counter_start(tst_timer_dev);
		err = spi_transceive_dt(&spim_spec, &tx_spi_buf_set, &rx_spi_buf_set);
		counter_get_value(tst_timer_dev, &tst_timer_value);
		dk_set_led_off(DK_LED1);
		counter_stop(tst_timer_dev);
		timer_value_us[repeat_counter] =
			counter_ticks_to_us(tst_timer_dev, tst_timer_value);
		average_timer_value_us += timer_value_us[repeat_counter];

		zassert_ok(err, "SPI transceive failed");
		zassert_mem_equal(tx_buffer, rx_buffer, buffer_size);
	}

	average_timer_value_us /= MEASUREMENT_REPEATS;

	TC_PRINT("Calculated transmission time (for %u bytes) [us]: %u\n", buffer_size,
		 theoretical_transmission_time_us);
	TC_PRINT("spi_transceive_dt: measured transmission time (for %u bytes) [us]: %llu\n",
		 buffer_size, average_timer_value_us);
	TC_PRINT("spi_transceive_dt: measured - claculated time delta (for %d bytes) [us]: %lld\n",
		 buffer_size, average_timer_value_us - theoretical_transmission_time_us);
	TC_PRINT("Theoretical maximal data rate (for buffer size %u bytes) [B/s]: %llu\n",
		 buffer_size, (uint64_t)(1e6 * buffer_size / theoretical_transmission_time_us));
	TC_PRINT("Calculated data rate (for buffer size %u bytes) [B/s]: %llu\n", buffer_size,
		 (uint64_t)(1e6 * buffer_size / average_timer_value_us));

	assess_measurement_result(average_timer_value_us, theoretical_transmission_time_us,
				  buffer_size);
}

ZTEST(spim_transmission_latency, test_spim_transceive_call_latencty)
{
	test_spim_transmission_latency(1);
	test_spim_transmission_latency(128);
	test_spim_transmission_latency(512);
	test_spim_transmission_latency(2000);
}

ZTEST_SUITE(spim_transmission_latency, NULL, test_setup, NULL, cleanup_buffers, NULL);
