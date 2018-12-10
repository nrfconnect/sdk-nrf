/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

#include <zephyr/types.h>
#include <zephyr.h>
#include <uart.h>

#include <soc.h>
#include <gpio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>

#include <bluetooth/services/nus.h>

#include <stdio.h>

#define STACKSIZE               CONFIG_BT_GATT_NUS_THREAD_STACK_SIZE
#define PRIORITY                7

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	        (sizeof(DEVICE_NAME) - 1)

/* Change this if you have an LED connected to a custom port */
#ifndef LED0_GPIO_CONTROLLER
#define LED0_GPIO_CONTROLLER    LED0_GPIO_PORT
#endif

#define LED_PORT                LED0_GPIO_CONTROLLER

#define RUN_STATUS_LED          LED0_GPIO_PIN
#define RUN_LED_BLINK_INTERVAL  1000

#define CON_STATUS_LED          LED1_GPIO_PIN

#define LED_ON                  0
#define LED_OFF                 1

#define UART_BUF_SIZE           CONFIG_BT_GATT_NUS_UART_BUFFER_SIZE

#ifdef CONFIG_BT_GATT_NUS_SECURITY_ENABLED
#ifdef CONFIG_BT_GATT_NUS_SECURITY_LEVEL_LOW
static const bt_security_t sec_level = BT_SECURITY_LOW;
#elif CONFIG_BT_GATT_NUS_SECURITY_LEVEL_MED
static const bt_security_t sec_level = BT_SECURITY_MEDIUM;
#elif CONFIG_BT_GATT_NUS_SECURITY_LEVEL_HIGH
static const bt_security_t sec_level = BT_SECURITY_HIGH;
#elif CONFIG_BT_GATT_NUS_SECURITY_LEVEL_FIPS
static const bt_security_t sec_level = BT_SECURITY_FIPS;
#endif
#endif

static K_SEM_DEFINE(ble_init_ok, 0, 2);

static struct bt_conn *current_conn;

static struct device  *led_port;
static struct device  *uart;

struct uart_data_t {
	void  *fifo_reserved;
	u8_t    data[UART_BUF_SIZE];
	u16_t   len;
};

static K_FIFO_DEFINE(fifo_uart_tx_data);
static K_FIFO_DEFINE(fifo_uart_rx_data);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, NUS_UUID_SERVICE),
};

static void set_led_state(int led, bool state)
{
	if (led_port) {
		gpio_pin_write(led_port, led, state);
	}
}

static void uart_cb(struct device *uart)
{
	static struct uart_data_t *rx;

	uart_irq_update(uart);

	if (uart_irq_rx_ready(uart)) {
		int data_length;

		if (!rx) {
			rx = k_malloc(sizeof(*rx));
			if (rx) {
				rx->len = 0;
			} else {
				char dummy;

				printk("Not able to allocate UART receive buffer\n");

				/* Drop one byte to avoid spinning in a
				 * eternal loop.
				 */
				uart_fifo_read(uart, &dummy, 1);

				return;
			}
		}

		data_length = uart_fifo_read(uart, &rx->data[rx->len],
					     UART_BUF_SIZE-rx->len);
		rx->len += data_length;

		if (rx->len > 0) {
			/* Send buffer to bluetooth unit if either buffer size
			 * is reached or the char \n or \r is received, which
			 * ever comes first
			 */
			if ((rx->len == UART_BUF_SIZE) ||
			   (rx->data[rx->len - 1] == '\n') ||
			   (rx->data[rx->len - 1] == '\r')) {
				k_fifo_put(&fifo_uart_rx_data, rx);
				rx = NULL;
			}
		}
	}

	if (uart_irq_tx_ready(uart)) {
		struct uart_data_t *buf =
			k_fifo_get(&fifo_uart_tx_data, K_NO_WAIT);
		u16_t written = 0;

		/* Nothing in the FIFO, nothing to send */
		if (!buf) {
			uart_irq_tx_disable(uart);
			return;
		}

		while (buf->len > written) {
			written += uart_fifo_fill(uart,
						  &buf->data[written],
						  buf->len - written);
		}

		while (!uart_irq_tx_complete(uart)) {
			/* Wait for the last byte to get
			 * shifted out of the module
			 */
		}

		if (k_fifo_is_empty(&fifo_uart_tx_data)) {
			uart_irq_tx_disable(uart);
		}

		k_free(buf);
	}
}

static int init_uart(void)
{
	uart = device_get_binding("UART_0");
	if (!uart) {
		return -ENXIO;
	}

	uart_irq_callback_set(uart, uart_cb);
	uart_irq_rx_enable(uart);

	return 0;
}

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");
	current_conn = bt_conn_ref(conn);

#ifdef CONFIG_BT_GATT_NUS_SECURITY_ENABLED
	if (bt_conn_security(conn, sec_level)) {
		printk("Failed to set security level %d\n", sec_level);
	}
#endif
	set_led_state(CON_STATUS_LED, LED_ON);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
		set_led_state(CON_STATUS_LED, LED_OFF);
	}
}

