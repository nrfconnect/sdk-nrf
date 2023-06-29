/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zboss_api.h>

#include <zb_nrf_async_serial.c>
#include <zb_nrf_serial.c>


#define TEST_UART_BUF_LEN 256


/* The following line declares uart_tx_buf_t type. */
ZB_RING_BUFFER_DECLARE(uart_tx_test_buf, zb_uint8_t, TEST_UART_BUF_LEN);
static uart_tx_test_buf_t uart_tx_test_buf;
static uint8_t uart_rx_test_buf[TEST_UART_BUF_LEN];
static uint32_t uart_rx_buf_cnt;


static size_t fill_with_test_data(uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = (i % 0x100);
	}

	return len;
}

static void uart_char_handler(zb_uint8_t ch)
{
	if (uart_rx_buf_cnt < sizeof(uart_rx_test_buf)) {
		uart_rx_test_buf[uart_rx_buf_cnt++] = ch;
	}
}

/**
 * Configure ZBOSS OSIF serial interface.
 */
static void configure_osif_serial(void)
{
	zb_osif_serial_init();
	zb_osif_set_uart_byte_received_cb(uart_char_handler);
	zb_osif_set_user_io_buffer((zb_byte_array_t *)&uart_tx_test_buf,
				   TEST_UART_BUF_LEN);
}

/**
 * Assert that the UART buffer uart_rx_test_buf was filled with exactly len
 * bytes and are equal to the contents of buf.
 */
static void assert_transmission(uint8_t *buf, size_t len)
{
	zassert_equal(len, uart_rx_buf_cnt,
		"The number of transmitted and received bytes is not equal");
	zassert_equal(
		memcmp(buf, uart_rx_test_buf, uart_rx_buf_cnt),
		0,
		"Transmitted and received bytes differ");
}

static void *setup_test_suite(void)
{
	printk("---------------------------------------------------\r\n");
	printk("PRECONDITION: Short UART1 pins before running tests\r\n");
	printk("---------------------------------------------------\r\n");

	/* Enable zigbee callbacks implemented in zigbee_scheduler mock. */
	zigbee_enable();

	/* Configure UART driver. */
	configure_osif_serial();

	return NULL;
}

/**
 * Verify that:
 *  - The driver is found and initialized by the OSIF implementation.
 *  - Driver uses user's buffer during transmission.
 *  - Reset RX buffer counter and its contents.
 */
static void setup_test_case(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_not_null(uart_dev, "UART device was not initialized");

	/* Ensure that the driver uses user's buffer. */
	zassert_equal_ptr(char_handler, uart_char_handler,
			  "The UART RX callback in not properly set");

	/* Reset rx buffer. */
	uart_rx_buf_cnt = 0;
	memset(uart_rx_test_buf, 0, sizeof(uart_rx_test_buf));
}


ZTEST_SUITE(nrf_osif_serial_tests, NULL, setup_test_suite, setup_test_case, NULL, NULL);

/**
 * Verify simple transmission and reception.
 */
ZTEST(nrf_osif_serial_tests, test_tx_rx)
{
	zb_uint8_t test_serial_data[TEST_UART_BUF_LEN] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	zb_osif_serial_put_bytes(test_serial_data, bytes_filled);
	k_sleep(K_MSEC(CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT + 100));
	assert_transmission(test_serial_data, bytes_filled);
}

/**
 * Verify UART sleep API.
 */
ZTEST(nrf_osif_serial_tests, test_sleep)
{
	zassert_not_null(uart_dev, "UART device was not initialized");

	/*
	 * Since power optimization is not yet implemented, simply verify
	 * that the device can withstand sleep state and continue reception.
	 */
	zb_osif_uart_sleep();
	zb_osif_uart_wake_up();

	zassert_not_null(uart_dev,
		"UART device is not initialized after sleep period");
}

/**
 * Verify reception of odd-size transmissions.
 */
ZTEST(nrf_osif_serial_tests, test_undiv_tx_rx)
{
	zb_uint8_t test_serial_data[TEST_UART_BUF_LEN] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	for (int i = 0; i < 3; i++) {
		size_t bytes_sent = (bytes_filled / 3) * (i + 1);

		zb_osif_serial_put_bytes(
			&test_serial_data[bytes_filled * i / 3],
			bytes_filled / 3);
		k_sleep(K_MSEC(CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT + 100));
		zassert_equal(bytes_sent, uart_rx_buf_cnt,
			"The number of transmitted and received bytes differ");
	}

	assert_transmission(test_serial_data, (bytes_filled / 3) * 3);
}

/**
 * Verify reception of several, continuous transmissions.
 */
ZTEST(nrf_osif_serial_tests, test_multi_tx_rx)
{
	zb_uint8_t test_serial_data[TEST_UART_BUF_LEN] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	for (int i = 0; i < 4; i++) {
		size_t bytes_sent = bytes_filled * i / 4;

		zb_osif_serial_put_bytes(&test_serial_data[bytes_sent],
					 bytes_filled / 4);
	}

	k_sleep(K_MSEC(CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT + 100));
	assert_transmission(test_serial_data, bytes_filled);
}
