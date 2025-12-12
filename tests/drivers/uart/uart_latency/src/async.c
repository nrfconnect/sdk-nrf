/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

static K_SEM_DEFINE(uart_rx_ready_sem, 0, 1);
static K_SEM_DEFINE(uart_tx_done_sem, 0, 1);

extern const struct device *const uart_dev;
extern const struct device *const tst_timer_dev;
extern uint8_t tx_test_buffer[MAX_BUFFER_SIZE];
extern uint8_t rx_test_buffer[MAX_BUFFER_SIZE];

static void async_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&uart_tx_done_sem);
		break;
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
		k_sem_give(&uart_rx_ready_sem);
		break;
	case UART_RX_BUF_RELEASED:
		break;
	case UART_RX_BUF_REQUEST:
		break;
	case UART_RX_DISABLED:
		break;
	default:
		break;
	}
}

static void enable_uart_rx(uart_test_data *test_data)
{
	int err;

	do {
		err = uart_rx_enable(uart_dev, test_data->rx_buffer, test_data->buffer_size,
				     UART_TIMEOUT_US);
	} while (err == -EBUSY);
	zassert_equal(err, 0, "Unexpected error when enabling UART RX: %d");
}

static void test_uart_latency(size_t buffer_size, uint32_t baudrate)
{
	int err;
	uint32_t tst_timer_value;
	uint64_t timer_value_us[MEASUREMENT_REPEATS];
	uint64_t average_timer_value_us = 0;
	uint32_t theoretical_transmission_time_us;
	uint32_t maximal_allowed_transmission_time_us;
	struct uart_config test_uart_config;
	uart_test_data test_data;

	test_data.tx_buffer = tx_test_buffer;
	test_data.rx_buffer = rx_test_buffer;
	test_data.buffer_size = buffer_size;

	zassert_true(buffer_size <= MAX_BUFFER_SIZE,
		     "Given buffer size is to big, allowed max %u bytes\n", MAX_BUFFER_SIZE);

	TC_PRINT("UART TX latency in ASYNC mode test with buffer size: %u bytes and baudrate: %u\n",
		 buffer_size, baudrate);
	set_test_pattern(&test_data);
	err = uart_config_get(uart_dev, &test_uart_config);
	zassert_equal(err, 0, "Failed to get uart config");
	test_uart_config.baudrate = baudrate;
	err = uart_configure(uart_dev, &test_uart_config);
	zassert_equal(err, 0, "UART configuration failed");
	err = uart_callback_set(uart_dev, async_uart_callback, NULL);
	zassert_equal(err, 0, "UART callback setup failed");

	theoretical_transmission_time_us =
		calculate_theoretical_transsmison_time_us(&test_uart_config, buffer_size);
	maximal_allowed_transmission_time_us = MAX_TOLERANCE * theoretical_transmission_time_us;

	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	for (uint32_t repeat_counter = 0; repeat_counter < MEASUREMENT_REPEATS; repeat_counter++) {
		memset(rx_test_buffer, 0xFF, buffer_size);
		enable_uart_rx(&test_data);
		counter_reset(tst_timer_dev);
		counter_start(tst_timer_dev);
		err = uart_tx(uart_dev, test_data.tx_buffer, buffer_size, UART_TIMEOUT_US);
		while (k_sem_take(&uart_rx_ready_sem, K_NO_WAIT) != 0) {
		};
		counter_get_value(tst_timer_dev, &tst_timer_value);
		counter_stop(tst_timer_dev);
		timer_value_us[repeat_counter] =
			counter_ticks_to_us(tst_timer_dev, tst_timer_value);
		average_timer_value_us += timer_value_us[repeat_counter];
		zassert_equal(err, 0, "UART transmission failed");
		uart_rx_disable(uart_dev);
		check_transmitted_data(&test_data);
	}

	average_timer_value_us /= MEASUREMENT_REPEATS;

	TC_PRINT("Calculated transmission time (for %u bytes) [us]: %u\n", buffer_size,
		 theoretical_transmission_time_us);
	TC_PRINT("Measured transmission time (for %u bytes) [us]: %llu\n", buffer_size,
		 average_timer_value_us);
	TC_PRINT("Measured - claculated time delta (for %d bytes) [us]: %lld\n", buffer_size,
		 average_timer_value_us - theoretical_transmission_time_us);
	TC_PRINT("Maximal allowed transmission time [us]: %u\n",
		 maximal_allowed_transmission_time_us);

	zassert_true(average_timer_value_us < maximal_allowed_transmission_time_us,
		     "Measured call latency is over the specified limit");
}

ZTEST(uart_latency, test_uart_latency_in_async_mode_baud_9k6)
{
	test_uart_latency(10, UART_BAUD_9k6);
	test_uart_latency(128, UART_BAUD_9k6);
	test_uart_latency(1024, UART_BAUD_9k6);
	test_uart_latency(3000, UART_BAUD_9k6);
}

ZTEST(uart_latency, test_uart_latency_in_async_mode_baud_115k2)
{
	test_uart_latency(10, UART_BAUD_115k2);
	test_uart_latency(128, UART_BAUD_115k2);
	test_uart_latency(1024, UART_BAUD_115k2);
	test_uart_latency(3000, UART_BAUD_115k2);
}

ZTEST(uart_latency, test_uart_latency_in_async_mode_baud_921k6)
{
	test_uart_latency(10, UART_BAUD_921k6);
	test_uart_latency(128, UART_BAUD_921k6);
	test_uart_latency(1024, UART_BAUD_921k6);
	test_uart_latency(3000, UART_BAUD_921k6);
}

#if defined(CONFIG_SOC_NRF54H20_CPUFLPR)
ZTEST(uart_latency, test_uart_latency_in_async_mode_baud_10M)
{
	test_uart_latency(10, UART_BAUD_10M);
	test_uart_latency(128, UART_BAUD_10M);
	test_uart_latency(1024, UART_BAUD_10M);
	test_uart_latency(3000, UART_BAUD_10M);
}
#endif

void *test_setup(void)
{
	zassert_true(device_is_ready(uart_dev), "UART device is not ready");
	TC_PRINT("Platform: %s\n", CONFIG_BOARD_TARGET);

	return NULL;
}

ZTEST_SUITE(uart_latency, NULL, test_setup, NULL, NULL, NULL);
