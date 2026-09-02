/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

LOG_MODULE_REGISTER(peripheral_multi_id, LOG_LEVEL_INF);

#define DEVICE_NAME_BUF_SIZE 25 /* The size of device name buffer. */

/* Structure to hold information about each advertiser. */
struct advertiser_info {
	struct k_work work;           /* Work item for the workqueue */
	struct bt_le_ext_adv *adv;    /* Pointer to the advertising set */
	uint8_t id;                   /* ID associated with the advertiser */
};

#define MIN_ADV_INTERVAL (700)
#define MAX_ADV_INTERVAL (700)

static struct advertiser_info advertisers[CONFIG_BT_EXT_ADV_MAX_ADV_SET];

static void start_connectable_advertiser(struct k_work *work);

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_WRN("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
		return;
	}

	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));
	LOG_INF("connected to %s", addr_str);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	LOG_INF("disconnected %s, reason %u", addr_str, reason);

	struct bt_conn_info connection_info;
	int err;

	err = bt_conn_get_info(conn, &connection_info);

	if (err) {
		LOG_ERR("Failed to get conn info (err %d)", err);
		return;
	}

	/* Get the ID of the disconnected advertiser. */
	uint8_t id_current = connection_info.id;

	LOG_INF("Advertiser %d disconnected", id_current);

	/* Restart the advertiser by submitting its work to the workqueue. */
	k_work_submit(&advertisers[id_current].work);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void start_connectable_advertiser(struct k_work *work)
{
	int err;

	struct advertiser_info *current_adv_info =
		CONTAINER_OF(work, struct advertiser_info, work);

	if (current_adv_info->adv) {
		err = bt_le_ext_adv_start(current_adv_info->adv, BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			LOG_ERR("Failed to start advertising set (err %d)", err);
			return;
		}
	} else {
		LOG_ERR("Advertiser not setup correctly");
		return;
	}

	LOG_INF("Advertiser %d successfully started", current_adv_info->id);
}

static int create_id(uint8_t id_adv)
{
	size_t id_count = 0xFF;

	/* Retrieve the number of currently configured identities. */
	bt_id_get(NULL, &id_count);

	/* Check if the identity has already been created. */
	if (id_adv == id_count) {
		int id;

		/* Creates a new identity, with a new random static address and random IRK. */
		id = bt_id_create(NULL, NULL);
		if (id < 0) {
			LOG_ERR("Create id failed (%d)", id);
			return id;
		}

		__ASSERT(id < CONFIG_BT_ID_MAX, "Identity %d exceeds max value", id);
		LOG_INF("New id: %d", id);
	}

	return 0;
}

static int setup_advertiser(uint8_t id_adv)
{
	int err;

	/* Initialize the parameters for each connecable advertiser. */
	struct bt_le_adv_param adv_param =
		BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONN,
				     MIN_ADV_INTERVAL,
				     MAX_ADV_INTERVAL,
				     NULL);

	LOG_INF("Using current id: %u", id_adv);
	adv_param.id = id_adv;
	advertisers[id_adv].id = id_adv;

	/* Create a new advertising set. */
	err = bt_le_ext_adv_create(&adv_param, NULL, &advertisers[id_adv].adv);
	if (err) {
		LOG_ERR("Failed to create advertiser set (err %d)", err);
		return err;
	}

	char device_name_buf[DEVICE_NAME_BUF_SIZE];
	/* Generate a new name to differentiate between advertisers. */
	snprintf(device_name_buf, sizeof(device_name_buf), "%s: %d", CONFIG_BT_DEVICE_NAME, id_adv);
	LOG_INF("Created %s: %p", device_name_buf, &advertisers[id_adv].adv);

	/* Set the advertising data. */
	struct bt_data adv_data[] = {
		BT_DATA(BT_DATA_NAME_COMPLETE, device_name_buf, strlen(device_name_buf)),
	};

	err = bt_le_ext_adv_set_data(advertisers[id_adv].adv, adv_data,
								 ARRAY_SIZE(adv_data), NULL, 0);
	if (err) {
		LOG_ERR("Failed to set advertising data (err %d)", err);
		return err;
	}

	return 0;
}

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}

	/* Register connection callbacks. */
	err = bt_conn_cb_register(&conn_callbacks);
	if (err) {
		LOG_ERR("Conn callback register failed (err %d)", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized");

	LOG_INF("Starting %d advertisers", CONFIG_BT_EXT_ADV_MAX_ADV_SET);
	for (uint8_t i = 0; i < CONFIG_BT_EXT_ADV_MAX_ADV_SET; i++) {
		err = create_id(i);
		if (err) {
			LOG_ERR("Create id failed check project configuration (err %d)", err);
			return 0;
		}

		err = setup_advertiser(i);
		if (err) {
			LOG_ERR("Setup Advertiser failed (err %d)", err);
			return 0;
		}

		/* Initialize and submit the work item for each advertiser */
		k_work_init(&advertisers[i].work, start_connectable_advertiser);
		k_work_submit(&advertisers[i].work);
	}
}
