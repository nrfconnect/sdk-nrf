/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ble_acl_gateway.h"
#include "ble_acl_common.h"
#include "ble_audio_services.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>

#include "macros_common.h"
#include "ble_acl_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ble, CONFIG_LOG_BLE_LEVEL);

K_WORK_DEFINE(start_scan_work, work_scan_start);

#define BT_LE_CONN_PARAM_MULTI                                                                     \
	BT_LE_CONN_PARAM(CONFIG_BLE_ACL_CONN_INTERVAL, CONFIG_BLE_ACL_CONN_INTERVAL,               \
			 CONFIG_BLE_ACL_SLAVE_LATENCY, CONFIG_BLE_ACL_SUP_TIMEOUT)

static struct bt_gatt_exchange_params exchange_params;

/* Connection to the headset device - the other nRF5340 Audio device
 * This is the device we are streaming audio to/from.
 */
static struct bt_conn *gateway_conn_peer[CONFIG_BT_MAX_CONN];

/** @brief BLE data stream device found handler.
 *
 * This function is called as part of the processing of a device found
 * during scanning (i.e. as a result of on_device_found() being called).
 *
 * This is done by checking whether the device name is the peer
 * connection device name.
 *
 * If so, this function will stop scanning and try to create
 * a connection to the peer.
 */
static int device_found(uint8_t type, const uint8_t *data, uint8_t data_len,
			const bt_addr_le_t *addr)
{
	int ret;
	struct bt_conn *conn;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (ble_acl_gateway_all_links_connected()) {
		return 0;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	if ((data_len == DEVICE_NAME_PEER_L_LEN) &&
	    (strncmp(DEVICE_NAME_PEER_L, data, DEVICE_NAME_PEER_L_LEN) == 0)) {
		bt_le_scan_stop();

		ret = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_MULTI,
					&conn);
		if (ret) {
			LOG_ERR("Could not init connection");
			return ret;
		}

		ret = ble_acl_gateway_conn_peer_set(0, &conn);
		ERR_CHK_MSG(ret, "Connection peer set error");

		return 0;
	} else if ((data_len == DEVICE_NAME_PEER_R_LEN) &&
		   (strncmp(DEVICE_NAME_PEER_R, data, DEVICE_NAME_PEER_R_LEN) == 0)) {
		bt_le_scan_stop();

		ret = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_MULTI,
					&conn);
		if (ret) {
			LOG_ERR("Could not init connection");
			return ret;
		}

		ret = ble_acl_gateway_conn_peer_set(1, &conn);
		ERR_CHK_MSG(ret, "Connection peer set error");

		return 0;
	}

	return -ENOENT;
}

/** @brief  BLE parse advertisement package.
 */
static void ad_parse(struct net_buf_simple *p_ad, const bt_addr_le_t *addr)
{
	while (p_ad->len > 1) {
		uint8_t len = net_buf_simple_pull_u8(p_ad);
		uint8_t type;

		/* Check for early termination */
		if (len == 0) {
			return;
		}

		if (len > p_ad->len) {
			LOG_ERR("AD malformed");
			return;
		}

		type = net_buf_simple_pull_u8(p_ad);

		if (device_found(type, p_ad->data, len - 1, addr) == 0) {
			return;
		}

		(void)net_buf_simple_pull(p_ad, len - 1);
	}
}

/** @brief Handle devices found during scanning
 *
 * (Will only be called for peer connections, as we do not
 *  scan for control connections.)
 */
static void on_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			    struct net_buf_simple *p_ad)
{
	/* We're only interested in general connectable events */
	if (type == BT_HCI_ADV_IND) {
		/* Note: May lead to connection creation */
		ad_parse(p_ad, addr);
	}
}

static int ble_acl_gateway_service_discover(struct bt_conn *conn, uint8_t channel_num)
{
#if (CONFIG_BT_VCS_CLIENT)
	int ret;

	ret = ble_vcs_discover(conn, channel_num);
	ERR_CHK_MSG(ret, "VCS discover failed");
#endif /* (CONFIG_BT_VCS_CLIENT) */
	return 0;
}

/** @brief Callback handler for GATT exchange MTU procedure
 *
 *  This handler will be triggered after MTU exchange procedure finished.
 */
static void mtu_exchange_handler(struct bt_conn *conn, uint8_t err,
				 struct bt_gatt_exchange_params *params)
{
	int ret;
	struct bt_conn *conn_active;

	if (err) {
		LOG_ERR("MTU exchange failed, err = %d", err);
		ret = bt_conn_disconnect(conn, BT_HCI_ERR_LOCALHOST_TERM_CONN);
		if (ret) {
			LOG_ERR("Failed to disconnected, %d", ret);
		}
	} else {
		LOG_DBG("MTU exchange success");
		if (!ble_acl_gateway_all_links_connected()) {
			k_work_submit(&start_scan_work);
		} else {
			LOG_INF("All ACL links are connected");
			bt_le_scan_stop();
		}

		for (uint8_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
			ret = ble_acl_gateway_conn_peer_get(i, &conn_active);
			ERR_CHK_MSG(ret, "Connection peer get error");
			if (conn == conn_active) {
				ble_acl_gateway_service_discover(conn, i);
				LOG_INF("Service discover for device %d", i);
			}
		}

		ret = ble_trans_iso_cis_connect(conn);
		ERR_CHK_MSG(ret, "Failed to connect to ISO CIS channel");
	}
}

int ble_acl_gateway_conn_peer_get(uint8_t chan_number, struct bt_conn **p_conn)
{
	if (chan_number >= CONFIG_BT_MAX_CONN) {
		return -EINVAL;
	}
	*p_conn = gateway_conn_peer[chan_number];
	return 0;
}

int ble_acl_gateway_conn_peer_set(uint8_t chan_number, struct bt_conn **p_conn)
{
	if (chan_number >= CONFIG_BT_MAX_CONN) {
		return -EINVAL;
	}

	if (gateway_conn_peer[chan_number] != NULL) {
		if (*p_conn == NULL) {
			gateway_conn_peer[chan_number] = NULL;
		} else {
			LOG_WRN("Already have a connection for peer: %d", chan_number);
		}
		/* Ignore duplicates as several peripherals might be
		 * advertising at the same time
		 */
		return 0;
	}

	gateway_conn_peer[chan_number] = *p_conn;
	return 0;
}

bool ble_acl_gateway_all_links_connected(void)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (gateway_conn_peer[i] == NULL) {
			return false;
		}
	}
	return true;
}

void work_scan_start(struct k_work *item)
{
	int ret;

	ret = bt_le_scan_start(BT_LE_SCAN_PASSIVE, on_device_found);
	if (ret) {
		LOG_WRN("Scanning failed to start (ret %d)", ret);
		return;
	}

	LOG_DBG("Scanning successfully started");
}

void ble_acl_gateway_on_connected(struct bt_conn *conn)
{
	LOG_DBG("Connected - nRF5340 Audio gateway");
}

int ble_acl_gateway_mtu_exchange(struct bt_conn *conn)
{
	exchange_params.func = mtu_exchange_handler;

	return bt_gatt_exchange_mtu(conn, &exchange_params);
}
