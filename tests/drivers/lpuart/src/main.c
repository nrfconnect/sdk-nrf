/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/busy_sim.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/random/random.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_uarte.h>
#include <nrfx_gpiote.h>

#define DT_DRV_COMPAT nordic_nrf_sw_lpuart

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
		TC_PRINT("Killing test\n");
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
	int err = uart_tx(dev, tx_buf, tx_len, 100000);

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
		TC_PRINT("Got kill packet\n");
		test_kill = true;
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

ZTEST(test_lpuart_stress, test_stress)
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

	k_msleep(100);

	err = uart_rx_disable(lpuart);
	zassert_equal(err, 0, NULL);

	if (IS_ENABLED(CONFIG_TEST_BUSY_SIM)) {
		busy_sim_stop();
	}
}

static uint8_t tx_buf[] = {0xA, 0xAA, 0xB, 0xBB};

static void resilency_uart_callback(const struct device *dev,
				    struct uart_event *evt,
				    void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		if ((evt->data.tx.buf != tx_buf) || (evt->data.tx.len != sizeof(tx_buf))) {
			report_error();
		}
		break;

	case UART_TX_ABORTED:
		TC_PRINT("Unexpected TX aborted.\n");
		report_error();
		break;

	case UART_RX_RDY:
		if (evt->data.rx.len != sizeof(tx_buf)) {
			report_error();
		}
		if (memcmp(evt->data.rx.buf, tx_buf, sizeof(tx_buf)) != 0) {
			report_error();
		}
		break;

	case UART_RX_BUF_REQUEST:
		break;

	case UART_RX_BUF_RELEASED:
		break;

	case UART_RX_DISABLED:
		break;

	case UART_RX_STOPPED:
		TC_PRINT("Unexpected RX stopped event.\n");
		report_error();
		break;
	}
}

static void validate_lpuart(const struct device *lpuart)
{
	int err;

	k_timer_start(&wdt_timer, K_MSEC(200), K_NO_WAIT);

	err = uart_tx(lpuart, tx_buf, sizeof(tx_buf), 10000);
	zassert_equal(err, 0, NULL);

	k_msleep(10);

	k_timer_stop(&wdt_timer);
	zassert_false(test_err, NULL);
}

static const struct device *counter =
	DEVICE_DT_GET(DT_PHANDLE(DT_COMPAT_GET_ANY_STATUS_OKAY(vnd_busy_sim), counter));

static void next_alarm(const struct device *counter, struct counter_alarm_cfg *alarm_cfg)
{
	int err;
	int mul = IS_ENABLED(CONFIG_NO_OPTIMIZATIONS) ? 10 : 1;

	alarm_cfg->ticks = (50 + sys_rand32_get() % 300) * mul;

	err = counter_set_channel_alarm(counter, 0, alarm_cfg);
	__ASSERT_NO_MSG(err == 0);
}

static nrf_gpio_pin_pull_t pin_toggle(uint32_t pin, nrf_gpio_pin_pull_t pull)
{
	nrf_gpio_cfg(pin,
		     NRF_GPIO_PIN_DIR_INPUT,
		     NRF_GPIO_PIN_INPUT_DISCONNECT,
		     pull,
		     NRF_GPIO_PIN_S0S1,
		     NRF_GPIO_PIN_NOSENSE);
	return (pull == NRF_GPIO_PIN_PULLUP) ? NRF_GPIO_PIN_PULLDOWN : NRF_GPIO_PIN_PULLUP;
}

static void pins_toggle(int32_t tx_pin)
{
	static nrf_gpio_pin_pull_t pull = NRF_GPIO_PIN_PULLUP;
	static nrf_gpio_pin_pull_t tx_pull = NRF_GPIO_PIN_PULLUP;
	nrfx_gpiote_pin_t req_pin = DT_INST_PROP(0, req_pin);
	bool req_pin_toggle;
	bool tx_pin_toggle;

	if (tx_pin > 0) {
		uint32_t rnd = sys_rand32_get();

		req_pin_toggle = (rnd & 0x6) == 0;
		tx_pin_toggle = rnd & 0x1;
	} else {
		req_pin_toggle = true;
		tx_pin_toggle = false;
	}

	if (req_pin_toggle) {
		pull = pin_toggle(req_pin, pull);
	}

	if (tx_pin_toggle) {
		tx_pull = pin_toggle(tx_pin, tx_pull);
	}
}

