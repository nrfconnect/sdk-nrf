/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** BIS receiver implementation for the time sync sample
 *
 * This file implements a device that syncs to a given BIS index.
 * When disconnected, it tries to sync to it again.
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <sdc_hci_vs.h>
#include <sdc_hci.h>
#include "iso_time_sync.h"

#define ADV_NAME_STR_MAX_LEN (sizeof(CONFIG_BT_DEVICE_NAME))

static K_SEM_DEFINE(sem_per_adv_info_valid, 0, 1);
static K_SEM_DEFINE(sem_big_info_valid, 0, 1);
static K_SEM_DEFINE(sem_per_adv_sync_lost, 0, 1);

static bool syncing_or_synced_to_periodic_adv;
static bool syncing_or_synced_to_big;

static bt_addr_le_t adv_addr;
static uint8_t adv_set_id;

static bool adv_data_parse_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, ADV_NAME_STR_MAX_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	char name_str[ADV_NAME_STR_MAX_LEN] = {0};

	bt_data_parse(buf, adv_data_parse_cb, name_str);

	if (strncmp(name_str, CONFIG_BT_DEVICE_NAME, ADV_NAME_STR_MAX_LEN) != 0) {
		return;
	}

	if (!syncing_or_synced_to_periodic_adv) {
		bt_addr_le_copy(&adv_addr, info->addr);
		adv_set_id = info->sid;

		syncing_or_synced_to_periodic_adv = true;
		k_sem_give(&sem_per_adv_info_valid);
	}
}

static void synced_to_per_adv_cb(struct bt_le_per_adv_sync *sync,
				 struct bt_le_per_adv_sync_synced_info *info)
{
	printk("Synced to periodic advertiser\n");
}

static void sync_to_per_adv_lost_cb(struct bt_le_per_adv_sync *sync,
				    const struct bt_le_per_adv_sync_term_info *info)
{
	printk("Sync to periodic advertiser lost\n");
	k_sem_give(&sem_per_adv_sync_lost);
}

static void biginfo_rcvd_cb(struct bt_le_per_adv_sync *sync, const struct bt_iso_biginfo *biginfo)
{
	if (!syncing_or_synced_to_big) {
		printk("BigInfo received\n");
		syncing_or_synced_to_big = true;
		k_sem_give(&sem_big_info_valid);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = synced_to_per_adv_cb,
	.term = sync_to_per_adv_lost_cb,
	.biginfo = biginfo_rcvd_cb,
};

static void reset_state(void)
{
	k_sem_reset(&sem_per_adv_info_valid);
	k_sem_reset(&sem_big_info_valid);
	k_sem_reset(&sem_per_adv_sync_lost);
	syncing_or_synced_to_periodic_adv = false;
	syncing_or_synced_to_big = false;
}

void bis_receiver_start(uint8_t bis_index_to_sync_to)
{
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_iso_big_sync_param big_sync_param;
	struct bt_le_per_adv_sync *sync;
	struct bt_iso_big *big;
	int err;

	uint8_t unused_rtn = 0;

	iso_rx_init(unused_rtn, NULL);

	bt_le_scan_cb_register(&scan_callbacks);
	bt_le_per_adv_sync_cb_register(&sync_callbacks);

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, NULL);
	if (err) {
		printk("Scan start failed (err %d)\n", err);
		return;
	}

	while (true) {
		reset_state();

		printk("Scanning for periodic advertiser\n");
		k_sem_take(&sem_per_adv_info_valid, K_FOREVER);

		syncing_or_synced_to_periodic_adv = true;

		memset(&sync_create_param, 0, sizeof(sync_create_param));
		bt_addr_le_copy(&sync_create_param.addr, &adv_addr);
		sync_create_param.options = 0;
		sync_create_param.sid = adv_set_id;
		sync_create_param.skip = 0;
		sync_create_param.timeout = 1000;

		err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
		if (err) {
			printk("Sync create failed (err %d)\n", err);
			return;
		}

		printk("Waiting for BigInfo\n");
		err = k_sem_take(&sem_big_info_valid, K_MSEC(5000));
		if (err) {
			printk("Failed obtaining BigInfo in time, starting over\n");

			err = bt_le_per_adv_sync_delete(sync);
			if (err) {
				printk("Deleting periodic sync failed (err %d)\n", err);
				return;
			}

			continue;
		}

		memset(&big_sync_param, 0, sizeof(big_sync_param));
		big_sync_param.bis_channels = iso_rx_channels_get();
		big_sync_param.num_bis = 1;
		big_sync_param.bis_bitfield = BIT(bis_index_to_sync_to - 1);
		big_sync_param.mse = BT_ISO_SYNC_MSE_ANY;
		big_sync_param.sync_timeout = 100; /* in 10 ms units */

		printk("Syncing to BIG index %d\n", bis_index_to_sync_to);
		err = bt_iso_big_sync(sync, &big_sync_param, &big);
		if (err) {
			printk("Creating BIG sync failed (err %d)\n", err);
			return;
		}

		k_sem_take(&sem_per_adv_sync_lost, K_FOREVER);

		err = bt_le_per_adv_sync_delete(sync);
		if (err) {
			printk("Deleting periodic sync failed (err %d)\n", err);
			return;
		}
	}
}
