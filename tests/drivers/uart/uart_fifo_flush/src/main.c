/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/uart.h>
#include "nrfx_uarte.h"

#define UART_TIMEOUT_US	 50000
#define SLEEP_TIME_MS	 10
#define RX_BUFFER_A_SIZE 17
#define RX_BUFFER_B_SIZE 9
#define TX_BUFFER_SIZE	 RX_BUFFER_A_SIZE + RX_BUFFER_B_SIZE
#define UART_NODE	 DT_NODELABEL(dut)

static K_SEM_DEFINE(uart_rx_buf_released_sem, 0, 1);

uint8_t tx_buffer[TX_BUFFER_SIZE];
uint8_t rx_buffer_a[RX_BUFFER_A_SIZE];
uint8_t rx_buffer_b[RX_BUFFER_B_SIZE];
static volatile uint8_t rx_buffer_release_counter;

const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);
NRF_UARTE_Type *uart_reg = (NRF_UARTE_Type *)DT_REG_ADDR(UART_NODE);

static void async_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	uint32_t rx_amount;

	switch (evt->type) {
	case UART_TX_DONE:
		break;
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
		break;
	case UART_RX_BUF_RELEASED:
		rx_amount = nrf_uarte_rx_amount_get(uart_reg);
		rx_buffer_release_counter++;
		if (rx_buffer_release_counter == 1) {
			zassert_equal(rx_amount, RX_BUFFER_A_SIZE);
		} else if (rx_buffer_release_counter == 2) {
			zassert_equal(rx_amount, RX_BUFFER_B_SIZE);
			k_sem_give(&uart_rx_buf_released_sem);
		}
		break;
	case UART_RX_BUF_REQUEST:
		zassert_ok(uart_rx_buf_rsp(uart_dev, rx_buffer_b, RX_BUFFER_B_SIZE),
			   "uart_rx_buf_rsp failed\n");
		break;
	case UART_RX_DISABLED:
		break;
	default:
		break;
	}
}

static void enable_uart_rx(uint8_t *rx_buffer)
{
	int err;

	do {
		err = uart_rx_enable(uart_dev, rx_buffer, RX_BUFFER_A_SIZE, UART_TIMEOUT_US);
	} while (err == -EBUSY);
	zassert_ok(err, "UART RX disable failed, err = %d\n", err);
}

static void set_test_pattern(uint8_t *tx_buffer)
{
	for (int i = 0; i < TX_BUFFER_SIZE; i++) {
		tx_buffer[i] = i;
	}
}

ZTEST(uart_fifo_flush, test_rx_amount_handling)
{
	uint32_t rx_amount;

	set_test_pattern(tx_buffer);
	memset(rx_buffer_a, 0xFF, RX_BUFFER_A_SIZE);
	memset(rx_buffer_b, 0xFF, RX_BUFFER_B_SIZE);

	zassert_ok(uart_callback_set(uart_dev, async_uart_callback, NULL),
		   "uart_callback_set failed\n");
	enable_uart_rx(rx_buffer_a);
	zassert_ok(uart_tx(uart_dev, tx_buffer, TX_BUFFER_SIZE, UART_TIMEOUT_US),
		   "uart_tx failed\n");

	while (k_sem_take(&uart_rx_buf_released_sem, K_NO_WAIT) != 0) {
		k_msleep(SLEEP_TIME_MS);
	};

	for (int i = 0; i < TX_BUFFER_SIZE; i++) {
		if (i < RX_BUFFER_A_SIZE) {
			if (rx_buffer_a[i] != tx_buffer[i]) {
				TC_PRINT("rx_buffer_a[%d] != tx_buffer[%d], 0x%x != 0x%x\n", i, i,
					 rx_buffer_a[i], tx_buffer[i]);
				zassert_true(false, "RX buffer A does not match pattern\n");
			}
		} else {
			if (rx_buffer_b[i - RX_BUFFER_A_SIZE] != tx_buffer[i]) {
				TC_PRINT("rx_buffer_b[%d] != tx_buffer[%d], 0x%x != 0x%x\n",
					 i - RX_BUFFER_A_SIZE, i, rx_buffer_b[i - RX_BUFFER_A_SIZE],
					 tx_buffer[i]);
				zassert_true(false, "RX buffer B does not match pattern\n");
			}
		}
	}
	uart_rx_disable(uart_dev);
	TC_PRINT("Transmission done\n");
	/* With STM logging some dealy is required before reading the register */
	k_msleep(SLEEP_TIME_MS);
	rx_amount = nrf_uarte_rx_amount_get(uart_reg);
	TC_PRINT("RX amount: %u\n", rx_amount);
	zassert_equal(rx_amount, 0,
		      "RX AMOUNT should be 0 after RX disable\n");
}

void *test_setup(void)
{
	zassert_true(device_is_ready(uart_dev), "UART device is not ready");
	TC_PRINT("Platform: %s\n", CONFIG_BOARD_TARGET);

	return NULL;
}

ZTEST_SUITE(uart_fifo_flush, NULL, test_setup, NULL, NULL, NULL);
