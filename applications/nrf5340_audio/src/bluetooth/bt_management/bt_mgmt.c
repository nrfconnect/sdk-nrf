/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_mgmt.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <nrfx.h>

#include "macros_common.h"
#include "zbus_common.h"
#include "button_handler.h"
#include "bt_mgmt_ctlr_cfg_internal.h"
#include "bt_mgmt_adv_internal.h"
#include "bt_mgmt_dfu_internal.h"
#include "button_assignments.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mgmt, CONFIG_BT_MGMT_LOG_LEVEL);

ZBUS_CHAN_DEFINE(bt_mgmt_chan, struct bt_mgmt_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

/* The bt_enable should take less than 15 ms.
 * Buffer added as this will not add to bootup time
 */
#define BT_ENABLE_TIMEOUT_MS 100

K_SEM_DEFINE(sem_bt_enabled, 0, 1);

/**
 * @brief	Iterative function used to find connected conns
 *
 * @param[in]	conn	The connection to check
 * @param[out]	data	Pointer to store number of valid conns
 */
static void conn_state_connected_check(struct bt_conn *conn, void *data)
{
	int ret;
	uint8_t *num_conn = (uint8_t *)data;
	struct bt_conn_info info;

	ret = bt_conn_get_info(conn, &info);
	if (ret) {
		LOG_ERR("Failed to get conn info for %p: %d", (void *)conn, ret);
		return;
	}

	if (info.state != BT_CONN_STATE_CONNECTED) {
		return;
	}

	(*num_conn)++;
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	int ret;
	char addr[BT_ADDR_LE_STR_LEN] = {0};
	struct bt_mgmt_msg msg;

	if (err == BT_HCI_ERR_ADV_TIMEOUT && IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		LOG_INF("Directed adv timed out with no connection, reverting to normal adv");

		bt_mgmt_dir_adv_timed_out(0);
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		if (err == BT_HCI_ERR_UNKNOWN_CONN_ID) {
			LOG_WRN("ACL connection to addr: %s timed out, will try again", addr);
		} else {
			LOG_ERR("ACL connection to addr: %s, conn: %p, failed, error %d %s", addr,
				(void *)conn, err, bt_hci_err_to_str(err));
		}

		bt_conn_unref(conn);

		if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_CONN, NULL,
						 BRDCAST_ID_NOT_USED);
			if (ret && ret != -EALREADY) {
				LOG_ERR("Failed to restart scanning: %d", ret);
			}
		}

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			ret = bt_mgmt_adv_start(0, NULL, 0, NULL, 0, true);
			if (ret) {
				LOG_ERR("Failed to restart advertising: %d", ret);
			}
		}

		return;
	}

	/* ACL connection established */
	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Connected: %s", addr);

	msg.event = BT_MGMT_CONNECTED;
	msg.conn = conn;

	ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
	ERR_CHK(ret);

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		ret = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (ret) {
			LOG_ERR("Failed to set security to L2: %d", ret);
		}
	}
}

K_MUTEX_DEFINE(mtx_duplicate_scan);

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	int ret;
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_mgmt_msg msg;

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Disconnected: %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		bt_conn_unref(conn);
	}

	/* Publish disconnected */
	msg.event = BT_MGMT_DISCONNECTED;
	msg.conn = conn;

	ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
	ERR_CHK(ret);

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		ret = bt_mgmt_adv_start(0, NULL, 0, NULL, 0, true);
		ERR_CHK(ret);
	}

	/* The mutex for preventing the racing condition if two headset disconnected too close,
	 * cause the disconnected_cb() triggered in short time leads to duplicate scanning
	 * operation.
	 */
	k_mutex_lock(&mtx_duplicate_scan, K_FOREVER);
	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_CONN, NULL, BRDCAST_ID_NOT_USED);
		if (ret && ret != -EALREADY) {
			LOG_ERR("Failed to restart scanning: %d", ret);
		}
	}
	k_mutex_unlock(&mtx_duplicate_scan);
}

