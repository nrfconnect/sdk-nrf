/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <misc/byteorder.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/hids_c.h>
#include <dk_buttons_and_leds.h>

#include <settings/settings.h>

/**
 * Switch between boot protocol and report protocol mode.
 */
#define KEY_BOOTMODE_MASK DK_BTN2_MSK
/**
 * Switch CAPSLOCK state.
 *
 * @note
 * For simplicity of the code it works only in boot mode.
 */
#define KEY_CAPSLOCK_MASK DK_BTN1_MSK
/**
 * Switch CAPSLOCK state with response
 *
 * Write CAPSLOCK with response.
 * Just for testing purposes.
 * The result should be the same like usine @ref KEY_CAPSLOCK_MASK
 */
#define KEY_CAPSLOCK_RSP_MASK DK_BTN3_MSK

/* Key used to accept or reject passkey value */
#define KEY_PAIRING_ACCEPT DK_BTN1_MSK
#define KEY_PAIRING_REJECT DK_BTN2_MSK

static struct bt_conn *default_conn;
static struct bt_gatt_hids_c hids_c;
static u8_t capslock_state;


static void hids_on_ready(struct k_work *work);
static K_WORK_DEFINE(hids_ready_work, hids_on_ready);


static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	if (!filter_match->uuid.match ||
	    (filter_match->uuid.count != 1)) {

		printk("Invalid device connected\n");

		return;
	}

	const struct bt_uuid *uuid = filter_match->uuid.uuid[0];

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	printk("Filters matched on UUID 0x%04x.\nAddress: %s connectable: %s\n",
		BT_UUID_16(uuid)->val,
		addr, connectable ? "yes" : "no");

	err = bt_scan_stop();
	if (err) {
		printk("Stop LE scan failed (err %d)\n", err);
	}
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static void discovery_completed_cb(struct bt_gatt_dm *dm,
				   void *context)
{
	int err;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_gatt_hids_c_handles_assign(dm, &hids_c);
	if (err) {
		printk("Could not init HIDS client object, error: %d\n", err);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
}

static void discovery_service_not_found_cb(struct bt_conn *conn,
					   void *context)
{
	printk("The service could not be found during the discovery\n");
}

static void discovery_error_found_cb(struct bt_conn *conn,
				     int err,
				     void *context)
{
	printk("The discovery procedure failed with %d\n", err);
}

static const struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed_cb,
	.service_not_found = discovery_service_not_found_cb,
	.error_found = discovery_error_found_cb,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != default_conn) {
		return;
	}

	err = bt_gatt_dm_start(conn, BT_UUID_HIDS, &discovery_cb, NULL);
	if (err) {
		printk("could not start the discovery procedure, error "
			"code: %d\n", err);
	}
}

static void connected(struct bt_conn *conn, u8_t conn_err)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err) {
		printk("Failed to set security: %d\n", err);

		gatt_discover(conn);
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (bt_gatt_hids_c_assign_check(&hids_c)) {
		printk("HIDS client active - releasing");
		bt_gatt_hids_c_release(&hids_c);
	}

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	/* This demo doesn't require active scan */
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
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
		printk("Security failed: %s level %u err %d\n", addr, level,
			err);
	}

	gatt_discover(conn);
}

static struct bt_conn_cb conn_callbacks = {
	.connected        = connected,
	.disconnected     = disconnected,
	.security_changed = security_changed
};

static void scan_init(void)
{
	int err;

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HIDS);
	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);

		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
	}
}

static u8_t hids_c_notify_cb(struct bt_gatt_hids_c *hids_c,
			     struct bt_gatt_hids_c_rep_info *rep,
			     u8_t err,
			     const u8_t *data)
{
	u8_t size = bt_gatt_hids_c_rep_size(rep);
	u8_t i;

	if (!data) {
		return BT_GATT_ITER_STOP;
	}
	printk("Notification, id: %u, size: %u, data:",
	       bt_gatt_hids_c_rep_id(rep),
	       size);
	for (i = 0; i < size; ++i) {
		printk(" 0x%x", data[i]);
	}
	printk("\n");
	return BT_GATT_ITER_CONTINUE;
}

