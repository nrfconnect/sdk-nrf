/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ztest.h>

#define UART_NODE_MODE_A DT_NODELABEL(dut_uart_a)
#define UART_NODE_MODE_B DT_NODELABEL(dut_uart_b)

#define BUFFER_SIZE		256
#define UART_TIMEOUT_US		1000000
#define SEM_CHECK_DEAD_TIME_MS	10
#define TRANSMISSION_TIMEOUT_MS 2000

typedef struct {
	uint8_t *tx_buffer;
	uint8_t *rx_buffer;
	size_t buffer_size;

} uart_test_data;

void set_test_pattern(uart_test_data *test_data);

void async_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data);

void enable_uart_rx(const struct device *const uart_dev, uart_test_data *test_data);

void uart_tx_interrupt_service(const struct device *dev, uint8_t *test_pattern, size_t buffer_size,
			       volatile uint32_t *tx_byte_offset);

void uart_rx_interrupt_service(const struct device *dev, uint8_t *receive_buffer_pointer,
			       size_t buffer_size, volatile uint32_t *rx_byte_offset);

void uart_isr_handler(const struct device *dev, void *user_data);
