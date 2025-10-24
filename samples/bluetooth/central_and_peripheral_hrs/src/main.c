/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <bluetooth/services/hrs_client.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/settings/settings.h>

#include <zephyr/kernel.h>

#define STACKSIZE 1024
#define PRIORITY 7

#define RUN_STATUS_LED             DK_LED1
#define CENTRAL_CON_STATUS_LED	   DK_LED2
#define PERIPHERAL_CONN_STATUS_LED DK_LED3

#define RUN_LED_BLINK_INTERVAL 1000

#define HRS_QUEUE_SIZE 16

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		(CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
		(CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL)), /* Heart Rate Service */
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME)
};

K_MSGQ_DEFINE(hrs_queue, sizeof(struct bt_hrs_client_measurement), HRS_QUEUE_SIZE, 4);

static struct bt_hrs_client hrs_c;
static struct bt_conn *central_conn;
static struct k_work adv_work;

static const char * const sensor_location_str[] = {
	"Other",
	"Chest",
	"Wrist",
	"Finger",
	"Hand",
	"Ear lobe",
	"Foot"
};

static void hrs_sensor_location_read_cb(struct bt_hrs_client *hrs_c,
					enum bt_hrs_client_sensor_location location,
					int err)
{
	if (err) {
		printk("HRS Sensor Body Location read error (err %d)\n", err);
		return;
	}

	printk("Heart Rate Sensor body location: %s\n", sensor_location_str[location]);
}

static void hrs_measurement_notify_cb(struct bt_hrs_client *hrs_c,
				      const struct bt_hrs_client_measurement *meas,
				      int err)
{
	if (err) {
		printk("Error during receiving Heart Rate Measurement notification, err: %d\n",
			err);
		return;
	}

	printk("Heart Rate Measurement notification received:\n\n");
	printk("\tHeart Rate Measurement Value Format: %s\n",
		meas->flags.value_format ? "16 - bit" : "8 - bit");
	printk("\tSensor Contact detected: %d\n", meas->flags.sensor_contact_detected);
	printk("\tSensor Contact supported: %d\n", meas->flags.sensor_contact_supported);
	printk("\tEnergy Expended present: %d\n", meas->flags.energy_expended_present);
	printk("\tRR-Intervals present: %d\n", meas->flags.rr_intervals_present);

	printk("\n\tHeart Rate Measurement Value: %d bpm\n", meas->hr_value);

	if (meas->flags.energy_expended_present) {
		printk("\n\tEnergy Expended: %d J\n", meas->energy_expended);
	}

	if (meas->flags.rr_intervals_present) {
		printk("\t RR-intervals: ");
		for (size_t i = 0; i < meas->rr_intervals_count; i++) {
			printk("%d ", meas->rr_intervals[i]);
		}
	}

	printk("\n");

	err = k_msgq_put(&hrs_queue, meas, K_NO_WAIT);
	if (err) {
		printk("Notification queue is full. Discarting HRS notification (err %d)\n", err);
	}
}

static void discovery_completed_cb(struct bt_gatt_dm *dm,
				   void *context)
{
	int err;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_hrs_client_handles_assign(dm, &hrs_c);
	if (err) {
		printk("Could not init HRS client object (err %d)\n", err);
		return;
	}

	err = bt_hrs_client_sensor_location_read(&hrs_c, hrs_sensor_location_read_cb);
	if (err) {
		printk("Could not read Heart Rate Sensor location (err %d)\n", err);
	}

	err = bt_hrs_client_measurement_subscribe(&hrs_c, hrs_measurement_notify_cb);
	if (err) {
		printk("Could not subscribe to Heart Rate Measurement characteristic (err %d)\n",
		       err);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data (err %d)\n", err);
	}
}

static void discovery_not_found_cb(struct bt_conn *conn,
				   void *context)
{
	printk("Heart Rate Service could not be found during the discovery\n");
}

static void discovery_error_found_cb(struct bt_conn *conn,
				     int err, void *context)
{
	printk("The discovery procedure failed with %d\n", err);
}

static const struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed_cb,
	.service_not_found = discovery_not_found_cb,
	.error_found = discovery_error_found_cb
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	err = bt_gatt_dm_start(conn, BT_UUID_HRS, &discovery_cb, NULL);
	if (err) {
		printk("Could not start the discovery procedure, error "
			   "code: %d\n", err);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
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
}

static const struct bt_conn_auth_cb auth_callbacks = {.cancel = auth_cancel};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

static int scan_start(void)
{
	int err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);

	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
	}

	return err;
}

static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

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

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	struct bt_conn_info info;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s, 0x%02x %s\n", addr, conn_err,
		       bt_hci_err_to_str(conn_err));

		if (conn == central_conn) {
			bt_conn_unref(central_conn);
			central_conn = NULL;

			scan_start();
		}

		return;
	}

	printk("Connected: %s\n", addr);

	err = bt_conn_get_info(conn, &info);
	if (err) {
		printk("Failed to get connection info (err %d)\n", err);
		return;
	}

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		dk_set_led_on(CENTRAL_CON_STATUS_LED);

		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			printk("Failed to set security (err %d)\n", err);

			gatt_discover(conn);
		}
	} else {
		dk_set_led_on(PERIPHERAL_CONN_STATUS_LED);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	if (conn == central_conn) {
		dk_set_led_off(CENTRAL_CON_STATUS_LED);

		bt_conn_unref(central_conn);
		central_conn = NULL;

		scan_start();
	} else {
		dk_set_led_off(PERIPHERAL_CONN_STATUS_LED);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d %s\n", addr, level, err,
		       bt_security_err_to_str(err));
	}

	if (conn == central_conn) {
		gatt_discover(conn);
	}
}

static void recycled_cb(void)
{
	printk("Connection object available from previous conn. Disconnect is complete!\n");
	advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.recycled = recycled_cb,
};

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n",
		   addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	central_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static void scan_init(void)
{
	int err;

	struct bt_scan_init_param param = {
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT,
		.connect_if_match = 1
	};

	bt_scan_init(&param);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
	}
}

static void hrs_notify_thread(void)
{
	struct bt_hrs_client_measurement meas;
	uint16_t heart_rate;

	while (1) {
		k_msgq_get(&hrs_queue, &meas, K_FOREVER);

		if (meas.flags.value_format) {
			heart_rate = (uint8_t)(meas.hr_value >> 0x08);
		} else {
			heart_rate = meas.hr_value;
		}

		bt_hrs_notify(heart_rate);
	}
}

int main(void)
{
	int err;
	int blink_status = 0;

	printk("Starting Bluetooth Central and Peripheral Heart Rate relay sample\n");

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return 0;
	}

	err = bt_conn_auth_cb_register(&auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks.\n");
		return 0;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_hrs_client_init(&hrs_c);
	if (err) {
		printk("Heart Rate Service client failed to init (err %d)\n", err);
		return 0;
	}

	scan_init();

	err = scan_start();
	if (err) {
		return 0;
	}

	printk("Scanning started\n");

	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}

K_THREAD_DEFINE(hrs_notify_thread_id, STACKSIZE, hrs_notify_thread,
		NULL, NULL, NULL, PRIORITY, 0, 0);
