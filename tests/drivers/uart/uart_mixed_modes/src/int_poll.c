/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

const struct device *const uart_dev_int_mode = DEVICE_DT_GET(UART_NODE_MODE_A);
const struct device *const uart_dev_poll_mode = DEVICE_DT_GET(UART_NODE_MODE_B);

extern struct k_sem uart_interrupt_mode_rx_done_sem;

extern uint8_t uart_a_tx_test_buffer[BUFFER_SIZE];
extern uint8_t uart_a_rx_test_buffer[BUFFER_SIZE];
extern uint8_t uart_b_tx_test_buffer[BUFFER_SIZE];
extern uint8_t uart_b_rx_test_buffer[BUFFER_SIZE];

extern volatile uint32_t tx_byte_offset;
extern volatile uint32_t rx_byte_offset;

ZTEST(uart_int_poll, test_uart_int_poll_transmission)
{
	int err;
	uint32_t sleep_time_ms = 0;
	uart_test_data uart_int_test_data;
	uart_test_data uart_poll_test_data;

	tx_byte_offset = 0;
	rx_byte_offset = 0;

	uart_int_test_data.tx_buffer = uart_a_tx_test_buffer;
	uart_int_test_data.rx_buffer = uart_a_rx_test_buffer;
	uart_int_test_data.buffer_size = BUFFER_SIZE;

	uart_poll_test_data.tx_buffer = uart_b_tx_test_buffer;
	uart_poll_test_data.rx_buffer = uart_b_rx_test_buffer;
	uart_poll_test_data.buffer_size = BUFFER_SIZE;

	set_test_pattern(&uart_int_test_data);
	set_test_pattern(&uart_poll_test_data);

	TC_PRINT("UART full duplex transmission test: UART (INT) <-> UART (POLL)\n");

	err = uart_irq_callback_set(uart_dev_int_mode, uart_isr_handler);
	zassert_equal(err, 0, "Unexpected error when setting callback for INT UART: %d", err);
	err = uart_irq_callback_user_data_set(uart_dev_int_mode, uart_isr_handler,
					      (void *)&uart_int_test_data);
	zassert_equal(err, 0,
		      "Unexpected error when setting user data for callback for INT UART: %d", err);

	TC_PRINT("Starting transmission\n");
	uart_irq_rx_enable(uart_dev_int_mode);
	uart_irq_tx_enable(uart_dev_int_mode);

	for (uint32_t counter = 0; counter < BUFFER_SIZE; counter++) {
		uart_poll_out(uart_dev_poll_mode, uart_poll_test_data.tx_buffer[counter]);
		while (uart_poll_in(uart_dev_poll_mode, &uart_poll_test_data.rx_buffer[counter]) ==
		       -1) {
		};
		uart_poll_in(uart_dev_poll_mode, &uart_poll_test_data.rx_buffer[counter]);
	}

	TC_PRINT("Waiting for transmission to finish\n");
	while (1) {
		if (k_sem_take(&uart_interrupt_mode_rx_done_sem, K_NO_WAIT) == 0) {
			break;
		}
		k_msleep(SEM_CHECK_DEAD_TIME_MS);
		sleep_time_ms += SEM_CHECK_DEAD_TIME_MS;
		zassert_true(sleep_time_ms < TRANSMISSION_TIMEOUT_MS, "Transmission timed out\n");
	}

	TC_PRINT("Full duplex transmission completed\n");
	uart_irq_rx_disable(uart_dev_int_mode);
	uart_irq_tx_disable(uart_dev_int_mode);
	zassert_mem_equal(uart_int_test_data.rx_buffer, uart_poll_test_data.rx_buffer, BUFFER_SIZE,
			  "UART (INT) RX buffer != UART (POLL) RX buffer\n");
}

void *test_setup(void)
{
	zassert_true(device_is_ready(uart_dev_int_mode), "UART in INTERRUPT mode is not ready");
	zassert_true(device_is_ready(uart_dev_poll_mode), "UART in POLLING mode is not ready");

	return NULL;
}

ZTEST_SUITE(uart_int_poll, NULL, test_setup, NULL, NULL, NULL);
