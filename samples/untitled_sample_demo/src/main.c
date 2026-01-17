/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <string.h>

#if defined(CONFIG_BT_MDS)
#include <bluetooth/services/mds.h>
#endif

#if defined(CONFIG_SETTINGS)
#include <zephyr/settings/settings.h>
#endif

#if defined(CONFIG_APP_SHOW_VERSION)
#include <app_version.h>
#endif

/* Declare defines here (if any). */
#define LED_BLINK_TIME K_MSEC(750)

/* Declare enums here (if any). */

/* Declare structs here (if any). */

/* Declare consts here (if any). */

/* Register logging here (if used). */
LOG_MODULE_REGISTER(untitled_sample_demo, CONFIG_APP_LOG_LEVEL);

/* Declare function prototypes here (if any). */
static void blinky_timer_handler(struct k_timer *timer);
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void recycled_cb(void);

#if defined(CONFIG_BT_MDS)
static void crash_application(struct k_work *work);
static void sw0_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
static bool mds_access_enable(struct bt_conn *conn);
#endif

#if defined(CONFIG_BT_SMP)
/*
 * Line length is 100 characters, this line is on the limit for maximum line length size before
 * needing to be split up into multiple.
 */
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);
#endif

/* Declare variables here (if any). */
static struct k_work adv_work;
static const struct gpio_dt_spec blinky_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec connection_led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

#if defined(CONFIG_BT_SMP)
static const struct gpio_dt_spec encryption_led = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
#endif


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, (sizeof(CONFIG_BT_DEVICE_NAME) - 1)),
};

static const struct bt_data sd[] = {
	/*
	 * Advertise with just memfault diagnostic service UUID if that is enabled, otherwise use
	 * the MCUmgr UUID. There is not enough space in the packet to add both UUIDs and remain
	 * visible to legacy devices, but MDS implies MCUmgr.
	 */
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
#if defined(CONFIG_BT_MDS)
		      BT_UUID_MDS_VAL
#else
		      SMP_BT_SVC_UUID_VAL
#endif
	),
};

#if defined(CONFIG_BT_MDS)
static struct k_work sw0_button_pressed_work;
static const struct gpio_dt_spec sw0_button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback sw0_button_pressed_data;
static struct bt_conn *mds_conn;

static const struct bt_mds_cb mds_cb = {
	.access_enable = mds_access_enable,
};
#endif

/*
 * No additional space padding is used to align variable at the same place on the previous line,
 * just one space either side of the `=`.
 */
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled_cb,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif
};

static K_TIMER_DEFINE(blinky_timer, blinky_timer_handler, NULL);

/* Define function implementations here */
static void blinky_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	gpio_pin_toggle_dt(&blinky_led);
}

#if defined(CONFIG_BT_MDS)
static void crash_application(struct k_work *work)
{
	volatile uint32_t i;

	LOG_INF("About to crash application...");
	LOG_PANIC();
	k_sleep(K_SECONDS(2));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"
	i = 1 / 0;
#pragma GCC diagnostic pop
}

static void sw0_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	k_work_submit(&sw0_button_pressed_work);
}

static bool mds_access_enable(struct bt_conn *conn)
{
	if (mds_conn != NULL && conn == mds_conn) {
		return true;
	}

	return false;
}
#endif

static void adv_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	int rc = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (rc) {
		LOG_ERR("Advertising failed to start: %d", rc);
		return;
	}

	LOG_INF("Advertising successfully started");
}

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

