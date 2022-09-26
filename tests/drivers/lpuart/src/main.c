/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/busy_sim.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/random/rand32.h>

static uint32_t tx_req_cnt;
static uint32_t tx_done_cnt;
static uint32_t tx_abort_cnt;
static uint32_t rx_cnt;
static volatile bool test_kill;
static volatile bool test_err;
static uint8_t rx_buf[5];
static bool buf_released;
static int test_time;

static void kill_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	TC_PRINT("TX_DONE: %d, RX_CNT:%d\n", tx_done_cnt, rx_cnt);
	test_time++;
	if (test_time == CONFIG_TEST_LPUART_TIMEOUT) {
		k_timer_stop(timer);
		test_kill = true;
	}
}

static void report_error(void);

static void wdt_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	TC_PRINT("Watchdog timer.\n");
	report_error();
}

static K_TIMER_DEFINE(kill_timer, kill_timer_handler, NULL);
static K_TIMER_DEFINE(wdt_timer, wdt_timer_handler, NULL);

static void tx_next_timeout(struct k_timer *timer);
static K_TIMER_DEFINE(tx_timer, tx_next_timeout, NULL);

static K_SEM_DEFINE(test_sem, 0, 1);

static void report_error(void)
{
	test_err = true;
	test_kill = true;
	k_timer_stop(&wdt_timer);
	k_sem_give(&test_sem);
}

static bool is_kill_packet(const uint8_t *buf, size_t len)
{
	static const uint8_t kill_packet[] = {0xFF, 0xFF, 0xFF, 0xFF};

	return (len == sizeof(kill_packet)) && (memcmp(buf, kill_packet, len) == 0);
}

static void next_tx(const struct device *dev)
{
	static uint8_t tx_buf[4];
	static size_t tx_len;

	if (test_kill) {
		memset(tx_buf, 0xFF, sizeof(tx_buf));
		tx_len = 4;
	} else {
		tx_len++;
		tx_len = (tx_len > 4) ? 1 : tx_len;
		tx_buf[0]++;
	}

	tx_req_cnt++;
	int err = uart_tx(dev, tx_buf, tx_len, 10000);

	if (err != 0) {
		TC_PRINT("uart_tx returned err:%d\n", err);
		report_error();
	}
}

static void tx_next_timeout(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);

	next_tx(dev);
}

static void on_tx_done(const struct device *dev, struct uart_event *evt)
{
	tx_done_cnt++;

	if (is_kill_packet(evt->data.tx.buf, evt->data.tx.len)) {
		k_sem_give(&test_sem);
		return;
	}

	uint32_t delay_us = sys_rand32_get() % 1000;

	if (delay_us < 50) {
		next_tx(dev);
	} else {
		k_timer_start(&tx_timer, K_USEC(delay_us), K_NO_WAIT);
	}
}

static void on_rx_rdy(const struct device *dev, struct uart_event *evt)
{
	uint8_t id = evt->data.rx.buf[0];
	uint8_t exp_id;

	k_timer_stop(&wdt_timer);

	if (is_kill_packet(evt->data.rx.buf, evt->data.rx.len)) {
		return;
	}

	rx_cnt++;
	exp_id = (uint8_t)rx_cnt;

	if (id != exp_id) {
		TC_PRINT("Got: %d (0x%02x), exp: %d (0x%02x)\n", id, id, exp_id, exp_id);
		report_error();
	} else {
		k_timer_start(&wdt_timer, K_MSEC(500), K_NO_WAIT);
	}
}

static void on_rx_buf_req(const struct device *dev)
{

	int err;

	if (!buf_released) {
		TC_PRINT("Requested buffer before releasing previous one\n");
		report_error();
	}

	buf_released = false;
	err = uart_rx_buf_rsp(dev, rx_buf, sizeof(rx_buf));
	if (err != 0) {
		TC_PRINT("uart_rx_buf_rsp returned err: %d\n", err);
		report_error();
	}
}

static void uart_callback(const struct device *dev,
			  struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		on_tx_done(dev, evt);
		break;

	case UART_TX_ABORTED:
		tx_abort_cnt++;
		TC_PRINT("Unexpected TX aborted.\n");
		report_error();
		break;

	case UART_RX_RDY:
		on_rx_rdy(dev, evt);
		break;

	case UART_RX_BUF_REQUEST:
		on_rx_buf_req(dev);
		break;

	case UART_RX_BUF_RELEASED:
		buf_released = true;
		break;

	case UART_RX_DISABLED:
		TC_PRINT("Unexpected RX disabled event.\n");
		report_error();
		break;

	case UART_RX_STOPPED:
		TC_PRINT("Unexpected RX stopped event.\n");
		report_error();
		break;
	}
}

int __weak bt_hci_transport_setup(const struct device *h4)
{
	return 0;
}

static void test_async_api_stress(void)
{
	const struct device *lpuart = DEVICE_DT_GET(DT_NODELABEL(lpuart));
	int err;

	(void)bt_hci_transport_setup(NULL);

	if (IS_ENABLED(CONFIG_TEST_BUSY_SIM)) {
		busy_sim_start(500, 400, 600, 550, NULL);
	}

	zassert_true(device_is_ready(lpuart), NULL);

	uart_callback_set(lpuart, uart_callback, NULL);

	buf_released = false;
	err = uart_rx_enable(lpuart, rx_buf, sizeof(rx_buf), 100);
	zassert_equal(err, 0, NULL);

	if (!IS_ENABLED(CONFIG_BOARD_NRF9160DK_NRF52840)) {
		k_msleep(1000);
	}

	k_timer_start(&kill_timer, K_MSEC(1000), K_MSEC(1000));
	k_timer_start(&wdt_timer, K_MSEC(2000), K_NO_WAIT);
	k_timer_user_data_set(&tx_timer, (void *)lpuart);

	next_tx(lpuart);

	err = k_sem_take(&test_sem, K_MSEC(1000 * CONFIG_TEST_LPUART_TIMEOUT + 100));
	k_timer_stop(&wdt_timer);
	k_timer_stop(&kill_timer);
	zassert_equal(err, 0, NULL);
	zassert_false(test_err, NULL);
	zassert_equal(tx_req_cnt, tx_done_cnt, NULL);
}

void test_main(void)
{
	/* Read first random number. There are some generators which do not support
	 * reading first random number from an interrupt context (initialization
	 * is performed at the first read).
	 */
	(void)sys_rand32_get();

	ztest_test_suite(lpuart_test,
			 ztest_unit_test(test_async_api_stress)
			);
	ztest_run_test_suite(lpuart_test);
}