#if defined(CONFIG_BT_SMP)
static void security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	/* The address may not be resolved at this point */
	int ret;
	struct bt_mgmt_msg msg;

	if (err) {
		if (err == BT_SECURITY_ERR_UNSPECIFIED) {
			LOG_WRN("Security failed: level %d err %d Clear bond on peer?", level, err);
		} else {
			LOG_WRN("Security failed: level %d err %d %s", level, err,
				bt_security_err_to_str(err));
		}
		ret = bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
		if (ret == -ENOTCONN) {
			LOG_DBG("Not connected");
		} else if (ret) {
			LOG_WRN("Failed to disconnect %d", ret);
		}

	} else if (level < BT_SECURITY_L2) {
		LOG_WRN("Security changed: level %d too low, disconnecting", level);
		ret = bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
		if (ret == -ENOTCONN) {
			LOG_DBG("Not connected");
		} else if (ret) {
			LOG_ERR("Failed to disconnect %d", ret);
		}
	} else {
		const bt_addr_le_t *peer_addr = bt_conn_get_dst(conn);
		char peer_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(peer_addr, peer_str, BT_ADDR_LE_STR_LEN);

		LOG_INF("Security changed: level %d %s", level, peer_str);

		/* Publish connected */
		msg.event = BT_MGMT_SECURITY_CHANGED;
		msg.conn = conn;
		msg.addr = *peer_addr;

		ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
		ERR_CHK(ret);
	}
}

void identity_resolved_cb(struct bt_conn *conn, const bt_addr_le_t *rpa,
			  const bt_addr_le_t *identity)
{
	char rpa_str[BT_ADDR_LE_STR_LEN];
	char identity_str[BT_ADDR_LE_STR_LEN];
	(void)bt_addr_le_to_str(rpa, rpa_str, BT_ADDR_LE_STR_LEN);
	(void)bt_addr_le_to_str(identity, identity_str, BT_ADDR_LE_STR_LEN);
	LOG_INF("ID is resolved. RPA: %s, Identity: %s", rpa_str, identity_str);
};

#endif /* defined(CONFIG_BT_SMP) */

static struct bt_conn_cb conn_callbacks = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
#if defined(CONFIG_BT_SMP)
	.identity_resolved = identity_resolved_cb,
	.security_changed = security_changed_cb,
#endif /* defined(CONFIG_BT_SMP) */
};

void bond_deleted_cb(uint8_t id, const bt_addr_le_t *peer)
{
	int ret;
	char str[BT_ADDR_LE_STR_LEN];
	struct bt_mgmt_msg msg;

	(void)bt_addr_le_to_str(peer, str, BT_ADDR_LE_STR_LEN);
	LOG_INF("Bond deleted: id %d, peer %s", id, str);

	msg.event = BT_MGMT_BOND_DELETED;
	msg.addr = *peer;

	ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
	ERR_CHK(ret);
}

void pairing_complete_cb(struct bt_conn *conn, bool bonded)
{
	LOG_INF("Pairing complete. Bonded: %d", bonded);
	int ret;
	struct bt_mgmt_msg msg;
	const bt_addr_le_t *peer_addr = bt_conn_get_dst(conn);
	char str[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(peer_addr, str, BT_ADDR_LE_STR_LEN);

	msg.event = BT_MGMT_PAIRING_COMPLETE;
	msg.addr = *peer_addr;
	msg.conn = conn;

	ret = zbus_chan_pub(&bt_mgmt_chan, &msg, K_NO_WAIT);
	ERR_CHK(ret);
}

void pairing_failed_cb(struct bt_conn *conn, enum bt_security_err reason)
{
	int ret;

	LOG_WRN("Pairing failed: %d %s", reason, bt_security_err_to_str(reason));

	ret = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (ret == -ENOTCONN) {
		LOG_DBG("Not connected");
	} else if (ret) {
		LOG_ERR("Failed to disconnect %d", ret);
	}
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.bond_deleted = bond_deleted_cb,
	.pairing_complete = pairing_complete_cb,
	.pairing_failed = pairing_failed_cb,
};

static void bt_enabled_cb(int err)
{
	if (err) {
		LOG_ERR("Bluetooth init failed (err code: %d)", err);
		ERR_CHK(err);
	}

	k_sem_give(&sem_bt_enabled);

	LOG_DBG("Bluetooth initialized");
}

static int bonding_clear_check(void)
{
	int ret;
	bool pressed;

	ret = button_pressed(BUTTON_5, &pressed);
	if (ret) {
		return ret;
	}

	if (pressed) {
		ret = bt_mgmt_bonding_clear();
		return ret;
	}

	return 0;
}

/* This function generates a random address for bonding testing */
static int random_static_addr_set(void)
{
	int ret;
	static bt_addr_le_t addr;

	ret = bt_addr_le_create_static(&addr);
	if (ret < 0) {
		LOG_ERR("Failed to create address %d", ret);
		return ret;
	}

	ret = bt_id_create(&addr, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to create ID %d", ret);
		return ret;
	}

	return 0;
}

static int local_identity_addr_print(void)
{
	size_t num_ids = 0;
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_id_get(NULL, &num_ids);
	if (num_ids != CONFIG_BT_ID_MAX) {
		LOG_ERR("The default config supports %d ids, but %d was found", CONFIG_BT_ID_MAX,
			num_ids);
		return -ENOMEM;
	}

	bt_id_get(addrs, &num_ids);

	for (int i = 0; i < num_ids; i++) {
		(void)bt_addr_le_to_str(&(addrs[i]), addr_str, BT_ADDR_LE_STR_LEN);
		LOG_INF("Local identity addr: %s", addr_str);
	}

	return 0;
}

void bt_mgmt_num_conn_get(uint8_t *num_conn)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, conn_state_connected_check, (void *)num_conn);
}

