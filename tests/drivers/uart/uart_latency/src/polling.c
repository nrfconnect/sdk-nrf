/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

extern const struct device *const uart_dev;
extern const struct device *const tst_timer_dev;
extern uint8_t tx_test_buffer[MAX_BUFFER_SIZE];
extern uint8_t rx_test_buffer[MAX_BUFFER_SIZE];

static void test_uart_latency(size_t buffer_size, uint32_t baudrate)
{
	int err;
	uint32_t tst_timer_value;
	uint64_t timer_value_us;
	uint32_t theoretical_transmission_time_us;
	uint32_t maximal_allowed_transmission_time_us;
	struct uart_config test_uart_config;
	uart_test_data test_data;

	test_data.buffer_size = buffer_size;
	test_data.tx_buffer = tx_test_buffer;
	test_data.rx_buffer = rx_test_buffer;
	test_data.buffer_size = buffer_size;

	zassert_true(buffer_size <= MAX_BUFFER_SIZE,
		     "Given buffer size is to big, allowed max %u bytes", MAX_BUFFER_SIZE);

	TC_PRINT("UART TX latency in POLLING mode test with buffer size: %u bytes and baudrate: "
		 "%u\n",
		 buffer_size, baudrate);

	set_test_pattern(&test_data);
	err = uart_config_get(uart_dev, &test_uart_config);
	zassert_equal(err, 0, "Failed to get uart config");
	test_uart_config.baudrate = baudrate;
	err = uart_configure(uart_dev, &test_uart_config);
	zassert_equal(err, 0, "UART configuration failed");

	theoretical_transmission_time_us =
		calculate_theoretical_transsmison_time_us(&test_uart_config, buffer_size);
	maximal_allowed_transmission_time_us = MAX_TOLERANCE * theoretical_transmission_time_us;

	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);
	counter_reset(tst_timer_dev);

	dk_set_led_on(DK_LED1);
	counter_start(tst_timer_dev);
	for (uint32_t counter = 0; counter < buffer_size; counter++) {
		uart_poll_out(uart_dev, test_data.tx_buffer[counter]);
		/* Wait for the character to be avilable in the RX buffer */
		while (uart_poll_in(uart_dev, &test_data.rx_buffer[counter]) == -1) {
		};
		uart_poll_in(uart_dev, &test_data.rx_buffer[counter]);
	}
	counter_get_value(tst_timer_dev, &tst_timer_value);
	counter_stop(tst_timer_dev);
	dk_set_led_off(DK_LED1);
	timer_value_us = counter_ticks_to_us(tst_timer_dev, tst_timer_value);
	check_transmitted_data(&test_data);

	TC_PRINT("Calculated transmission time (for %u bytes) [us]: %u\n", buffer_size,
		 theoretical_transmission_time_us);
	TC_PRINT("Measured transmission time (for %u bytes) [us]: %llu\n", buffer_size,
		 timer_value_us);
	TC_PRINT("Measured - claculated time delta (for %u bytes) [us]: %lld\n", buffer_size,
		 timer_value_us - theoretical_transmission_time_us);
	TC_PRINT("Maximal allowed transmission time [us]: %u\n",
		 maximal_allowed_transmission_time_us);

	zassert_true(timer_value_us < maximal_allowed_transmission_time_us,
		     "Measured call latency is over the specified limit");
}

ZTEST(uart_latency, test_uart_latency_in_polling_mode_baud_9k6)
{
	test_uart_latency(10, UART_BAUD_9k6);
	test_uart_latency(128, UART_BAUD_9k6);
	test_uart_latency(1024, UART_BAUD_9k6);
	test_uart_latency(3000, UART_BAUD_9k6);
}

ZTEST(uart_latency, test_uart_latency_in_polling_mode_baud_115k2)
{
	test_uart_latency(10, UART_BAUD_115k2);
	test_uart_latency(128, UART_BAUD_115k2);
	test_uart_latency(1024, UART_BAUD_115k2);
	test_uart_latency(3000, UART_BAUD_115k2);
}

ZTEST(uart_latency, test_uart_latency_in_polling_mode_baud_921k6)
{
	test_uart_latency(10, UART_BAUD_921k6);
	test_uart_latency(128, UART_BAUD_921k6);
	test_uart_latency(1024, UART_BAUD_921k6);
	test_uart_latency(3000, UART_BAUD_921k6);
}

void *test_setup(void)
{
	zassert_true(device_is_ready(uart_dev), "UART device is not ready");
	zassert_equal(dk_leds_init(), 0, "DK leds init failed");

	dk_set_led_off(DK_LED1);
	return NULL;
}

ZTEST_SUITE(uart_latency, NULL, test_setup, NULL, NULL, NULL);
