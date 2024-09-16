/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_mgmt_scan_for_conn_internal.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/csip.h>

/* Will become public when https://github.com/zephyrproject-rtos/zephyr/pull/73445 is merged */
#include <../subsys/bluetooth/audio/csip_internal.h>

#include "bt_mgmt.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bt_mgmt_scan);

#define CONNECTION_PARAMETERS                                                                      \
	BT_LE_CONN_PARAM(CONFIG_BLE_ACL_CONN_INTERVAL, CONFIG_BLE_ACL_CONN_INTERVAL,               \
			 CONFIG_BLE_ACL_SLAVE_LATENCY, CONFIG_BLE_ACL_SUP_TIMEOUT)

static uint8_t bonded_num;
static struct bt_le_scan_cb scan_callback;
static bool cb_registered;
static char const *srch_name;
static uint8_t const *server_sirk;

static void bond_check(const struct bt_bond_info *info, void *user_data)
{
	char addr_buf[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&info->addr, addr_buf, BT_ADDR_LE_STR_LEN);

	LOG_DBG("Stored bonding found: %s", addr_buf);
	bonded_num++;
}

static void bond_connect(const struct bt_bond_info *bond_info, void *user_data)
{
	int ret;
	const bt_addr_le_t *adv_addr = user_data;
	struct bt_conn *conn;
	char addr_string[BT_ADDR_LE_STR_LEN];

	if (!bt_addr_le_cmp(&bond_info->addr, adv_addr)) {
		LOG_DBG("Found bonded device");

		/* Check if the device is still connected due to waiting for ACL timeout */
		struct bt_conn *bonded_conn =
			bt_conn_lookup_addr_le(BT_ID_DEFAULT, &bond_info->addr);
		struct bt_conn_info conn_info;

		if (bonded_conn != NULL) {
			ret = bt_conn_get_info(bonded_conn, &conn_info);
			if (ret == 0 && conn_info.state == BT_CONN_STATE_CONNECTED) {
				LOG_DBG("Trying to connect to an already connected conn");
				bt_conn_unref(bonded_conn);
				return;
			}

			/* Unref is needed due to bt_conn_lookup */
			bt_conn_unref(bonded_conn);
		}

		bt_le_scan_cb_unregister(&scan_callback);
		cb_registered = false;

		ret = bt_le_scan_stop();
		if (ret) {
			LOG_WRN("Stop scan failed: %d", ret);
		}

		bt_addr_le_to_str(adv_addr, addr_string, BT_ADDR_LE_STR_LEN);

		LOG_INF("Creating connection to bonded device: %s", addr_string);

		ret = bt_conn_le_create(adv_addr, BT_CONN_LE_CREATE_CONN, CONNECTION_PARAMETERS,
					&conn);
		if (ret) {
			LOG_WRN("Create ACL connection failed: %d", ret);

			ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_CONN, NULL,
						 BRDCAST_ID_NOT_USED);
			if (ret) {
				LOG_ERR("Failed to restart scanning: %d", ret);
			}
		}
	}
}

/**
 * @brief	Check if the address belongs to an already connected device.
 *
 * @param[in]	addr	Address to check.
 *
 * @retval	false	No device in connected state with that address.
 * @retval	true	Device found.
 */
static bool conn_exist_check(bt_addr_le_t *addr)
{
	int ret;
	struct bt_conn_info info;
	struct bt_conn *existing_conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);

	if (existing_conn != NULL) {
		ret = bt_conn_get_info(existing_conn, &info);
		if (ret == 0 && info.state == BT_CONN_STATE_CONNECTED) {
			LOG_DBG("Trying to connect to an already connected conn");
			bt_conn_unref(existing_conn);
			return true;
		} else if (ret) {
			LOG_WRN("Failed to get info from conn: %d", ret);
		}

		/* Unref is needed due to bt_conn_lookup */
		bt_conn_unref(existing_conn);
	}

	/* No existing connection found with that address */
	return false;
}

/**
 * @brief	Check the advertising data for the matching device name.
 *
 * @param[in]	data		The advertising data to be checked.
 * @param[in]	user_data	Pointer to the address.
 *
 * @retval	false	Stop going through adv data.
 * @retval	true	Continue checking the data.
 */
static bool device_name_check(struct bt_data *data, void *user_data)
{
	int ret;
	bt_addr_le_t *addr = user_data;
	struct bt_conn *conn;
	char addr_string[BT_ADDR_LE_STR_LEN];

	/* We only care about LTVs with name */
	if (data->type == BT_DATA_NAME_COMPLETE || data->type == BT_DATA_NAME_SHORTENED) {
		size_t srch_name_size = strlen(srch_name);
		if ((data->data_len == srch_name_size) &&
		    (strncmp(srch_name, data->data, srch_name_size) == 0)) {
			/* Check if the device is still connected due to waiting for ACL timeout */
			if (conn_exist_check(addr)) {
				/* Device is already connected, stop parsing the adv data */
				return false;
			}

			LOG_DBG("Device found: %s", srch_name);

			bt_le_scan_cb_unregister(&scan_callback);
			cb_registered = false;

			ret = bt_le_scan_stop();
			if (ret) {
				LOG_ERR("Stop scan failed: %d", ret);
			}

			bt_addr_le_to_str(addr, addr_string, BT_ADDR_LE_STR_LEN);

			LOG_INF("Creating connection to device: %s", addr_string);

			ret = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, CONNECTION_PARAMETERS,
						&conn);
			if (ret) {
				LOG_ERR("Could not init connection: %d", ret);

				ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_CONN, NULL,
							 BRDCAST_ID_NOT_USED);
				if (ret) {
					LOG_ERR("Failed to restart scanning: %d", ret);
				}
			}

			return false;
		}
	}

	return true;
}