int bt_mgmt_bonding_clear(void)
{
	int ret;

	/* TODO: Delay. Awaiting fix in NCSDK-35186 */
	k_sleep(K_MSEC(100));

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		LOG_INF("Clearing all bonds");

		ret = bt_unpair(BT_ID_DEFAULT, NULL);
		if (ret) {
			LOG_ERR("Failed to clear bonding: %d", ret);
			return ret;
		}
	}

	return 0;
}

int bt_mgmt_pa_sync_delete(struct bt_le_per_adv_sync *pa_sync)
{
	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC)) {
		int ret;

		ret = bt_le_per_adv_sync_delete(pa_sync);
		if (ret) {
			LOG_ERR("Failed to delete PA sync");
			return ret;
		}
	} else {
		LOG_WRN("Periodic advertisement sync not enabled");
		return -ENOTSUP;
	}

	return 0;
}

int bt_mgmt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	if (IS_ENABLED(CONFIG_BT_CONN)) {
		int ret;

		ret = bt_conn_disconnect(conn, reason);
		if (ret) {
			LOG_ERR("Failed to disconnect connection %p (%d)", (void *)conn, ret);
			return ret;
		}
	} else {
		LOG_WRN("BT conn not enabled");
		return -ENOTSUP;
	}

	return 0;
}

int bt_mgmt_init(void)
{
	int ret;

	ret = bt_enable(bt_enabled_cb);
	if (ret) {
		return ret;
	}

	ret = k_sem_take(&sem_bt_enabled, K_MSEC(BT_ENABLE_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("bt_enable timed out");
		return ret;
	}

	if (IS_ENABLED(CONFIG_TESTING_BLE_ADDRESS_RANDOM)) {
		ret = random_static_addr_set();
		if (ret) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		ret = settings_load();
		if (ret) {
			return ret;
		}

		ret = bonding_clear_check();
		if (ret) {
			return ret;
		}

		if (IS_ENABLED(CONFIG_TESTING_BLE_ADDRESS_RANDOM)) {
			ret = bt_mgmt_bonding_clear();
			if (ret) {
				return ret;
			}
		}
	}

#if defined(CONFIG_AUDIO_BT_MGMT_DFU)
	bool pressed;

	ret = button_pressed(BUTTON_4, &pressed);
	if (ret) {
		return ret;
	}

	if (pressed) {
		ret = bt_mgmt_ctlr_cfg_init(false);
		if (ret) {
			return ret;
		}
		/* This call will not return */
		bt_mgmt_dfu_start();
	}

#endif /* CONFIG_AUDIO_BT_MGMT_DFU */

#ifdef CONFIG_MCUMGR_TRANSPORT_BT_DYNAMIC_SVC_REGISTRATION
	/* Unregister SMP (Simple Management Protocol) service if DFU is not enabled */
	ret = smp_bt_unregister();
	if (ret) {
		LOG_ERR("Failed to unregister SMP service: %d", ret);
		return ret;
	}
#endif

	ret = bt_mgmt_ctlr_cfg_init(IS_ENABLED(CONFIG_WDT_CTLR));
	if (ret) {
		return ret;
	}

	ret = local_identity_addr_print();
	if (ret) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_BT_CONN)) {
		ret = bt_conn_cb_register(&conn_callbacks);
		if (ret) {
			LOG_ERR("Failed to register conn callbacks: %d", ret);
			return ret;
		}

		ret = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
		if (ret) {
			LOG_ERR("Failed to register conn auth info callbacks: %d", ret);
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) || IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		bt_mgmt_adv_init();
	}

	return 0;
}