static void connected(struct bt_conn *conn, uint8_t reason)
{
#if !defined(CONFIG_BT_MDS) || defined(CONFIG_BT_SMP)
	ARG_UNUSED(conn);
#endif

	if (reason != 0) {
		LOG_ERR("Connection failed: 0x%02x (%s)", reason, bt_hci_err_to_str(reason));
		return;
	}

	LOG_INF("Connected");

#if !defined(CONFIG_BT_SMP) && defined(CONFIG_BT_MDS)
	if (mds_conn == NULL) {
		mds_conn = conn;
	}
#endif

	if (gpio_is_ready_dt(&connection_led)) {
		gpio_pin_set_dt(&connection_led, 1);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
#if !defined(CONFIG_BT_MDS)
	ARG_UNUSED(conn);
#endif

	LOG_INF("Disconnected: 0x%02x (%s)", reason, bt_hci_err_to_str(reason));

#if defined(CONFIG_BT_MDS)
	if (conn == mds_conn) {
		mds_conn = NULL;
	}
#endif

	if (gpio_is_ready_dt(&connection_led)) {
		gpio_pin_set_dt(&connection_led, 0);
	}

#if defined(CONFIG_BT_SMP)
	if (gpio_is_ready_dt(&encryption_led)) {
		gpio_pin_set_dt(&encryption_led, 0);
	}
#endif
}

static void recycled_cb(void)
{
	LOG_INF("Connection object available from previous conn. Disconnect complete!");
	advertising_start();
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
#if !defined(CONFIG_BT_MDS)
	ARG_UNUSED(conn);
#endif

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err == 0) {
		LOG_INF("Security changed: %s level %u", addr, level);

#if defined(CONFIG_BT_MDS)
		if (level >= BT_SECURITY_L2) {
			if (mds_conn == NULL) {
				mds_conn = conn;
			}
		}
#endif

		if (gpio_is_ready_dt(&encryption_led)) {
			gpio_pin_set_dt(&encryption_led, (level >= BT_SECURITY_L2 ? 1 : 0));
		}
	} else {
		LOG_ERR("Security failed: %d (%s) %s level %u", err, bt_security_err_to_str(err),
			addr, level);

		if (gpio_is_ready_dt(&encryption_led)) {
			gpio_pin_set_dt(&encryption_led, 0);
		}
	}
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	ARG_UNUSED(conn);

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_ERR("Pairing failed: %d (%s) for device %s", reason, bt_security_err_to_str(reason),
		addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};
#endif

int main(void)
{
	int rc;

#if defined(CONFIG_APP_SHOW_VERSION)
	LOG_INF("Application version: " APP_VERSION_STRING);
#endif

#if defined(CONFIG_APP_SHOW_BUILD_DATE)
	LOG_INF("Built on: " __DATE__);
#endif

#if defined(CONFIG_BT_SMP)
	LOG_INF("Bluetooth security enabled");
#endif

	if (gpio_is_ready_dt(&blinky_led)) {
		rc = gpio_pin_configure_dt(&blinky_led, GPIO_OUTPUT_INACTIVE);

		if (rc == 0) {
			k_timer_start(&blinky_timer, LED_BLINK_TIME, LED_BLINK_TIME);
		} else {
			LOG_ERR("Blinky LED (led0) configure failed: %d", rc);
		}
	} else {
		LOG_ERR("Blinky LED node (led0) is not ready");
	}

	if (gpio_is_ready_dt(&connection_led)) {
		rc = gpio_pin_configure_dt(&connection_led, GPIO_OUTPUT_INACTIVE);

		if (rc != 0) {
			LOG_ERR("Connection LED (led1) configure failed: %d", rc);
		}
	} else {
		LOG_ERR("Connection LED node (led1) is not ready");
	}

#if defined(CONFIG_BT_SMP)
	if (gpio_is_ready_dt(&encryption_led)) {
		rc = gpio_pin_configure_dt(&encryption_led, GPIO_OUTPUT_INACTIVE);

		if (rc != 0) {
			LOG_ERR("Encryption LED (led2) configure failed: %d", rc);
		}
	} else {
		LOG_ERR("Encryption LED node (led2) is not ready");
	}

	rc = bt_conn_auth_cb_register(&conn_auth_callbacks);

	if (rc != 0) {
		LOG_ERR("Failed to register Bluetooth authorization callback: %d", rc);
		return 0;
	}

	rc = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);

	if (rc != 0) {
		LOG_ERR("Failed to register Bluetooth authorization info callback: %d", rc);
		return 0;
	}
#endif

#if defined(CONFIG_BT_MDS)
	if (gpio_is_ready_dt(&sw0_button)) {
		rc = gpio_pin_configure_dt(&sw0_button, GPIO_INPUT);

		if (rc == 0) {
			rc = gpio_pin_interrupt_configure_dt(&sw0_button, GPIO_INT_EDGE_TO_ACTIVE);

			if (rc == 0) {
				gpio_init_callback(&sw0_button_pressed_data, sw0_button_pressed,
						   BIT(sw0_button.pin));
				gpio_add_callback(sw0_button.port, &sw0_button_pressed_data);
				k_work_init(&sw0_button_pressed_work, crash_application);
				LOG_INF("Press sw0 button to simulate application crash");
			} else {
				LOG_ERR("Crash button (sw01) interrupt configure failed: %d", rc);
			}
		} else {
			LOG_ERR("Crash button (sw01) configure failed: %d", rc);
		}
	} else {
		LOG_ERR("Connection LED node (led1) is not ready");
	}

	rc = bt_mds_cb_register(&mds_cb);

	if (rc != 0) {
		LOG_ERR("Failed to register Memfault Diagnostic service: %d", rc);
		return 0;
	}
#endif

	/*
	 * Enable Bluetooth and wait for it to init (without callback) before proceeding. Note
	 * that the return code to this is an int, and the if checks the return value (int)
	 * against the success code (int, value 0) rather than wrongly just checking for rc,
	 * which would be non-compliant with MISRA rule 14.4.
	 */
	rc = bt_enable(NULL);

	if (rc != 0) {
		/*
		 * Logging is used for outputting error messages, logging can be set up on a
		 * per-module basis in Kconfig and the level of logging per module can also be
		 * configured. Log messages are automatically end-of-line'd so should not have a
		 * `\n` at the end. In this case, rc returned an error code, so the error log
		 * macro is used. Because of the configurability and to allow usage of different
		 * logging backends (e.g. UART, RTT, network, etc.) its usage is far preferred
		 * over using `printk()`.
		 */
		LOG_ERR("Bluetooth init failed: %d", rc);

		/*
		 * Despite the application having failed, `main()` must currently return 0 as the
		 * status is not checked.
		 */
		return 0;
	}

#if defined(CONFIG_SETTINGS)
	settings_load();
#endif

	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

	return 0;
}
