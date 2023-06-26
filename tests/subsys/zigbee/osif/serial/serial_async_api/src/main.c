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
#define TEST_RX_LOCK_TIMEOUT K_MSEC(CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT + 100)
#define TEST_TX_LOCK_TIMEOUT K_MSEC(CONFIG_ZIGBEE_UART_PARTIAL_TX_TIMEOUT + 100)


static K_SEM_DEFINE(tx_lock, 1, 1);
static K_SEM_DEFINE(rx_lock, 0, 1);

static uint8_t uart_rx_test_buf[TEST_UART_BUF_LEN];
static uint32_t uart_rx_buf_cnt;
static uint8_t uart_rx_mem_test_buf[TEST_UART_BUF_LEN];


static size_t fill_with_test_data(uint8_t *buf, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = (i % 0x100);
	}

	return len;
}

static void uart_recv_cb(zb_uint8_t *buf, zb_ushort_t len)
{
	if (uart_rx_buf_cnt + len <= sizeof(uart_rx_test_buf)) {
		memcpy(&uart_rx_test_buf[uart_rx_buf_cnt], buf, len);
		uart_rx_buf_cnt += len;
	}
	k_sem_give(&rx_lock);
}

static void uart_send_cb(zb_uint8_t status)
{
	k_sem_give(&tx_lock);
}


/**
 * Configure ZBOSS OSIF serial interface.
 */
static void configure_osif_serial(void)
{
	zb_osif_serial_init();
	zb_osif_serial_set_cb_recv_data(uart_recv_cb);
	zb_osif_serial_set_cb_send_data(uart_send_cb);
}

/**
 * Assert that the UART buffer uart_rx_test_buf was filled with exactly len
 * bytes and are equal to the contents of buf.
 */
