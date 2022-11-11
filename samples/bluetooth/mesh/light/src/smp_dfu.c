/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>

#include <img_mgmt/img_mgmt.h>
#include <os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/smp_bt.h>

#include <zephyr/bluetooth/bluetooth.h>

static struct bt_le_ext_adv *adv;
static struct bt_le_ext_adv_start_param ext_adv_param = { .num_events = 0, .timeout = 1000 };
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, 0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86, 0xd3,
		      0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
};

void sent_cb(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info)
{
	int err;

	err = bt_le_ext_adv_start(adv, &ext_adv_param);

	if (err) {
		printk("Advertising SMP service failed (err %d)\n", err);
		return;
	}

	printk("Advertising SMP service\n");
}

struct bt_le_ext_adv_cb adv_callbacks = {
	.sent = sent_cb,
};

void smp_service_adv_init(void)
{
	int err;

	err = bt_le_ext_adv_create(BT_LE_ADV_CONN_NAME, &adv_callbacks, &adv);
	if (err) {
		printk("Creating SMP service adv instance failed (err %d)\n", err);
	}

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Setting SMP service adv data failed (err %d)\n", err);
	}

	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		printk("Starting advertising of SMP service failed (err %d)\n", err);
	}
}

void smp_dfu_init(void)
{
	int err;

	img_mgmt_register_group();
	os_mgmt_register_group();

	err = smp_bt_register();

	if (err) {
		printk("SMP BT service registration failed (err %d)\n", err);
		return;
	}

	/**
	 * Since Bluetooth mesh utilizes the advertiser as the main channel of
	 * communication, a secondary advertising set is necessary to broadcast
	 * the SMP service.
	 */
	smp_service_adv_init();
}
