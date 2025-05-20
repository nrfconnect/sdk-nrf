/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/services/cts_client.h>

#include <zephyr/settings/settings.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

#define KEY_READ_TIME DK_BTN1_MSK

static struct bt_cts_client cts_c;
static struct k_work adv_work;
static bool has_cts;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
		      (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_LIMITED | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_SOLICIT16, BT_UUID_16_ENCODE(BT_UUID_CTS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const char *day_of_week[] = { "Unknown",	  "Monday",   "Tuesday",
				     "Wednesday", "Thursday", "Friday",
				     "Saturday",  "Sunday" };

static const char *month_of_year[] = { "Unknown",   "January", "February",
				       "March",	    "April",   "May",
				       "June",	    "July",    "August",
				       "September", "October", "November",
				       "December" };

static void current_time_print(struct bt_cts_current_time *current_time)
{
	printk("\nCurrent Time:\n");
	printk("\nDate:\n");

	printk("\tDay of week   %s\n",
	       day_of_week[current_time->exact_time_256.day_of_week]);

	if (current_time->exact_time_256.day == 0) {
		printk("\tDay of month  Unknown\n");
	} else {
		printk("\tDay of month  %u\n",
		       current_time->exact_time_256.day);
	}

	printk("\tMonth of year %s\n",
	       month_of_year[current_time->exact_time_256.month]);
	if (current_time->exact_time_256.year == 0) {
		printk("\tYear          Unknown\n");
	} else {
		printk("\tYear          %u\n",
		       current_time->exact_time_256.year);
	}
	printk("\nTime:\n");
	printk("\tHours     %u\n", current_time->exact_time_256.hours);
	printk("\tMinutes   %u\n", current_time->exact_time_256.minutes);
	printk("\tSeconds   %u\n", current_time->exact_time_256.seconds);
	printk("\tFractions %u/256 of a second\n",
	       current_time->exact_time_256.fractions256);

	printk("\nAdjust reason:\n");
	printk("\tDaylight savings %x\n",
	       current_time->adjust_reason.change_of_daylight_savings_time);
	printk("\tTime zone        %x\n",
	       current_time->adjust_reason.change_of_time_zone);
	printk("\tExternal update  %x\n",
	       current_time->adjust_reason.external_reference_time_update);
	printk("\tManual update    %x\n",
	       current_time->adjust_reason.manual_time_update);
}

static void notify_current_time_cb(struct bt_cts_client *cts_c,
				   struct bt_cts_current_time *current_time)
{
	current_time_print(current_time);
}

static void enable_notifications(void)
{
	int err;

	if (has_cts && (bt_conn_get_security(cts_c.conn) >= BT_SECURITY_L2)) {
		err = bt_cts_subscribe_current_time(&cts_c,
						    notify_current_time_cb);
		if (err) {
			printk("Cannot subscribe to current time value notification (err %d)\n",
			       err);
		}
	}
}

static void discover_completed_cb(struct bt_gatt_dm *dm, void *ctx)
{
	int err;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_cts_handles_assign(dm, &cts_c);
	if (err) {
		printk("Could not assign CTS client handles, error: %d\n", err);
	} else {
		has_cts = true;

		if (bt_conn_get_security(cts_c.conn) < BT_SECURITY_L2) {
			err = bt_conn_set_security(cts_c.conn, BT_SECURITY_L2);
			if (err) {
				printk("Failed to set security (err %d)\n",
				       err);
			}
		} else {
			enable_notifications();
		}
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n",
		       err);
	}
}

static void discover_service_not_found_cb(struct bt_conn *conn, void *ctx)
{
	printk("The service could not be found during the discovery\n");
}

static void discover_error_found_cb(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
}

static const struct bt_gatt_dm_cb discover_cb = {
	.completed = discover_completed_cb,
	.service_not_found = discover_service_not_found_cb,
	.error_found = discover_error_found_cb,
};


static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected %s\n", addr);

	dk_set_led_on(CON_STATUS_LED);

	has_cts = false;

	err = bt_gatt_dm_start(conn, BT_UUID_CTS, &discover_cb, NULL);
	if (err) {
		printk("Failed to start discovery (err %d)\n", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected from %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	dk_set_led_off(CON_STATUS_LED);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);

		enable_notifications();
	} else {
		printk("Security failed: %s level %u err %d %s\n", addr, level, err,
		       bt_security_err_to_str(err));
	}
}

static void recycled_cb(void)
{
	printk("Connection object available from previous conn. Disconnect is complete!\n");
	advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
	.security_changed = security_changed,
	.recycled         = recycled_cb,
};

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);

	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d %s\n", addr, reason,
	       bt_security_err_to_str(reason));

	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

static void read_current_time_cb(struct bt_cts_client *cts_c,
				 struct bt_cts_current_time *current_time,
				 int err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(cts_c->conn), addr, sizeof(addr));

	if (err) {
		printk("Cannot read Current Time: %s, error: %d\n", addr, err);
		return;
	}

	current_time_print(current_time);
}

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;
	int err;

	if (buttons & KEY_READ_TIME) {
		err = bt_cts_read_current_time(&cts_c, read_current_time_cb);
		if (err) {
			printk("Failed reading current time (err: %d)\n", err);
		}
	}
}

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	return err;
}

int main(void)
{
	int blink_status = 0;
	int err;

	printk("Starting Current Time Service client sample\n");

	err = bt_cts_client_init(&cts_c);
	if (err) {
		printk("CTS client init failed (err %d)\n", err);
		return 0;
	}

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return 0;
	}

	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("BLE init failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks\n");
		return 0;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return 0;
	}

	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
