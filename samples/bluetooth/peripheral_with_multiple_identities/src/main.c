/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

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
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}

	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));
	printk("connected to %s\n", addr_str);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	printk("disconnected %s, reason %u\n", addr_str, reason);

	struct bt_conn_info connection_info;
	int err;

	err = bt_conn_get_info(conn, &connection_info);

	if (err) {
		printk("Failed to get conn info (err %d)\n", err);
		return;
	}

	/* Get the ID of the disconnected advertiser. */
	uint8_t id_current = connection_info.id;

	printk("Advertiser %d disconnected\n", id_current);

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
			printk("Failed to start advertising set (err %d)\n", err);
			return;
		}
	} else {
		printk("Advertiser not setup correctly\n");
		return;
	}

	printk("Advertiser %d successfully started\n", current_adv_info->id);
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
			printk("Create id failed (%d)\n", id);
			return id;
		}

		__ASSERT(id < CONFIG_BT_ID_MAX, "Identity %d exceeds max value", id);
		printk("New id: %d\n", id);
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

	printk("Using current id: %u\n", id_adv);
	adv_param.id = id_adv;
	advertisers[id_adv].id = id_adv;

	/* Create a new advertising set. */
	err = bt_le_ext_adv_create(&adv_param, NULL, &advertisers[id_adv].adv);
	if (err) {
		printk("Failed to create advertiser set (err %d)\n", err);
		return err;
	}

	char device_name_buf[DEVICE_NAME_BUF_SIZE];
	/* Generate a new name to differentiate between advertisers. */
	snprintf(device_name_buf, sizeof(device_name_buf), "%s: %d", CONFIG_BT_DEVICE_NAME, id_adv);
	printk("Created %s: %p\n", device_name_buf, &advertisers[id_adv].adv);

	/* Set the advertising data. */
	struct bt_data adv_data[] = {
		BT_DATA(BT_DATA_NAME_COMPLETE, device_name_buf, strlen(device_name_buf)),
	};

	err = bt_le_ext_adv_set_data(advertisers[id_adv].adv, adv_data,
								 ARRAY_SIZE(adv_data), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return err;
	}

	return 0;
}

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	/* Register connection callbacks. */
	err = bt_conn_cb_register(&conn_callbacks);
	if (err) {
		printk("Conn callback register failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	printk("Starting %d advertisers\n", CONFIG_BT_EXT_ADV_MAX_ADV_SET);
	for (uint8_t i = 0; i < CONFIG_BT_EXT_ADV_MAX_ADV_SET; i++) {
		err = create_id(i);
		if (err) {
			printk("Create id failed check project configuration (err %d)\n", err);
			return 0;
		}

		err = setup_advertiser(i);
		if (err) {
			printk("Setup Advertiser failed (err %d)\n", err);
			return 0;
		}

		/* Initialize and submit the work item for each advertiser */
		k_work_init(&advertisers[i].work, start_connectable_advertiser);
		k_work_submit(&advertisers[i].work);
	}
}
