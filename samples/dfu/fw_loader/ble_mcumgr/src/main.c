/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>

#if defined(CONFIG_FW_LOADER_SETTINGS_ADV_NAME)
#include <dfu/fw_loader_settings.h>
#endif

LOG_MODULE_REGISTER(fw_loader_ble_mcumgr, CONFIG_LOG_DEFAULT_LEVEL);

static void advertising_work_handler(struct k_work *work);

static K_WORK_DEFINE(advertising_work, advertising_work_handler);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMP_BT_SVC_UUID_VAL),
};

#if defined(CONFIG_FW_LOADER_SETTINGS_ADV_NAME)
static char adv_name[CONFIG_FW_LOADER_SETTINGS_ADV_NAME_LEN];
static struct bt_data sd[] = {
	{
		.type = BT_DATA_NAME_COMPLETE,
		.data = adv_name,
	},
};

static void adv_name_prepare(void)
{
	int err;

	err = fw_loader_settings_adv_name_load(adv_name, sizeof(adv_name));
	if (err != 0) {
		LOG_WRN("Failed to load advertising name: %d", err);
		strncpy(adv_name, CONFIG_BT_DEVICE_NAME, sizeof(adv_name) - 1);
		adv_name[sizeof(adv_name) - 1] = '\0';
	}

	sd[0].data_len = strlen(adv_name);
}
#else
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};
#endif

static void advertising_work_handler(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

#if defined(CONFIG_FW_LOADER_SETTINGS_ADV_NAME)
	adv_name_prepare();
#endif

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err != 0) {
		LOG_ERR("Failed to start advertising: %d", err);
		return;
	}

	LOG_INF("Bluetooth advertising started");
}

static void advertising_start(void)
{
	int err;

	err = k_work_submit(&advertising_work);
	if (err < 0) {
		LOG_ERR("Failed to submit advertising work: %d", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	ARG_UNUSED(conn);

	if (conn_err) {
		LOG_WRN("Bluetooth connection failed, err 0x%02x %s", conn_err,
			bt_hci_err_to_str(conn_err));
		return;
	}

	LOG_INF("Bluetooth connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Bluetooth disconnected: 0x%02x %s", reason, bt_hci_err_to_str(reason));
}

static void recycled(void)
{
	advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Failed to enable Bluetooth: %d", err);
		return 0;
	}
	advertising_start();

	LOG_INF("BLE MCUmgr firmware loader started");

	return 0;
}
