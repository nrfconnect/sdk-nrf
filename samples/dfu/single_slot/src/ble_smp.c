/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/logging/log.h>

<<<<<<< HEAD
#if defined(CONFIG_FW_LOADER_SETTINGS_ADV_NAME)
#include <dfu/fw_loader_settings.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/retention/bootmode.h>
#include <zephyr/settings/settings.h>
#endif
=======
#include "ble_smp.h"
>>>>>>> samples: dfu: Move BLE MCUmgr firmware loader

LOG_MODULE_REGISTER(single_slot_ble_smp, LOG_LEVEL_INF);

static struct k_work adv_work;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMP_BT_SVC_UUID_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, (sizeof(CONFIG_BT_DEVICE_NAME) - 1)),
};

<<<<<<< HEAD
#if defined(CONFIG_FW_LOADER_SETTINGS_ADV_NAME)
static bool fw_loader_reset_pending;

static void disconnect_conn_before_reset(struct bt_conn *conn, void *data)
{
	ARG_UNUSED(data);

	(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void disconnect_before_reset(void)
{
	/* Attempt to terminate the BLE connection before reset so the central can faster
	 * reconnect to the device once the Firmware Loader is running, instead of
	 * having to wait for the connection to time out (supervision timeout).
	 */
	bt_conn_foreach(BT_CONN_TYPE_LE, disconnect_conn_before_reset, NULL);
}

static enum mgmt_cb_return fw_loader_reset_callback(uint32_t event, enum mgmt_cb_return prev_status,
						    int32_t *rc, uint16_t *group, bool *abort_more,
						    void *data, size_t data_size)
{
	ARG_UNUSED(prev_status);
	ARG_UNUSED(rc);
	ARG_UNUSED(group);
	ARG_UNUSED(abort_more);

	if (event == MGMT_EVT_OP_OS_MGMT_RESET) {
		struct os_mgmt_reset_data *reset_data = data;

		if (data_size != sizeof(*reset_data)) {
			return MGMT_CB_OK;
		}

		if (reset_data->boot_mode == BOOT_MODE_TYPE_BOOTLOADER) {
			int err;

			LOG_INF("FW loader reset requested (boot_mode=%u)", reset_data->boot_mode);

			err = settings_save();
			if (err != 0) {
				LOG_WRN("settings_save before fw loader reset failed: %d", err);
			} else {
				LOG_INF("settings_save completed before FW loader reset");
			}

			fw_loader_reset_pending = true;
		}

		return MGMT_CB_OK;
	}

	if (event == MGMT_EVT_OP_CMD_DONE) {
		const struct mgmt_evt_op_cmd_arg *cmd = data;

		if (data_size != sizeof(*cmd) || !fw_loader_reset_pending) {
			return MGMT_CB_OK;
		}

		if (cmd->group == MGMT_GROUP_ID_OS && cmd->id == OS_MGMT_ID_RESET &&
		    cmd->err == MGMT_ERR_EOK) {
			fw_loader_reset_pending = false;
			disconnect_before_reset();
		}

		return MGMT_CB_OK;
	}

	return MGMT_CB_OK;
}

static struct mgmt_callback reset_cb = {
	.callback = fw_loader_reset_callback,
	.event_id = MGMT_EVT_OP_OS_MGMT_RESET,
};

static struct mgmt_callback cmd_done_cb = {
	.callback = fw_loader_reset_callback,
	.event_id = MGMT_EVT_OP_CMD_DONE,
};

static int fw_loader_reset_hook_init(void)
{
	mgmt_callback_register(&reset_cb);
	mgmt_callback_register(&cmd_done_cb);

	return 0;
}

#endif /* CONFIG_FW_LOADER_SETTINGS_ADV_NAME */

=======
>>>>>>> samples: dfu: Move BLE MCUmgr firmware loader
static void adv_work_handler(struct k_work *work)
{
	int rc = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (rc) {
		LOG_ERR("Advertising failed to start: %d", rc);
		return;
	}

	LOG_INF("Advertising successfully started, device name: %s", CONFIG_BT_DEVICE_NAME);
}

static void advertising_start(void)
{
	k_work_submit(&adv_work);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed: 0x%02x (%s)", err, bt_hci_err_to_str(err));
		return;
	}

	LOG_INF("Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason: 0x%02x (%s)", reason, bt_hci_err_to_str(reason));
}

static void recycled_cb(void)
{
	advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected    = connected,
	.disconnected = disconnected,
	.recycled     = recycled_cb,
};

<<<<<<< HEAD
static int ble_smp_init(void)
{
	int rc;

#if defined(CONFIG_FW_LOADER_SETTINGS_ADV_NAME)
	fw_loader_reset_hook_init();
#endif
	rc = bt_enable(NULL);
=======
int ble_smp_init(void)
{
	int rc = bt_enable(NULL);

>>>>>>> samples: dfu: Move BLE MCUmgr firmware loader
	if (rc) {
		LOG_ERR("Bluetooth init failed: %d", rc);
		return rc;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	k_work_init(&adv_work, adv_work_handler);
	advertising_start();

	return 0;
}
<<<<<<< HEAD

SYS_INIT(ble_smp_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
=======
>>>>>>> samples: dfu: Move BLE MCUmgr firmware loader