static void pins_to_default(int32_t tx_pin)
{
	nrfx_gpiote_pin_t req_pin = DT_INST_PROP(0, req_pin);

	nrf_gpio_cfg(req_pin,
		     NRF_GPIO_PIN_DIR_OUTPUT,
		     NRF_GPIO_PIN_INPUT_DISCONNECT,
		     NRF_GPIO_PIN_NOPULL,
		     NRF_GPIO_PIN_S0S1,
		     NRF_GPIO_PIN_NOSENSE);

	if (tx_pin > 0) {
		nrf_gpio_cfg(tx_pin,
			     NRF_GPIO_PIN_DIR_OUTPUT,
			     NRF_GPIO_PIN_INPUT_DISCONNECT,
			     NRF_GPIO_PIN_NOPULL,
			     NRF_GPIO_PIN_S0S1,
			     NRF_GPIO_PIN_NOSENSE);
	}
}

struct test_data {
	struct counter_alarm_cfg alarm_cfg;
	int32_t tx_pin;
};

static void counter_alarm_callback(const struct device *dev,
				   uint8_t chan_id, uint32_t ticks,
				   void *user_data)
{
	struct test_data *data = (struct test_data *)user_data;

	pins_toggle(data->tx_pin);
	next_alarm(dev, &data->alarm_cfg);
}

static void floating_pins_start(int32_t tx_pin)
{
	struct test_data data;

	data.alarm_cfg.callback = counter_alarm_callback;
	data.alarm_cfg.flags = COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	data.alarm_cfg.user_data = (void *)&data;
	data.tx_pin = tx_pin;

	counter_start(counter);
	next_alarm(counter, &data.alarm_cfg);
}

static void floating_pins_stop(int32_t tx_pin)
{
	counter_cancel_channel_alarm(counter, 0);
	counter_stop(counter);
	pins_to_default(tx_pin);
}

static void test_resilency(bool tx_pin_float)
{
	const struct device *lpuart = DEVICE_DT_GET(DT_NODELABEL(lpuart));
	uint32_t t = k_uptime_get_32() + 3000;
	int err;
	NRF_UARTE_Type *uarte = (NRF_UARTE_Type *)DT_REG_ADDR(DT_INST_BUS(0));
	int32_t tx_pin = nrf_uarte_tx_pin_get(uarte);

	if (!IS_ENABLED(CONFIG_TEST_LPUART_LOOPBACK)) {
		return;
	}

	zassert_true(device_is_ready(lpuart), NULL);

	uart_callback_set(lpuart, resilency_uart_callback, NULL);

	while (k_uptime_get_32() < t) {
		err = uart_rx_enable(lpuart, rx_buf, sizeof(rx_buf), 100);
		zassert_equal(err, 0, "Unexpected err:%d", err);

		floating_pins_start(tx_pin_float ? tx_pin : -1);
		k_msleep(100);
		floating_pins_stop(tx_pin_float ? tx_pin : -1);
		k_msleep(2);

		validate_lpuart(lpuart);

		err = uart_rx_disable(lpuart);
		zassert_equal(err, 0, "Unexpected err:%d", err);
	}
}

ZTEST(test_lpuart_resilency, test_resilency_no_tx_pin_float)
{
	test_resilency(false);
}

ZTEST(test_lpuart_resilency, test_resilency_with_tx_pin_float)
{
	test_resilency(true);
}

static void *suite_setup(void)
{
	/* Read first random number. There are some generators which do not support
	 * reading first random number from an interrupt context (initialization
	 * is performed at the first read).
	 */
	(void)sys_rand32_get();

	return NULL;
}

ZTEST_SUITE(test_lpuart_stress, NULL, suite_setup, NULL, NULL, NULL);
ZTEST_SUITE(test_lpuart_resilency, NULL, suite_setup, NULL, NULL, NULL);
