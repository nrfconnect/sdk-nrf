/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_acl_common.h"
#include "ctrl_events.h"
#include "ble_trans.h"
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/iso.h>
#include "ble_trans.h"
#include "ble_audio_services.h"
#include "ble_acl_gateway.h"
#include "ble_hci_vsc.h"

#if (CONFIG_AUDIO_DEV == GATEWAY)
#include "ble_acl_gateway.h"
#elif (CONFIG_AUDIO_DEV == HEADSET)
#include "ble_acl_headset.h"
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */

#include "macros_common.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(ble, CONFIG_LOG_BLE_LEVEL);

/* Connection to the peer device - the other nRF5340 Audio device
 * This is the device we are streaming audio to/from.
 */
static struct bt_conn *_p_conn_peer;

#if (CONFIG_AUDIO_DEV == HEADSET)
K_WORK_DEFINE(adv_work, work_adv_start);
#elif (CONFIG_AUDIO_DEV == GATEWAY)
K_WORK_DEFINE(scan_work, work_scan_start);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */

/**@brief BLE connected handler.
 */
static void on_connected_cb(struct bt_conn *conn, uint8_t err)
{
	int ret;
	char addr[BT_ADDR_LE_STR_LEN];
	uint16_t peer_conn_handle;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	ARG_UNUSED(ret);
	if (err) {
		LOG_ERR("ACL connection to %s failed, error %d", addr, err);
	} else {
		/* ACL connection established */
		LOG_DBG("ACL connection to %s established", addr);
		ret = bt_hci_get_conn_handle(conn, &peer_conn_handle);
		if (ret) {
			LOG_ERR("Unable to get handle, ret = %d", ret);
		} else {
			ret = ble_hci_vsc_set_conn_tx_pwr(peer_conn_handle,
							  BLE_HCI_VSC_TX_PWR_Pos3dBm);
			ERR_CHK(ret);
		}
#if (CONFIG_AUDIO_DEV == HEADSET)
		ble_acl_headset_on_connected(conn);
#elif (CONFIG_AUDIO_DEV == GATEWAY)
		ble_acl_gateway_on_connected(conn);
#if (CONFIG_BT_SMP)
		ret = bt_conn_set_security(conn, BT_SECURITY_L2);
		ERR_CHK_MSG(ret, "Failed to set security");
#else
		if (!ble_acl_gateway_all_links_connected()) {
			k_work_submit(&scan_work);
		} else {
			LOG_INF("All ACL links are connected");
			bt_le_scan_stop();
		}
		ret = ble_trans_iso_cis_connect(conn);
		ERR_CHK_MSG(ret, "Failed to connect to ISO CIS channel");
#endif /* (CONFIG_BT_SMP) */
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
	}
}

/**@brief BLE disconnected handler.
 */
static void on_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_DBG("ACL disconnected with %s reason: %d", addr, reason);
#if (CONFIG_AUDIO_DEV == HEADSET)
	if (_p_conn_peer == conn) {
		bt_conn_unref(_p_conn_peer);
		_p_conn_peer = NULL;
	} else {
		LOG_WRN("Unknown peer disconnected");
	}
#elif (CONFIG_AUDIO_DEV == GATEWAY)
	struct bt_conn *conn_active;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		int ret;

		ret = ble_acl_gateway_conn_peer_get(i, &conn_active);
		ERR_CHK_MSG(ret, "Connection peer get error");
		if (conn_active == conn) {
			bt_conn_unref(conn_active);
			conn_active = NULL;
			ret = ble_acl_gateway_conn_peer_set(i, &conn_active);
			ERR_CHK_MSG(ret, "Connection peer set error");
			LOG_DBG("Headset %d disconnected", i);
			break;
		}
	}
	k_work_submit(&scan_work);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
}

/**@brief BLE connection parameter request handler.
 */
static bool conn_param_req_cb(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	/* Connection between two nRF5340 Audio DKs are fixed */
	if ((param->interval_min != CONFIG_BLE_ACL_CONN_INTERVAL) ||
	    (param->interval_max != CONFIG_BLE_ACL_CONN_INTERVAL)) {
		return false;
	}

	return true;
}

/**@brief BLE connection parameters updated handler.
 */
static void conn_param_updated_cb(struct bt_conn *conn, uint16_t interval, uint16_t latency,
				  uint16_t timeout)
{
	LOG_DBG("conn: %x04, int: %d", (int)conn, interval);
	LOG_DBG("lat: %d, to: %d", latency, timeout);

	if (interval <= CONFIG_BLE_ACL_CONN_INTERVAL) {
		/* Link could be ready, since there's no service discover now */
		LOG_DBG("Connection parameters updated");
	}
}

#if (CONFIG_BT_SMP)
static void security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	int ret;

	if (err) {
		LOG_ERR("Security failed: level %u err %d", level, err);
		ret = bt_conn_disconnect(conn, err);
		if (ret) {
			LOG_ERR("Failed to disconnect %d", ret);
		}
	} else {
		LOG_DBG("Security changed: level %u", level);
	}
#if (CONFIG_AUDIO_DEV == GATEWAY)

	ret = ble_acl_gateway_mtu_exchange(conn);
	if (ret) {
		LOG_WRN("MTU exchange procedure failed = %d", ret);
	}
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */
}
#endif /* (CONFIG_BT_SMP) */

/**@brief BLE connection callback handlers.
 */
static struct bt_conn_cb conn_callbacks = {
	.connected = on_connected_cb,
	.disconnected = on_disconnected_cb,
	.le_param_req = conn_param_req_cb,
	.le_param_updated = conn_param_updated_cb,
#if (CONFIG_BT_SMP)
	.security_changed = security_changed_cb,
#endif /* (CONFIG_BT_SMP) */
};

void ble_acl_common_conn_peer_set(struct bt_conn *conn)
{
	_p_conn_peer = conn;
}

void ble_acl_common_conn_peer_get(struct bt_conn **p_conn)
{
	*p_conn = _p_conn_peer;
}

void ble_acl_common_start(void)
{
#if (CONFIG_AUDIO_DEV == HEADSET)
	k_work_submit(&adv_work);
#elif (CONFIG_AUDIO_DEV == GATEWAY)
	k_work_submit(&scan_work);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
}

int ble_acl_common_init(void)
{
	int ret;

	ARG_UNUSED(ret);
	bt_conn_cb_register(&conn_callbacks);

/* TODO: Initialize BLE services here, SMP is required*/
#if (CONFIG_BT_VCS)
	ret = ble_vcs_server_init();
	if (ret) {
		return ret;
	}
#endif /* (CONFIG_BT_VCS) */
#if (CONFIG_BT_VCS_CLIENT)
	ret = ble_vcs_client_init();
	if (ret) {
		return ret;
	}
#endif /* (CONFIG_BT_VCS_CLIENT) */

	return 0;
}
