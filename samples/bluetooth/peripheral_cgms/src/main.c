/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/services/cgms.h>
#include <sfloat.h>

#define APP_GLUCOSE_MIN    88
#define APP_GLUCOSE_MAX    92
#define APP_GLUCOSE_STEP   0.1f

static bool session_state;
static struct k_work adv_work;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
				BT_UUID_16_ENCODE(BT_UUID_CGMS_VAL),
				BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

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

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	if (err) {
		printk("Failed to connect to %s, err 0x%02x %s\n", addr, err,
		       bt_hci_err_to_str(err));
	} else {
		printk("Connected %s\n", addr);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected from %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
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
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
	.recycled = recycled_cb,
};

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	printk("Pairing key is %06d.\n", passkey);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
	.passkey_display = auth_passkey_display,
};

static void cgms_session_state_changed(const bool state)
{
	session_state = state;
	if (state) {
		printk("Session starts.\n");
	} else {
		printk("Session stops.\n");
	}
}

int main(void)
{
	int err;
	const uint8_t measurement_interval = 1; /* time in minutes. */
	/* The time it will try to submit a record if last attempt fails. */
	const uint8_t retry_count = 3;
	/* The time interval between two attempts. Unit is second. */
	const uint8_t retry_interval = 1;
	float glucose = APP_GLUCOSE_MIN;
	struct bt_cgms_measurement result;
	struct bt_cgms_cb cb;
	struct bt_cgms_init_param params;

	printk("Starting Bluetooth Peripheral CGM sample\n");

	bt_conn_auth_cb_register(&auth_cb_display);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	params.type = BT_CGMS_FEAT_TYPE_CAP_PLASMA;
	params.sample_location = BT_CGMS_FEAT_LOC_FINGER;
	/* The session will run 1 hour */
	params.session_run_time = 1;
	/* The initial communication interval will be 5 minutes */
	params.initial_comm_interval = 5;

	cb.session_state_changed = cgms_session_state_changed;
	params.cb = &cb;

	err = bt_cgms_init(&params);
	if (err) {
		printk("Error occurred when initializing cgm service (err %d)\n", err);
		return 0;
	}

	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

	/* Submit the measured glucose result in main loop. */
	while (1) {
		if (session_state) {
			/* Users can implement the routine that gets the glucose measurement.
			 * Here, we use dummy glucose results, which increase from APP_GLUCOSE_MIN
			 * to APP_GLUCOSE_MAX, with step of APP_GLUCOSE_STEP.
			 */
			glucose += APP_GLUCOSE_STEP;
			if (glucose > APP_GLUCOSE_MAX) {
				glucose = APP_GLUCOSE_MIN;
			}
			result.glucose = sfloat_from_float(glucose);

			/* Check return value. If it is error code, which means submission
			 * to database fails, retry until it reaches limit.
			 */
			for (int i = 0; i < retry_count; i++) {
				err = bt_cgms_measurement_add(result);
				if (err == 0) {
					break;
				}
				k_sleep(K_SECONDS(retry_interval));
			}
			if (err < 0) {
				printk("Cannot submit measurement result. Record discarded.\n");
			}
		}
		/* Sleep until next sampling time arrives. */
		k_sleep(K_MINUTES(measurement_interval));
	}
}
