/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <zboss_api.h>

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
static void configure_test_suite(void)
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


/**
 * Verify that:
 *  - The driver is found and initialized by the OSIF implementation.
 *  - There is no ongoing TX operation.
 *  - Reset RX buffer counter and its contents.
 */
static void setup_test_case(void)
{
	zassert_not_null(uart_dev, "UART device was not initialized");

	/* Ensure that there is no ongoing transmission. */
	zassert_equal(0, k_sem_take(&tx_lock, TEST_TX_LOCK_TIMEOUT),
		"There is a pending transmission corrupting the test case");

	/* Reset rx buffer. */
	uart_rx_buf_cnt = 0;
	memset(uart_rx_test_buf, 0, sizeof(uart_rx_test_buf));
}


/**
 * Verify simple transmission and reception.
 */
static void test_tx_rx(void)
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
static void test_sleep(void)
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
static void test_undiv_tx_rx(void)
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
static void test_multi_tx_rx(void)
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
static void test_small_rx_buffer(void)
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
static void test_single_rx_chunk(void)
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
static void test_two_rx_chunks(void)
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
static void test_rx_timeout_single_byte(void)
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
static void test_rx_timeout_chunk_and_single_byte(void)
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
static void test_rx_timeout_two_chunks_and_single_byte(void)
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

void test_main(void)
{
	printk("---------------------------------------------------\r\n");
	printk("PRECONDITION: Short UART1 pins before running tests\r\n");
	printk("---------------------------------------------------\r\n");

	/* Enable zigbee callbacks implemented in zigbee_scheduler mock. */
	zigbee_enable();

	/* Configure UART driver. */
	configure_test_suite();

	ztest_test_suite(nrf_osif_serial_tests,
		ztest_unit_test_setup_teardown(
			test_tx_rx,
			setup_test_case,
			unit_test_noop),
		ztest_unit_test(test_sleep),
		ztest_unit_test_setup_teardown(
			test_undiv_tx_rx,
			setup_test_case,
			unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_multi_tx_rx,
			setup_test_case,
			unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_small_rx_buffer,
			setup_test_case,
			unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_single_rx_chunk,
			setup_test_case,
			unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_two_rx_chunks,
			setup_test_case,
			unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_rx_timeout_single_byte,
			setup_test_case,
			unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_rx_timeout_chunk_and_single_byte,
			setup_test_case,
			unit_test_noop),
		ztest_unit_test_setup_teardown(
			test_rx_timeout_two_chunks_and_single_byte,
			setup_test_case,
			unit_test_noop)
		);

	ztest_run_test_suite(nrf_osif_serial_tests);
}
