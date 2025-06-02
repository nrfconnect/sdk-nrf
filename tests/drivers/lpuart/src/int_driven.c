/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/uart.h>
#include "common.h"

#define TEST_BUFFER_LEN 30

static K_SEM_DEFINE(uart_transmission_done_sem, 0, 1);

const struct device *lpuart = DEVICE_DT_GET(DT_NODELABEL(lpuart));

const uint8_t test_pattern[TEST_BUFFER_LEN] = {
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
	0x26, 0x27, 0x28, 0x29, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40};
static uint8_t test_buffer[TEST_BUFFER_LEN];
static int tx_byte_offset;
static int rx_byte_offset;

static void uart_tx_interrupt_service(const struct device *dev, int *tx_byte_offset)
{
	uint8_t bytes_sent = 0;
	uint8_t *tx_data_pointer = (uint8_t *)(test_pattern + *tx_byte_offset);

	if (*tx_byte_offset < TEST_BUFFER_LEN) {
		bytes_sent = uart_fifo_fill(dev, tx_data_pointer, 1);
		*tx_byte_offset += bytes_sent;
	} else {
		*tx_byte_offset = 0;
		uart_irq_tx_disable(dev);
		k_sem_give(&uart_transmission_done_sem);
	}
}

static void uart_rx_interrupt_service(const struct device *dev, uint8_t *receive_buffer_pointer,
				      int *rx_byte_offset)
{
	int rx_data_length = 0;

	do {
		rx_data_length = uart_fifo_read(dev, receive_buffer_pointer + *rx_byte_offset,
						TEST_BUFFER_LEN);
		*rx_byte_offset += rx_data_length;
	} while (rx_data_length);
}

static void lpuart_isr_handler(const struct device *dev, void *user_data)
{
	uart_irq_update(dev);
	while (uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uart_rx_interrupt_service(dev, (uint8_t *)user_data, &rx_byte_offset);
		}
		if (uart_irq_tx_ready(dev)) {
			uart_tx_interrupt_service(dev, &tx_byte_offset);
		}
	}
}

static void test_transmission(bool is_tx_pin_float_enabled)
{
	int err;
	NRF_UARTE_Type *uarte = (NRF_UARTE_Type *)DT_REG_ADDR(DT_INST_BUS(0));
	int32_t tx_pin = nrf_uarte_tx_pin_get(uarte);

	err = uart_irq_callback_set(lpuart, lpuart_isr_handler);
	zassert_equal(err, 0, "Unexpected error when setting callback for LPUART %d", err);

	err = uart_irq_callback_user_data_set(lpuart, lpuart_isr_handler, (void *)test_buffer);
	zassert_equal(err, 0, "Unexpected error when setting user data for callback %d", err);
	uart_irq_err_enable(lpuart);
	uart_irq_rx_enable(lpuart);
	uart_irq_tx_enable(lpuart);

	if (is_tx_pin_float_enabled) {
		TC_PRINT("Starting floating pins\n");
		floating_pins_start(tx_pin);
		k_msleep(90);
		TC_PRINT("Stopping floating pins\n");
		floating_pins_stop(tx_pin);
		k_msleep(2);
	} else {
		// k_msleep(100);
		TC_PRINT("Waiting for transmission to finish\n");
		zassert_equal(k_sem_take(&uart_transmission_done_sem, K_MSEC(100)), 0);
		TC_PRINT("Transmission done\n");
	}

	uart_irq_tx_disable(lpuart);
	uart_irq_rx_disable(lpuart);
	uart_irq_err_disable(lpuart);

	for (int index = 0; index < TEST_BUFFER_LEN; index++) {
		zassert_equal(test_buffer[index], test_pattern[index],
			      "Received data byte %d does not match pattern 0x%x != 0x%x", index,
			      test_buffer[index], test_pattern[index]);
	}
}

ZTEST(test_lpuart_int_driven, test_transmission_no_tx_pin_float)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_TEST_LPUART_LOOPBACK);

	tx_byte_offset = 0;
	rx_byte_offset = 0;

	test_transmission(false);
}

ZTEST(test_lpuart_int_driven, test_transmission_with_tx_pin_float)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_TEST_LPUART_LOOPBACK);

	tx_byte_offset = 0;
	rx_byte_offset = 0;

	test_transmission(true);
}

static void *suite_setup(void)
{
	zassert_true(device_is_ready(lpuart), "LPUART device is not ready");

	return NULL;
}

ZTEST_SUITE(test_lpuart_int_driven, NULL, suite_setup, NULL, NULL, NULL);
