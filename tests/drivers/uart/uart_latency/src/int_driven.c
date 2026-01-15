/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

#define TX_TIMEOUT_MS 500

extern const struct device *const uart_dev;
extern const struct device *const tst_timer_dev;
extern uint8_t tx_test_buffer[MAX_BUFFER_SIZE];
extern uint8_t rx_test_buffer[MAX_BUFFER_SIZE];

static uint32_t tx_byte_offset;
static uint32_t rx_byte_offset;

K_SEM_DEFINE(uart_tx_done_sem, 0, 1);
K_SEM_DEFINE(uart_rx_done_sem, 0, 1);

void uart_tx_interrupt_service(const struct device *dev, uint8_t *test_pattern, size_t buffer_size,
			       uint32_t *tx_byte_offset)
{
	uint8_t bytes_sent = 0;
	uint8_t *tx_data_pointer = (uint8_t *)(test_pattern + *tx_byte_offset);

	if (*tx_byte_offset < buffer_size) {
		bytes_sent = uart_fifo_fill(dev, tx_data_pointer, 1);
		*tx_byte_offset += bytes_sent;
	} else {
		*tx_byte_offset = 0;
		k_sem_give(&uart_tx_done_sem);
		uart_irq_tx_disable(dev);
	}
}

void uart_rx_interrupt_service(const struct device *dev, uint8_t *receive_buffer_pointer,
			       size_t buffer_size, uint32_t *rx_byte_offset)
{
	int rx_data_length = 0;

	do {
		rx_data_length =
			uart_fifo_read(dev, receive_buffer_pointer + *rx_byte_offset, buffer_size);
		*rx_byte_offset += rx_data_length;
	} while (rx_data_length);
	if (*rx_byte_offset >= buffer_size) {
		*rx_byte_offset = 0;
		k_sem_give(&uart_rx_done_sem);
		uart_irq_rx_disable(uart_dev);
	}
}

void uart_isr_handler(const struct device *dev, void *user_data)
{
	uart_test_data *test_data = (uart_test_data *)user_data;

	uart_irq_update(dev);
	while (uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uart_rx_interrupt_service(dev, test_data->rx_buffer, test_data->buffer_size,
						  &rx_byte_offset);
		}
		if (uart_irq_tx_ready(dev)) {
			uart_tx_interrupt_service(dev, test_data->tx_buffer, test_data->buffer_size,
						  &tx_byte_offset);
		}
	}
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

	test_data.buffer_size = buffer_size;
	test_data.tx_buffer = tx_test_buffer;
	test_data.rx_buffer = rx_test_buffer;
	test_data.buffer_size = buffer_size;

	tx_byte_offset = 0;
	rx_byte_offset = 0;

	zassert_true(buffer_size <= MAX_BUFFER_SIZE,
		     "Given buffer size is to big, allowed max %u bytes\n",
		     MAX_BUFFER_SIZE);

	TC_PRINT("UART TX latency in INTERRUPT mode test with buffer size: %u bytes and baudrate: "
		 "%u\n",
		 buffer_size, baudrate);

	set_test_pattern(&test_data);
	configure_test_timer(tst_timer_dev, TEST_TIMER_COUNT_TIME_LIMIT_MS);

	err = uart_config_get(uart_dev, &test_uart_config);
	zassert_equal(err, 0, "Failed to get uart config");
	test_uart_config.baudrate = baudrate;
	err = uart_configure(uart_dev, &test_uart_config);
	zassert_equal(err, 0, "UART configuration failed");
	err = uart_irq_callback_set(uart_dev, uart_isr_handler);
	zassert_equal(err, 0, "Unexpected error when setting callback for LPUART %d", err);
	err = uart_irq_callback_user_data_set(uart_dev, uart_isr_handler, (void *)&test_data);
	zassert_equal(err, 0, "Unexpected error when setting user data for callback %d", err);

	theoretical_transmission_time_us =
		calculate_theoretical_transsmison_time_us(&test_uart_config, buffer_size);
	maximal_allowed_transmission_time_us = MAX_TOLERANCE * theoretical_transmission_time_us;

	for (uint32_t repeat_counter = 0; repeat_counter < MEASUREMENT_REPEATS; repeat_counter++) {
		memset(rx_test_buffer, 0xFF, MAX_BUFFER_SIZE);
		uart_irq_rx_enable(uart_dev);
		uart_irq_tx_enable(uart_dev);

		counter_reset(tst_timer_dev);
		counter_start(tst_timer_dev);
		while (k_sem_take(&uart_rx_done_sem, K_NO_WAIT) != 0) {
		};
		counter_get_value(tst_timer_dev, &tst_timer_value);
		counter_stop(tst_timer_dev);
		timer_value_us[repeat_counter] =
			counter_ticks_to_us(tst_timer_dev, tst_timer_value);
		average_timer_value_us += timer_value_us[repeat_counter];

		uart_irq_rx_disable(uart_dev);
		uart_irq_tx_disable(uart_dev);
		check_transmitted_data(&test_data);
	}

	average_timer_value_us /= MEASUREMENT_REPEATS;

	TC_PRINT("Calculated transmission time (for %u bytes) [us]: %u\n", buffer_size,
		 theoretical_transmission_time_us);
	TC_PRINT("Measured transmission time (for %u bytes) [us]: %llu\n", buffer_size,
		 average_timer_value_us);
	TC_PRINT("Measured - claculated time delta (for %u bytes) [us]: %lld\n", buffer_size,
		 average_timer_value_us - theoretical_transmission_time_us);
	TC_PRINT("Maximal allowed transmission time [us]: %u\n",
		 maximal_allowed_transmission_time_us);

	zassert_true(average_timer_value_us < maximal_allowed_transmission_time_us,
		     "Measured call latency is over the specified limit");
}

ZTEST(uart_latency, test_uart_latency_in_interrupt_mode_baud_9k6)
{
	test_uart_latency(10, UART_BAUD_9k6);
	test_uart_latency(128, UART_BAUD_9k6);
	test_uart_latency(1024, UART_BAUD_9k6);
	test_uart_latency(3000, UART_BAUD_9k6);
}

ZTEST(uart_latency, test_uart_latency_in_interrupt_mode_baud_115k2)
{
	test_uart_latency(10, UART_BAUD_115k2);
	test_uart_latency(128, UART_BAUD_115k2);
	test_uart_latency(1024, UART_BAUD_115k2);
	test_uart_latency(3000, UART_BAUD_115k2);
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
