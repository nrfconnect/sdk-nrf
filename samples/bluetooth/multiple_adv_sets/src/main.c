/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/settings/settings.h>

#include <dk_buttons_and_leds.h>

#define NON_CONNECTABLE_ADV_IDX 0
#define CONNECTABLE_ADV_IDX     1

#define RUN_STATUS_LED          DK_LED1
#define CON_STATUS_LED          DK_LED2
#define RUN_LED_BLINK_INTERVAL  1000

#define NON_CONNECTABLE_DEVICE_NAME "Nordic Beacon"

static void advertising_work_handle(struct k_work *work);

static K_WORK_DEFINE(advertising_work, advertising_work_handle);

static struct bt_le_ext_adv *ext_adv[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
static const struct bt_le_adv_param *non_connectable_adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_SCANNABLE,
			0x140, /* 200 ms */
			0x190, /* 250 ms */
			NULL);

static const struct bt_le_adv_param *connectable_adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN,
			BT_GAP_ADV_FAST_INT_MIN_2, /* 100 ms */
			BT_GAP_ADV_FAST_INT_MAX_2, /* 150 ms */
			NULL);

static const struct bt_data non_connectable_ad_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_URI, /* The URI of the https://www.nordicsemi.com website */
		      0x17, /* UTF-8 code point for “https:” */
		      '/', '/', 'w', 'w', 'w', '.',
		      'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.',
		      'c', 'o', 'm'),
};

static const struct bt_data non_connectable_sd_data[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, NON_CONNECTABLE_DEVICE_NAME,
		sizeof(NON_CONNECTABLE_DEVICE_NAME) - 1),
};

static const struct bt_data connectable_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void adv_connected_cb(struct bt_le_ext_adv *adv,
			     struct bt_le_ext_adv_connected_info *info)
{
	printk("Advertiser[%d] %p connected conn %p\n", bt_le_ext_adv_get_index(adv),
		adv, info->conn);
}

static const struct bt_le_ext_adv_cb adv_cb = {
	.connected = adv_connected_cb
};

static void connectable_adv_start(void)
{
	int err;

	err = bt_le_ext_adv_start(ext_adv[CONNECTABLE_ADV_IDX], BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start connectable advertising (err %d)\n", err);
	}
}

static void advertising_work_handle(struct k_work *work)
{
	connectable_adv_start();
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	dk_set_led_on(CON_STATUS_LED);

	printk("Connected %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	dk_set_led_off(CON_STATUS_LED);

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	/* Process the disconnect logic in the workqueue so that
	 * the BLE stack is finished with the connection bookkeeping
	 * logic and advertising is possible.
	 */
	k_work_submit(&advertising_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static int advertising_set_create(struct bt_le_ext_adv **adv,
				  const struct bt_le_adv_param *param,
				  const struct bt_data *ad, size_t ad_len,
				  const struct bt_data *sd, size_t sd_len)
{
	int err;
	struct bt_le_ext_adv *adv_set;

	err = bt_le_ext_adv_create(param, &adv_cb,
				   adv);
	if (err) {
		return err;
	}

	adv_set = *adv;

	printk("Created adv: %p\n", adv_set);

	err = bt_le_ext_adv_set_data(adv_set, ad, ad_len, sd, sd_len);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return err;
	}

	return bt_le_ext_adv_start(adv_set, BT_LE_EXT_ADV_START_DEFAULT);
}

static int non_connectable_adv_create(void)
{
	int err;

	err = advertising_set_create(&ext_adv[NON_CONNECTABLE_ADV_IDX], non_connectable_adv_param,
				     non_connectable_ad_data, ARRAY_SIZE(non_connectable_ad_data),
				     non_connectable_sd_data, ARRAY_SIZE(non_connectable_sd_data));
	if (err) {
		printk("Failed to create a non-connectable advertising set (err %d)\n", err);
	}

	return err;
}

static int connectable_adv_create(void)
{
	int err;

	err = advertising_set_create(&ext_adv[CONNECTABLE_ADV_IDX], connectable_adv_param,
				     connectable_data, ARRAY_SIZE(connectable_data),
				     NULL, 0);
	if (err) {
		printk("Failed to create a connectable advertising set (err %d)\n", err);
	}

	return err;
}

int main(void)
{
	int err;
	int blink_status = 0;

	printk("Starting Bluetooth multiple advertising sets sample\n");

	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	err = non_connectable_adv_create();
	if (err) {
		return 0;
	}

	printk("Non-connectable advertising started\n");

	err = connectable_adv_create();
	if (err) {
		return 0;
	}

	printk("Connectable advertising started\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
