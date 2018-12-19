/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <gpio.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/lbs.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

/* Change this if you have a LED connected to a custom port */
#ifndef LED0_GPIO_CONTROLLER
#define LED0_GPIO_CONTROLLER	LED0_GPIO_PORT
#endif

#define LED_PORT                LED0_GPIO_CONTROLLER

#define RUN_STATUS_LED          LED0_GPIO_PIN
#define CON_STATUS_LED          LED1_GPIO_PIN
#define RUN_LED_BLINK_INTERVAL  1000

#define LED_ON                  0
#define LED_OFF                 1

#define USER_LED                LED2_GPIO_PIN

/* change this to use another GPIO port */
#ifndef SW0_GPIO_CONTROLLER
#ifdef SW0_GPIO_NAME
#define SW0_GPIO_CONTROLLER SW0_GPIO_NAME
#else
#error SW0_GPIO_NAME or SW0_GPIO_CONTROLLER needs to be set in soc.h
#endif
#endif
#define BUTTON_PORT SW0_GPIO_CONTROLLER

/* change this to use another GPIO pin */
#ifdef SW0_GPIO_PIN
#define USER_BUTTON SW0_GPIO_PIN
#else
#error SW0_GPIO_PIN needs to be set in soc.h
#endif

/* change to use another GPIO pin interrupt config */
#ifdef SW0_GPIO_INT_CONF
#define EDGE_SENSE_CONF SW0_GPIO_INT_CONF
#else
/*
 * If SW0_GPIO_INT_CONF not defined used default EDGE value.
 * Change this to use a different interrupt trigger
 */
#define EDGE_SENSE_CONF (GPIO_INT_EDGE | \
			 GPIO_INT_DOUBLE_EDGE | \
			 GPIO_INT_ACTIVE_LOW)
#endif

/* change this to enable pull-up/pull-down */
#ifdef SW0_GPIO_PIN_PUD
#define PULL_UP SW0_GPIO_PIN_PUD
#else
#define PULL_UP	0
#endif

static K_SEM_DEFINE(ble_init_ok, 0, 1);

static struct device        *button_port;
static struct device        *led_port;

static struct gpio_callback button_cb;
static bool                 button_state;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, LBS_UUID_SERVICE),
};

static void set_led_state(int led, bool state)
{
	if (led_port) {
		gpio_pin_write(led_port, led, state);
	}
}

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	printk("Connected\n");

	set_led_state(CON_STATUS_LED, LED_ON);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	set_led_state(CON_STATUS_LED, LED_OFF);
}

#ifdef CONFIG_BT_GATT_LBS_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	printk("Security level was raised to %d\n", level);
}
#endif

static struct bt_conn_cb conn_callbacks = {
	.connected        = connected,
	.disconnected     = disconnected,
#ifdef CONFIG_BT_GATT_LBS_SECURITY_ENABLED
	.security_changed = security_changed,
#endif
};

#if defined(CONFIG_BT_GATT_LBS_SECURITY_ENABLED)
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
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
#endif

static void app_led_cb(bool led_state)
{
	set_led_state(USER_LED, !led_state);
}

static bool app_button_cb(void)
{
	return button_state;
}

static struct bt_lbs_cb lbs_callbacs = {
	.led_cb    = app_led_cb,
	.button_cb = app_button_cb,
};

static void bt_ready(int err)
{
	if (err) {
		printk("BLE init failed with error code %d\n", err);
		return;
	}

	err = lbs_init(&lbs_callbacs);

	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	k_sem_give(&ble_init_ok);
}

static void button_pressed(struct device *button_port, struct gpio_callback *cb,
		    u32_t pins)
{
	button_state = button_state ^ 0x01;
	lbs_send_button_state(button_state);
}

static int init_button(void)
{
	int err;

	button_port = device_get_binding(BUTTON_PORT);
	if (!button_port) {
		printk("Could not bind to BUTTON port\n");
		return -ENXIO;
	}

	err = gpio_pin_configure(button_port, USER_BUTTON,
			   GPIO_DIR_IN | GPIO_INT | PULL_UP |
			   EDGE_SENSE_CONF);

	if (!err) {
		gpio_init_callback(&button_cb,
				    button_pressed,
				    BIT(USER_BUTTON));
		err = gpio_add_callback(button_port, &button_cb);
	}

	if (!err) {
		err = gpio_pin_enable_callback(button_port, USER_BUTTON);
	}

	if (err) {
		printk("Unable to initialize button pin (err:%d)\n", err);
	}

	return err;
}

static void init_leds(void)
{
	int err;

	led_port = device_get_binding(LED_PORT);
	if (!led_port) {
		printk("Could not bind to LED port\n");
	}

	err = gpio_pin_configure(led_port, RUN_STATUS_LED, GPIO_DIR_OUT);
	err += gpio_pin_configure(led_port, USER_LED, GPIO_DIR_OUT);
	err += gpio_pin_configure(led_port, CON_STATUS_LED, GPIO_DIR_OUT);
	if (!err) {
		err = gpio_port_write(led_port, LED_OFF << RUN_STATUS_LED |
						LED_OFF << USER_LED |
						LED_OFF << CON_STATUS_LED);
	}

	if (err) {
		printk("Unable to initialize LED pins (err:%d)\n", err);
		led_port = NULL;
	}
}

static void error(void)
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

void main(void)
{
	int blink_status = 0;
	int err;

	printk("Starting Nordic LED-Button service example\n");

	err = init_button();
	if (!err) {
		err = bt_enable(bt_ready);
	}

	if (!err) {
		bt_conn_cb_register(&conn_callbacks);

		if (IS_ENABLED(CONFIG_BT_GATT_LBS_SECURITY_ENABLED)) {
			bt_conn_auth_cb_register(&conn_auth_callbacks);
		}

		/* Bluetooth stack should be ready in less than 100 msec */
		err = k_sem_take(&ble_init_ok, K_MSEC(100));

		if (!err) {
			printk("Bluetooth initialized\n");
		} else {
			printk("BLE initialization did not complete in time\n");
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