#ifdef CONFIG_BT_GATT_NUS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	printk("Security level was raised to %d\n", level);
}
#endif

static struct bt_conn_cb conn_callbacks = {
	.connected    = connected,
	.disconnected = disconnected,
#ifdef CONFIG_BT_GATT_NUS_SECURITY_ENABLED
	.security_changed = security_changed,
#endif
};

#if !defined(CONFIG_BT_GATT_NUS_SECURITY_LEVEL_LOW) && \
	defined(CONFIG_BT_GATT_NUS_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};
#endif

static void bt_receive_cb(const u8_t *const data, u16_t len)
{
	for (u16_t pos = 0; pos != len;) {
		struct uart_data_t *tx = k_malloc(sizeof(*tx));

		if (!tx) {
			printk("Not able to allocate UART send data buffer\n");
			return;
		}

		/* Keep the last byte of TX buffer for potential LF char. */
		size_t tx_data_size = sizeof(tx->data) - 1;

		if ((len - pos) > tx_data_size) {
			tx->len = tx_data_size;
		} else {
			tx->len = (len - pos);
		}

		memcpy(tx->data, &data[pos], tx->len);

		pos += tx->len;

		/* Append the LF character when the CR character triggered
		 * transmission from the peer.
		 */
		if ((pos == len) && (data[len - 1] == '\r')) {
			tx->data[tx->len] = '\n';
			tx->len++;
		}

		k_fifo_put(&fifo_uart_tx_data, tx);
	}

	/* Start the UART transfer by enabling the TX ready interrupt */
	uart_irq_tx_enable(uart);
}

static struct bt_nus_cb nus_cb = {
	.received_cb = bt_receive_cb,
};

static void bt_ready(int err)
{
	if (err) {
		printk("BLE init failed with error code %d\n", err);
		return;
	}

	err = nus_init(&nus_cb);
	if (err) {
		printk("Failed to initialize UART service (err: %d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
	}

	/* Give two semaphores to signal both the led_blink_thread, and
	 * and the ble_write_thread that ble initialized successfully
	 */
	k_sem_give(&ble_init_ok);
	k_sem_give(&ble_init_ok);
}

static int init_leds(void)
{
	int err = 0;

	led_port = device_get_binding(LED_PORT);

	if (!led_port) {
		printk("Could not bind to LED port\n");
		return -ENXIO;
	}

	err = gpio_pin_configure(led_port, RUN_STATUS_LED,
			   GPIO_DIR_OUT);
	if (!err) {
		err = gpio_pin_configure(led_port, CON_STATUS_LED,
			   GPIO_DIR_OUT);
	}

	if (!err) {
		err = gpio_port_write(led_port, LED_OFF << RUN_STATUS_LED |
						LED_OFF << CON_STATUS_LED);
	}

	if (err) {
		printk("Not able to correctly initialize LED pins (err:%d)",
			err);
		led_port = NULL;
	}

	return err;
}

void error(void)
{
	int err = -1;

	led_port = device_get_binding(LED_PORT);
	if (led_port) {
		err = gpio_port_configure(led_port, GPIO_DIR_OUT);
	}

	if (!err) {
		gpio_port_write(led_port, LED_ON << LED0_GPIO_PIN |
					  LED_ON << LED1_GPIO_PIN |
					  LED_ON << LED2_GPIO_PIN |
					  LED_ON << LED3_GPIO_PIN);
	}

	while (true) {
		/* Spin for ever */
		k_sleep(1000);
	}
}

static void led_blink_thread(void)
{
	int    blink_status       = 0;
	int    err                = 0;

	printk("Starting Nordic UART service example\n");

	err = init_uart();
	if (!err) {
		err = bt_enable(bt_ready);
	}

	if (!err) {
		bt_conn_cb_register(&conn_callbacks);
		#if !defined(CONFIG_BT_GATT_NUS_SECURITY_LEVEL_LOW) && \
		     defined(CONFIG_BT_GATT_NUS_SECURITY_ENABLED)
		bt_conn_auth_cb_register(&conn_auth_callbacks);
		#endif

		err = k_sem_take(&ble_init_ok, K_MSEC(100));

		if (!err) {
			printk("Bluetooth initialized\n");
		} else {
			printk("BLE initialization \
				did not complete in time\n");
		}
	}

	if (err) {
		error();
	}

	init_leds();

	for (;;) {
		set_led_state(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(RUN_LED_BLINK_INTERVAL);
	}
}

void ble_write_thread(void)
{
	/* Don't go any further until BLE is initailized */
	k_sem_take(&ble_init_ok, K_FOREVER);

	for (;;) {
		/* Wait indefinitely for data to be sent over bluetooth */
		struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data,
						     K_FOREVER);

		if (nus_send(buf->data, buf->len)) {
			printk("Failed to send data over BLE connection\n");
		}
		k_free(buf);
	}
}

K_THREAD_DEFINE(led_blink_thread_id, STACKSIZE, led_blink_thread, NULL, NULL,
		NULL, PRIORITY, 0, K_NO_WAIT);

K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, NULL, NULL,
		NULL, PRIORITY, 0, K_NO_WAIT);