static u8_t hids_c_boot_mouse_report(struct bt_gatt_hids_c *hids_c,
				     struct bt_gatt_hids_c_rep_info *rep,
				     u8_t err,
				     const u8_t *data)
{
	u8_t size = bt_gatt_hids_c_rep_size(rep);
	u8_t i;

	if (!data) {
		return BT_GATT_ITER_STOP;
	}
	printk("Notification, mouse boot, size: %u, data:", size);
	for (i = 0; i < size; ++i) {
		printk(" 0x%x", data[i]);
	}
	printk("\n");
	return BT_GATT_ITER_CONTINUE;
}

static u8_t hids_c_boot_kbd_report(struct bt_gatt_hids_c *hids_c,
				   struct bt_gatt_hids_c_rep_info *rep,
				   u8_t err,
				   const u8_t *data)
{
	u8_t size = bt_gatt_hids_c_rep_size(rep);
	u8_t i;

	if (!data) {
		return BT_GATT_ITER_STOP;
	}
	printk("Notification, keyboard boot, size: %u, data:", size);
	for (i = 0; i < size; ++i) {
		printk(" 0x%x", data[i]);
	}
	printk("\n");
	return BT_GATT_ITER_CONTINUE;
}

static void hids_c_ready_cb(struct bt_gatt_hids_c *hids_c)
{
	k_work_submit(&hids_ready_work);
}

static void hids_on_ready(struct k_work *work)
{
	int err;
	struct bt_gatt_hids_c_rep_info *rep = NULL;

	printk("HIDS is ready to work\n");

	while (NULL != (rep = bt_gatt_hids_c_rep_next(&hids_c, rep))) {
		if (bt_gatt_hids_c_rep_type(rep) ==
		    BT_GATT_HIDS_REPORT_TYPE_INPUT) {
			printk("Subscribe to report id: %u\n",
			       bt_gatt_hids_c_rep_id(rep));
			err = bt_gatt_hids_c_rep_subscribe(&hids_c, rep,
							   hids_c_notify_cb);
			if (err) {
				printk("Subscribe error (%d)\n", err);
			}
		}
	}
	if (hids_c.rep_boot.kbd_inp) {
		printk("Subscribe to boot keyboard report\n");
		err = bt_gatt_hids_c_rep_subscribe(&hids_c,
						   hids_c.rep_boot.kbd_inp,
						   hids_c_boot_kbd_report);
		if (err) {
			printk("Subscribe error (%d)\n", err);
		}
	}
	if (hids_c.rep_boot.mouse_inp) {
		printk("Subscribe to boot mouse report\n");
		err = bt_gatt_hids_c_rep_subscribe(&hids_c,
						   hids_c.rep_boot.mouse_inp,
						   hids_c_boot_mouse_report);
		if (err) {
			printk("Subscribe error (%d)\n", err);
		}
	}
}

static void hids_c_prep_fail_cb(struct bt_gatt_hids_c *hids_c, int err)
{
	printk("ERROR: HIDS client preparation failed!\n");
}

static void hids_c_pm_update_cb(struct bt_gatt_hids_c *hids_c)
{
	printk("Protocol mode updated: %s\n",
	      bt_gatt_hids_c_pm_get(hids_c) == BT_GATT_HIDS_PM_BOOT ?
	      "BOOT" : "REPORT");
}

/* HIDS client initialization parameters */
static const struct bt_gatt_hids_c_init_params hids_c_init_params = {
	.ready_cb      = hids_c_ready_cb,
	.prep_error_cb = hids_c_prep_fail_cb,
	.pm_update_cb  = hids_c_pm_update_cb
};


static void button_bootmode(void)
{
	if (!bt_gatt_hids_c_ready_check(&hids_c)) {
		printk("HID device not ready\n");
		return;
	}
	int err;
	enum bt_gatt_hids_pm pm = bt_gatt_hids_c_pm_get(&hids_c);

	printk("Setting protocol mode: %s\n",
	       (pm == BT_GATT_HIDS_PM_BOOT) ?
	       "BOOT" : "REPORT");
	err = bt_gatt_hids_c_pm_write(&hids_c,
				      (pm == BT_GATT_HIDS_PM_BOOT) ?
				      BT_GATT_HIDS_PM_REPORT :
				      BT_GATT_HIDS_PM_BOOT);
	if (err) {
		printk("Cannot change protocol mode (err %d)\n", err);
	}
}


