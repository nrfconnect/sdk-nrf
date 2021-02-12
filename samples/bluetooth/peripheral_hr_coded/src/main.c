/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Peripheral Heart Rate over LE Coded PHY sample
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>
#include <bluetooth/services/hrs.h>

static struct k_work start_advertising_worker;

static struct bt_le_ext_adv *adv;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0d, 0x18, 0x0f, 0x18, 0x0a, 0x18),
};

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	struct bt_conn_info info;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Connection failed (err %d)\n", conn_err);
		return;
	}

	err = bt_conn_get_info(conn, &info);

	if (err) {
		printk("Failed to get connection info\n");
	} else {
		const struct bt_conn_le_phy_info *phy_info;
		phy_info = info.le.phy;

		printk("Connected: %s, tx_phy %u, rx_phy %u\n",
		       addr, phy_info->tx_phy, phy_info->rx_phy);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);

	k_work_submit(&start_advertising_worker);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static int create_advertising_coded(void)
{
	int err;
	struct bt_le_adv_param param =
		BT_LE_ADV_PARAM_INIT(BT_LE_ADV_OPT_CONNECTABLE |
				     BT_LE_ADV_OPT_EXT_ADV |
				     BT_LE_ADV_OPT_CODED,
				     BT_GAP_ADV_FAST_INT_MIN_2,
				     BT_GAP_ADV_FAST_INT_MAX_2,
				     NULL);

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	if (err) {
		printk("Failed to create advertiser set (%d)\n", err);
		return err;
	}

	printk("Created adv: %p\n", adv);

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (%d)\n", err);
		return err;
	}

	return 0;
}

static void start_advertising_coded(struct k_work *item)
{
	int err;

	err = bt_le_ext_adv_start(adv, NULL);
	if (err) {
		printk("Failed to start advertising set (%d)\n", err);
		return;
	}

	printk("Advertiser %p set started\n", adv);
}

static void bt_ready(void)
{
	int err = 0;

	printk("Bluetooth initialized\n");

	k_work_init(&start_advertising_worker, start_advertising_coded);

	err = create_advertising_coded();
	if (err) {
		printk("Advertising failed to create (err %d)\n", err);
		return;
	}

	k_work_submit(&start_advertising_worker);
}

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

static void hrs_notify(void)
{
	static uint8_t heartrate = 90U;

	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U) {
		heartrate = 90U;
	}

	bt_hrs_notify(heartrate);
}

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready();

	bt_conn_cb_register(&conn_callbacks);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
		k_sleep(K_SECONDS(1));

		/* Heartrate measurements simulation */
		hrs_notify();

		/* Battery level simulation */
		bas_notify();
	}
}
