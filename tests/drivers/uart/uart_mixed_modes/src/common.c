/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

K_SEM_DEFINE(uart_async_mode_rx_ready_sem, 0, 1);
K_SEM_DEFINE(uart_interrupt_mode_rx_done_sem, 0, 1);

uint8_t uart_a_tx_test_buffer[BUFFER_SIZE];
uint8_t uart_a_rx_test_buffer[BUFFER_SIZE];
uint8_t uart_b_tx_test_buffer[BUFFER_SIZE];
uint8_t uart_b_rx_test_buffer[BUFFER_SIZE];

volatile uint32_t tx_byte_offset;
volatile uint32_t rx_byte_offset;

void set_test_pattern(uart_test_data *test_data)
{
	for (uint32_t counter = 0; counter < test_data->buffer_size; counter++) {
		test_data->tx_buffer[counter] = (uint8_t)(counter & 0xFF);
		test_data->rx_buffer[counter] = 0xFF;
	}
}

void async_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&uart_async_mode_rx_ready_sem);
		break;
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
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

void enable_uart_rx(const struct device *const uart_dev, uart_test_data *test_data)
{
	int err;

	do {
		err = uart_rx_enable(uart_dev, test_data->rx_buffer, test_data->buffer_size,
				     UART_TIMEOUT_US);
	} while (err == -EBUSY);
	zassert_equal(err, 0, "Unexpected error when enabling UART RX: %d");
}

void uart_tx_interrupt_service(const struct device *dev, uint8_t *test_pattern, size_t buffer_size,
			       volatile uint32_t *tx_byte_offset)
{
	uint8_t bytes_sent = 0;
	uint8_t *tx_data_pointer = (uint8_t *)(test_pattern + *tx_byte_offset);

	if (*tx_byte_offset < buffer_size) {
		bytes_sent = uart_fifo_fill(dev, tx_data_pointer, 1);
		*tx_byte_offset += bytes_sent;
	} else {
		*tx_byte_offset = 0;
		uart_irq_tx_disable(dev);
	}
}

void uart_rx_interrupt_service(const struct device *dev, uint8_t *receive_buffer_pointer,
			       size_t buffer_size, volatile uint32_t *rx_byte_offset)
{
	int rx_data_length = 0;

	do {
		rx_data_length =
			uart_fifo_read(dev, receive_buffer_pointer + *rx_byte_offset, buffer_size);
		*rx_byte_offset += rx_data_length;
	} while (rx_data_length);
	if (*rx_byte_offset >= buffer_size) {
		*rx_byte_offset = 0;
		k_sem_give(&uart_interrupt_mode_rx_done_sem);
		uart_irq_rx_disable(dev);
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
