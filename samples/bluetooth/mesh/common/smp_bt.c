/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>

/* .. include_startingpoint_mesh_smp_dfu_rst_1 */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "smp_bt_auth.h"

static void pending_adv_start(struct k_work *work);

static struct bt_le_ext_adv *adv;
/* This will result in the device advertising SMP service continuously. If this is undesirable,
 * set timeout to a finite value, and then create a mechanism in the application (e.g. by pressing
 * a button) to start SMP advertisements when needed by calling @ref bt_le_ext_adv_start.
 */
static struct bt_le_ext_adv_start_param ext_adv_param = { .num_events = 0, .timeout = 0 };
static struct k_work_delayable adv_work = Z_WORK_DELAYABLE_INITIALIZER(pending_adv_start);

static struct bt_le_adv_param adv_params = {
	.id = BT_ID_DEFAULT,
	.sid = 0,
	.secondary_max_skip = 0,
	.options = BT_LE_ADV_OPT_CONN,
	.interval_min = BT_GAP_ADV_SLOW_INT_MIN,
	.interval_max = BT_GAP_ADV_SLOW_INT_MAX,
	.peer = NULL
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86, 0xd3,
		      0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void pending_adv_start(struct k_work *work)
{
	int err;

	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		printk("Unable to restart SMP service advertising (err %d)\n", err);
		return;
	}

	printk("SMP service advertising restarted\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info cinfo;
	int ec;

	ec = bt_conn_get_info(conn, &cinfo);
	if (ec) {
		printk("Unable to get connection info (err %d)\n", ec);
		return;
	}

	if (cinfo.id != adv_params.id) {
		return;
	}

	printk("Connected to SMP service: err %d id %d\n", err, cinfo.id);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err;
	struct bt_conn_info cinfo;

	err = bt_conn_get_info(conn, &cinfo);
	if (err) {
		printk("Unable to get connection info (err %d)\n", err);
		return;
	}

	if (cinfo.id != adv_params.id) {
		return;
	}

	printk("Disconnected from SMP service: reason %d id %d\n", reason, cinfo.id);

	k_work_schedule(&adv_work, K_NO_WAIT);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

int smp_service_adv_init(void)
{
	int err;
	size_t id_count = 0xFF;
	int id;

	/* Use different identity from Bluetooth Mesh to avoid conflicts with Mesh Provisioning
	 * Service and Mesh Proxy Service advertisements.
	 */
	bt_id_get(NULL, &id_count);

	if (id_count < CONFIG_BT_ID_MAX) {
		id = bt_id_create(NULL, NULL);
		if (id < 0) {
			printk("Unable to create a new identity for SMP (err %d)."
			       " Using the default one.\n", id);
			id = BT_ID_DEFAULT;
		}

		printk("Created a new identity for SMP: %d\n", id);
	} else {
		id = BT_ID_DEFAULT + 1;
		printk("Recovered identity for SMP: %d\n", id);
	}

	adv_params.id = id;

	err = bt_le_ext_adv_create(&adv_params, NULL, &adv);
	if (err) {
		printk("Creating SMP service adv instance failed (err %d)\n", err);
		return err;
	}

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Setting SMP service adv data failed (err %d)\n", err);
		return err;
	}

	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		printk("Starting advertising of SMP service failed (err %d)\n", err);
	}

	return err;
}
/* .. include_endpoint_mesh_smp_dfu_rst_1 */

int smp_dfu_init(void)
{
	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN) &&
	    IS_ENABLED(CONFIG_BT_MESH_LE_PAIR_RESP)) {
		int err;

		err = smp_bt_auth_init();
		if (err) {
			printk("Can't enable authentication for SMP (err: %d)\n", err);
			return err;
		}
	} else {
		printk("Authentication for SMP over BLE is disabled\n");
	}

/* .. include_startingpoint_mesh_smp_dfu_rst_2 */
	bt_conn_cb_register(&conn_callbacks);

	/**
	 * Since Bluetooth Mesh utilizes the advertiser as the main channel of
	 * communication, a secondary advertising set is necessary to broadcast
	 * the SMP service.
	 */
	return smp_service_adv_init();
/* .. include_endpoint_mesh_smp_dfu_rst_2 */
}