static void assert_transmission(uint8_t *buf, size_t len)
{
	zassert_equal(0, k_sem_take(&rx_lock, TEST_RX_LOCK_TIMEOUT),
		"Reception was not finished within the RX timeout");
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
 *  - There is no ongoing TX operation.
 *  - Reset RX buffer counter and its contents.
 */
static void setup_test_case(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_not_null(uart_dev, "UART device was not initialized");

	/* Ensure that there is no ongoing transmission. */
	zassert_equal(0, k_sem_take(&tx_lock, TEST_TX_LOCK_TIMEOUT),
		"There is a pending transmission corrupting the test case");

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

	/* Perform transmission through asynchronous API. */
	zb_osif_serial_recv_data(uart_rx_mem_test_buf, bytes_filled);
	zb_osif_serial_send_data(test_serial_data, bytes_filled);
	zassert_equal(0, k_sem_take(&tx_lock, TEST_TX_LOCK_TIMEOUT),
		"Transmission was not finished within the TX timeout");
	k_sem_give(&tx_lock);

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

	k_sem_give(&tx_lock);
}

/**
 * Verify reception of odd-size transmissions.
 */
ZTEST(nrf_osif_serial_tests, test_undiv_tx_rx)
{
	zb_uint8_t test_serial_data[TEST_UART_BUF_LEN] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	zb_osif_serial_recv_data(uart_rx_mem_test_buf, (bytes_filled / 3) * 3);

	for (int i = 0; i < 3; i++) {
		size_t bytes_sent = (bytes_filled / 3) * i;

		/*
		 * In asynchronous mode there should be no data received until
		 * the whole, planned buffer is received.
		 */
		zassert_equal(0, uart_rx_buf_cnt,
			"The number of transmitted and received bytes differ");

		zb_osif_serial_send_data(&test_serial_data[bytes_sent],
					 bytes_filled / 3);
		zassert_equal(0, k_sem_take(&tx_lock, TEST_TX_LOCK_TIMEOUT),
			"Transmission was not finished within the TX timeout");
	}

	k_sem_give(&tx_lock);
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

	zb_osif_serial_recv_data(uart_rx_mem_test_buf, bytes_filled);

	for (int i = 0; i < 4; i++) {
		size_t bytes_sent = (bytes_filled / 4) * i;

		/*
		 * Perform several transmissions through asynchronous API as
		 * fast as possible.
		 */
		zb_osif_serial_send_data(&test_serial_data[bytes_sent],
					 bytes_filled / 4);
		zassert_equal(0, k_sem_take(&tx_lock, TEST_TX_LOCK_TIMEOUT),
			"Transmission was not finished within the TX timeout");
	}

	k_sem_give(&tx_lock);
	assert_transmission(test_serial_data, bytes_filled);
}

/**
 * Verify reception of large buffer through several reception requests.
 */
ZTEST(nrf_osif_serial_tests, test_small_rx_buffer)
{
	zb_uint8_t test_serial_data[TEST_UART_BUF_LEN] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	zb_osif_serial_recv_data(uart_rx_mem_test_buf, bytes_filled / 4);
	zb_osif_serial_send_data(test_serial_data, bytes_filled);

	for (int i = 0; i < 3; i++) {
		/*
		 * Perform several receptions through asynchronous API while
		 * transmitter is continuously sending data.
		 */
		zassert_equal(0, k_sem_take(&rx_lock, TEST_RX_LOCK_TIMEOUT),
			"Intermediate Reception timed out");
		zb_osif_serial_recv_data(uart_rx_mem_test_buf,
					 bytes_filled / 4);
	}

	assert_transmission(test_serial_data, bytes_filled);
}

/**
 * Verify that it is possible to receive transmission of single-chunk length.
 */
ZTEST(nrf_osif_serial_tests, test_single_rx_chunk)
{
	zb_uint8_t test_serial_data[CONFIG_ZIGBEE_UART_RX_BUF_LEN] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	zb_osif_serial_recv_data(uart_rx_mem_test_buf, bytes_filled);
	zb_osif_serial_send_data(test_serial_data, bytes_filled);

	assert_transmission(test_serial_data, bytes_filled);
}

/**
 * Verify that it is possible to receive transmission of double-chunk length.
 */
ZTEST(nrf_osif_serial_tests, test_two_rx_chunks)
{
	zb_uint8_t test_serial_data[CONFIG_ZIGBEE_UART_RX_BUF_LEN * 2] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	zb_osif_serial_recv_data(uart_rx_mem_test_buf, bytes_filled);
	zb_osif_serial_send_data(test_serial_data, bytes_filled);

	assert_transmission(test_serial_data, bytes_filled);
}

/**
 * Verify that it is possible to receive transmission shorter than requested
 * (one byte).
 */
ZTEST(nrf_osif_serial_tests, test_rx_timeout_single_byte)
{
#if CONFIG_ZIGBEE_UART_RX_BUF_LEN < 2
	ztest_test_skip();
#else
	zb_uint8_t test_serial_data[1] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	/* Reset rx buffer. */
	uart_rx_buf_cnt = 0;
	memset(uart_rx_test_buf, 0, sizeof(uart_rx_test_buf));
	zb_osif_serial_recv_data(uart_rx_mem_test_buf,
				 sizeof(uart_rx_mem_test_buf));
	zb_osif_serial_send_data(test_serial_data, bytes_filled);

	assert_transmission(test_serial_data, bytes_filled);
#endif
}

/**
 * Verify that it is possible to receive transmission shorter than requested
 * (one chunk and one byte).
 */
ZTEST(nrf_osif_serial_tests, test_rx_timeout_chunk_and_single_byte)
{
#if CONFIG_ZIGBEE_UART_RX_BUF_LEN < 2
	ztest_test_skip();
#else
	zb_uint8_t test_serial_data[CONFIG_ZIGBEE_UART_RX_BUF_LEN + 1] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	/* Reset rx buffer. */
	uart_rx_buf_cnt = 0;
	memset(uart_rx_test_buf, 0, sizeof(uart_rx_test_buf));
	zb_osif_serial_recv_data(uart_rx_mem_test_buf,
				 sizeof(uart_rx_mem_test_buf));
	zb_osif_serial_send_data(test_serial_data, bytes_filled);

	assert_transmission(test_serial_data, bytes_filled);
#endif
}

/**
 * Verify that it is possible to receive transmission shorter than requested
 * (two chunks and one byte).
 */
ZTEST(nrf_osif_serial_tests, test_rx_timeout_two_chunks_and_single_byte)
{
	zb_uint8_t test_serial_data[CONFIG_ZIGBEE_UART_RX_BUF_LEN*2+1] = { 0 };
	size_t bytes_filled = fill_with_test_data(test_serial_data,
						  sizeof(test_serial_data));

	/* Reset rx buffer. */
	uart_rx_buf_cnt = 0;
	memset(uart_rx_test_buf, 0, sizeof(uart_rx_test_buf));
	zb_osif_serial_recv_data(uart_rx_mem_test_buf,
				 sizeof(uart_rx_mem_test_buf));
	zb_osif_serial_send_data(test_serial_data, bytes_filled);

	assert_transmission(test_serial_data, bytes_filled);
}
