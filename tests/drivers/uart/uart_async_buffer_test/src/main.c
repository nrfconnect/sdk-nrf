/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test);

#define BUF_SIZE 4
static K_MEM_SLAB_DEFINE(uart_rx_slab, BUF_SIZE, 8, 4);
K_SEM_DEFINE(rx_rdy, 0, 1);
K_SEM_DEFINE(tx_done, 0, 1);

static const struct device *const uart_dev_tx = DEVICE_DT_GET(DT_NODELABEL(dut));
static const struct device *const uart_dev_rx = DEVICE_DT_GET(DT_NODELABEL(dut_aux));

#define DATA_SIZE 5
static const uint8_t tx_buf[DATA_SIZE] = {1, 2, 3, 4, 5};
volatile static uint8_t rx_buf[DATA_SIZE];
volatile static uint8_t rx_len;

#define DATA_SIZE_SMALL 2
static const uint8_t tx_buf_small[DATA_SIZE_SMALL] = {6,7};

static void tdata_clear()
{
	k_sem_reset(&tx_done);
	k_sem_reset(&rx_rdy);
	rx_len = 0;
	memset(rx_buf, 0, DATA_SIZE);
}

static void tdata_check_recv_buffers(const uint8_t *tx_buf, uint32_t sent_bytes)
{
	zassert_equal(rx_len, sent_bytes, "Invalid data received %d vs %d", rx_len, sent_bytes);
	zassert_equal(memcmp(tx_buf, rx_buf, rx_len), 0, "Invalid data received in first buffer");
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	int ret;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;

	case UART_TX_ABORTED:
		break;

	case UART_RX_RDY:
		zassert_true(evt->data.rx.len < DATA_SIZE);
		memcpy(&rx_buf[rx_len], &evt->data.rx.buf[evt->data.rx.offset], evt->data.rx.len);
		rx_len += evt->data.rx.len;
		k_sem_give(&rx_rdy);
		break;

	case UART_RX_BUF_REQUEST: {
		uint8_t *buf;

		ret = k_mem_slab_alloc(&uart_rx_slab, (void **)&buf, K_NO_WAIT);
		zassert_true(ret == 0, "Failed to allocate slab: %d\n", ret);

		ret = uart_rx_buf_rsp(dev, buf, BUF_SIZE);
		zassert_true(ret == 0, "Failed to provide new buffer: %d\n", ret);
		break;
	}

	case UART_RX_BUF_RELEASED:
		k_mem_slab_free(&uart_rx_slab, (void *)evt->data.rx_buf.buf);
		break;

	case UART_RX_DISABLED:
		break;

	case UART_RX_STOPPED:
		break;
	}
}

ZTEST(uart_async_buffer_test, dummy)
{
	int ret;
	uint8_t *buf;

	tdata_clear();

	ret = k_mem_slab_alloc(&uart_rx_slab, (void **)&buf, K_NO_WAIT);
	zassert_true(ret == 0, "Failed to alloc slab: %d\n", ret);

	ret = uart_callback_set(uart_dev_rx, uart_callback, NULL);
	zassert_true(ret == 0, "Failed to set callback: %d\n", ret);

	ret = uart_callback_set(uart_dev_tx, uart_callback, NULL);
	zassert_true(ret == 0, "Failed to set callback: %d\n", ret);

	ret = uart_rx_enable(uart_dev_rx, buf, BUF_SIZE, 1000);
	zassert_true(ret == 0, "Failed to enable RX: %d\n", ret);

	for (uint8_t i=0; i<10;++i) {
		tdata_clear();
		// LOG_INF("uart_tx: %d\n", i);
		ret = uart_tx(uart_dev_tx, tx_buf_small, sizeof(tx_buf_small), 10000);
		zassert_true(ret == 0, "Failed to initiate transmission: %d\n", ret);
		// LOG_INF("uart_tx end: %d\n", i);

		zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
		zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
		zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN, "Extra RX_RDY received");

		tdata_check_recv_buffers(tx_buf_small, sizeof(tx_buf_small));
	}

	tdata_clear();
	ret = uart_tx(uart_dev_tx, tx_buf, sizeof(tx_buf), 10000);
	zassert_true(ret == 0, "Failed to initiate transmission: %d\n", ret);

	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN, "Extra RX_RDY received");

	tdata_check_recv_buffers(tx_buf, sizeof(tx_buf));

}

static void *setup(void)
{
	/*
	 * Wait for any initialization to finish
	 */
	k_msleep(1000);

	zassert_true(device_is_ready(uart_dev_tx), "UART device is not ready");
	zassert_true(device_is_ready(uart_dev_rx), "UART device is not ready");

	return NULL;
}

ZTEST_SUITE(uart_async_buffer_test, NULL, setup, NULL, NULL, NULL);