static void button_capslock(void)
{
	int err;
	u8_t data;

	if (!bt_gatt_hids_c_ready_check(&hids_c)) {
		printk("HID device not ready\n");
		return;
	}
	if (!hids_c.rep_boot.kbd_out) {
		printk("HID device does not have Keyboard OUT report\n");
		return;
	}
	if (bt_gatt_hids_c_pm_get(&hids_c) != BT_GATT_HIDS_PM_BOOT) {
		printk("This function works only in BOOT Report mode\n");
		return;
	}
	capslock_state = capslock_state ? 0 : 1;
	data = capslock_state ? 0x02 : 0;
	err = bt_gatt_hids_c_rep_write_wo_rsp(&hids_c,
					      hids_c.rep_boot.kbd_out,
					      &data, sizeof(data));
	if (err) {
		printk("Keyboard data write error (err: %d)\n", err);
		return;
	}
	printk("Caps lock send (val: 0x%x)\n", data);
}


static u8_t capslock_read_cb(struct bt_gatt_hids_c *hids_c,
			     struct bt_gatt_hids_c_rep_info *rep,
			     u8_t err,
			     const u8_t *data)
{
	if (err) {
		printk("Capslock read error (err: %u)\n", err);
		return BT_GATT_ITER_STOP;
	}
	if (!data) {
		printk("Capslock read - no data\n");
		return BT_GATT_ITER_STOP;
	}
	printk("Received data (size: %u, data[0]: 0x%x)\n",
	       bt_gatt_hids_c_rep_size(rep), data[0]);

	return BT_GATT_ITER_STOP;
}


static void capslock_write_cb(struct bt_gatt_hids_c *hids_c,
			      struct bt_gatt_hids_c_rep_info *rep,
			      u8_t err)
{
	int ret;

	printk("Capslock write result: %u\n", err);

	ret = bt_gatt_hids_c_rep_read(hids_c, rep, capslock_read_cb);
	if (ret) {
		printk("Cannot read capslock value (err: %d)\n", ret);
	}
}


static void button_capslock_rsp(void)
{
	if (!bt_gatt_hids_c_ready_check(&hids_c)) {
		printk("HID device not ready\n");
		return;
	}
	if (!hids_c.rep_boot.kbd_out) {
		printk("HID device does not have Keyboard OUT report\n");
		return;
	}
	int err;
	u8_t data;

	capslock_state = capslock_state ? 0 : 1;
	data = capslock_state ? 0x02 : 0;
	err = bt_gatt_hids_c_rep_write(&hids_c,
				       hids_c.rep_boot.kbd_out,
				       capslock_write_cb,
				       &data, sizeof(data));
	if (err) {
		printk("Keyboard data write error (err: %d)\n", err);
		return;
	}
	printk("Caps lock send using write with response (val: 0x%x)\n", data);
}


static void button_handler(u32_t button_state, u32_t has_changed)
{
	u32_t button = button_state & has_changed;

	if (button & KEY_BOOTMODE_MASK) {
		button_bootmode();
	}
	if (button & KEY_CAPSLOCK_MASK) {
		button_capslock();
	}
	if (button & KEY_CAPSLOCK_RSP_MASK) {
		button_capslock_rsp();
	}
}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}


static void pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_auth_pairing_confirm(conn);

	printk("Pairing confirmed: %s\n", addr);
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

	printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};


void main(void)
{
	int err;

	printk("Starting Bluetooth Central HIDS example\n");

	bt_gatt_hids_c_init(&hids_c, &hids_c_init_params);

	bt_conn_cb_register(&conn_callbacks);

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("failed to register authorization callbacks.\n");
		return;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	scan_init();

	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Failed to initialize buttons (err %d)\n", err);
		return;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}
