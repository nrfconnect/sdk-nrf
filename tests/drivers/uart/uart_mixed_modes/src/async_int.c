/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

const struct device *const uart_dev_async_mode = DEVICE_DT_GET(UART_NODE_MODE_A);
const struct device *const uart_dev_int_mode = DEVICE_DT_GET(UART_NODE_MODE_B);

extern struct k_sem uart_async_mode_rx_ready_sem;
extern struct k_sem uart_interrupt_mode_rx_done_sem;

extern uint8_t uart_a_tx_test_buffer[BUFFER_SIZE];
extern uint8_t uart_a_rx_test_buffer[BUFFER_SIZE];
extern uint8_t uart_b_tx_test_buffer[BUFFER_SIZE];
extern uint8_t uart_b_rx_test_buffer[BUFFER_SIZE];

extern volatile uint32_t tx_byte_offset;
extern volatile uint32_t rx_byte_offset;

ZTEST(uart_async_int, test_uart_async_int_transmission)
{
	int err, rx_done_async_sem_take_result, rx_done_int_sem_take_result;
	uint32_t sleep_time_ms = 0;
	uart_test_data uart_async_test_data;
	uart_test_data uart_int_test_data;

	tx_byte_offset = 0;
	rx_byte_offset = 0;

	uart_async_test_data.tx_buffer = uart_a_tx_test_buffer;
	uart_async_test_data.rx_buffer = uart_a_rx_test_buffer;
	uart_async_test_data.buffer_size = BUFFER_SIZE;

	uart_int_test_data.tx_buffer = uart_b_tx_test_buffer;
	uart_int_test_data.rx_buffer = uart_b_rx_test_buffer;
	uart_int_test_data.buffer_size = BUFFER_SIZE;

	set_test_pattern(&uart_async_test_data);
	set_test_pattern(&uart_int_test_data);

	TC_PRINT("UART full duplex transmission test: UART (ASYNC) <-> UART (INT)\n");

	err = uart_callback_set(uart_dev_async_mode, async_uart_callback, NULL);
	zassert_equal(err, 0, "ASYNC UART callback setup failed");

	err = uart_irq_callback_set(uart_dev_int_mode, uart_isr_handler);
	zassert_equal(err, 0, "Unexpected error when setting callback for INT UART: %d", err);
	err = uart_irq_callback_user_data_set(uart_dev_int_mode, uart_isr_handler,
					      (void *)&uart_int_test_data);
	zassert_equal(err, 0,
		      "Unexpected error when setting user data for callback for INT UART: %d", err);

	TC_PRINT("Starting transmission\n");
	enable_uart_rx(uart_dev_async_mode, &uart_async_test_data);
	uart_irq_rx_enable(uart_dev_int_mode);

	uart_irq_tx_enable(uart_dev_int_mode);
	err = uart_tx(uart_dev_async_mode, uart_async_test_data.tx_buffer, BUFFER_SIZE,
		      UART_TIMEOUT_US);

	TC_PRINT("Waiting for transmission to finish\n");
	while (1) {
		rx_done_async_sem_take_result =
			k_sem_take(&uart_async_mode_rx_ready_sem, K_NO_WAIT);
		rx_done_int_sem_take_result =
			k_sem_take(&uart_interrupt_mode_rx_done_sem, K_NO_WAIT);
		if ((rx_done_async_sem_take_result + rx_done_int_sem_take_result) == 0) {
			TC_PRINT("Full duplex transmission completed\n");
			break;
		}
		k_msleep(SEM_CHECK_DEAD_TIME_MS);
		sleep_time_ms += SEM_CHECK_DEAD_TIME_MS;
		zassert_true(sleep_time_ms < TRANSMISSION_TIMEOUT_MS, "Transmission timed out\n");
	}
	uart_rx_disable(uart_dev_async_mode);
	uart_irq_rx_disable(uart_dev_int_mode);
	uart_irq_tx_disable(uart_dev_int_mode);
	zassert_mem_equal(uart_async_test_data.rx_buffer, uart_int_test_data.rx_buffer, BUFFER_SIZE,
			  "UART (ASYNC) RX buffer != UART (INT) RX buffer\n");
}

void *test_setup(void)
{
	zassert_true(device_is_ready(uart_dev_async_mode), "UART in ASYNC mode is not ready");
	zassert_true(device_is_ready(uart_dev_int_mode), "UART in INTERRUPT mode is not ready");

	return NULL;
}

ZTEST_SUITE(uart_async_int, NULL, test_setup, NULL, NULL, NULL);