/**
 * @brief	Check the advertising data for the matching 'Set Identity Resolving Key' (SIRK).
 *
 * @param[in]	data		The advertising data to be checked.
 * @param[in]	user_data	Pointer to the address.
 *
 * @retval	false	Stop going through adv data.
 * @retval	true	Continue checking the data.
 */
static bool csip_found(struct bt_data *data, void *user_data)
{
	int ret;
	bt_addr_le_t *addr = user_data;
	struct bt_conn *conn;
	char addr_string[BT_ADDR_LE_STR_LEN];

	if (!bt_csip_set_coordinator_is_set_member(server_sirk, data)) {
		/* This part of the data doesn't contain matching SIRK, continue parsing */
		return true;
	}

	/* Check if the device is still connected due to waiting for ACL timeout */
	if (conn_exist_check(addr)) {
		/* Device is already connected, stop parsing the adv data */
		return false;
	}

	LOG_DBG("Coordinated set device found");
	server_sirk = NULL;

	bt_le_scan_cb_unregister(&scan_callback);
	cb_registered = false;

	ret = bt_le_scan_stop();
	if (ret) {
		LOG_ERR("Stop scan failed: %d", ret);
	}

	bt_addr_le_to_str(addr, addr_string, BT_ADDR_LE_STR_LEN);

	LOG_INF("Creating connection to device in coordinated set: %s", addr_string);

	ret = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, CONNECTION_PARAMETERS, &conn);
	if (ret) {
		LOG_ERR("Could not init connection: %d", ret);

		ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_CONN, NULL, BRDCAST_ID_NOT_USED);
		if (ret) {
			LOG_ERR("Failed to restart scanning: %d", ret);
		}
	}

	/* Set member found and connected, stop parsing */
	return false;
}

/**
 * @brief	Callback handler for scan receive when scanning for connections.
 *
 * @param[in]	info	Advertiser packet and scan response information.
 * @param[in]	ad	Received advertising data.
 */
static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{

	/* We only care about connectable advertisers */
	if (!(info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE)) {
		return;
	}

	switch (info->adv_type) {
	case BT_GAP_ADV_TYPE_ADV_DIRECT_IND:
		/* Direct advertising has no payload, so no need to parse */
		bt_foreach_bond(BT_ID_DEFAULT, bond_connect, (void *)info->addr);
		break;
	case BT_GAP_ADV_TYPE_ADV_IND:
		/* Fall through */
	case BT_GAP_ADV_TYPE_EXT_ADV:
		/* Fall through */
	case BT_GAP_ADV_TYPE_SCAN_RSP:
		/* Note: May lead to connection creation */
		if (bonded_num < CONFIG_BT_MAX_PAIRED) {
			if (server_sirk == NULL) {
				bt_data_parse(ad, device_name_check, (void *)info->addr);
			} else {
				bt_data_parse(ad, csip_found, (void *)info->addr);
			}
		} else {
			/* All bonded slots are taken, so we will only
			 * accept previously bonded devices
			 */
			bt_foreach_bond(BT_ID_DEFAULT, bond_connect, (void *)info->addr);
		}
		break;
	default:
		break;
	}
}

static void conn_in_coord_set_check(struct bt_conn *conn, void *data)
{
	int ret;
	struct bt_conn_info info;
	const struct bt_csip_set_coordinator_set_member *member;

	if (data == NULL) {
		LOG_ERR("Got NULL pointer");
		return;
	}

	uint8_t *num_filled = (uint8_t *)data;

	ret = bt_conn_get_info(conn, &info);
	if (ret) {
		LOG_ERR("Failed to get conn info for %p: %d", (void *)conn, ret);
		return;
	}

	/* Don't care about connection not in a connected state */
	if (info.state != BT_CONN_STATE_CONNECTED) {
		return;
	}

	member = bt_csip_set_coordinator_set_member_by_conn(conn);

	if (memcmp((void *)server_sirk, (void *)member->insts[0].info.sirk, BT_CSIP_SIRK_SIZE) ==
	    0) {
		(*num_filled)++;
	}
}

void bt_mgmt_set_size_filled_get(uint8_t *num_filled)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, conn_in_coord_set_check, (void *)num_filled);
}

void bt_mgmt_scan_sirk_set(uint8_t const *const sirk)
{
	server_sirk = sirk;
}

int bt_mgmt_scan_for_conn_start(struct bt_le_scan_param *scan_param, char const *const name)
{
	int ret;

	srch_name = name;

	if (!cb_registered) {
		scan_callback.recv = scan_recv_cb;
		bt_le_scan_cb_register(&scan_callback);
		cb_registered = true;
	} else {
		/* Already scanning, stop current scan to update param in case it has changed */
		ret = bt_le_scan_stop();
		if (ret && ret != -EALREADY) {
			LOG_ERR("Failed to stop scan: %d", ret);
			return ret;
		}
	}

	/* Reset number of bonds found */
	bonded_num = 0;

	bt_foreach_bond(BT_ID_DEFAULT, bond_check, NULL);

	if (bonded_num >= CONFIG_BT_MAX_PAIRED) {
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_INF("All bonded slots filled, will not accept new devices");
	}

	ret = bt_le_scan_start(scan_param, NULL);
	if (ret && ret != -EALREADY) {
		return ret;
	}

	return 0;
}
