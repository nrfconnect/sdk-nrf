/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_mgmt_scan_for_conn_internal.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

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

static void bond_check(const struct bt_bond_info *info, void *user_data)
{
	char addr_buf[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&info->addr, addr_buf, BT_ADDR_LE_STR_LEN);

	LOG_DBG("Stored bonding found: %s", addr_buf);
	bonded_num++;
}

static void bond_connect(const struct bt_bond_info *info, void *user_data)
{
	int ret;
	const bt_addr_le_t *adv_addr = user_data;
	struct bt_conn *conn;

	if (!bt_addr_le_cmp(&info->addr, adv_addr)) {
		LOG_DBG("Found bonded device");

		bt_le_scan_cb_unregister(&scan_callback);
		cb_registered = false;

		ret = bt_le_scan_stop();
		if (ret) {
			LOG_WRN("Stop scan failed: %d", ret);
		}

		ret = bt_conn_le_create(adv_addr, BT_CONN_LE_CREATE_CONN, CONNECTION_PARAMETERS,
					&conn);
		if (ret) {
			LOG_WRN("Create ACL connection failed: %d", ret);

			ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_CONN, NULL);
			if (ret) {
				LOG_ERR("Failed to restart scanning: %d", ret);
			}
		}
	}
}

/**
 * @brief	Check the advertising data for the matching device name.
 *
 * @param[in]	data		The advertising data to be checked.
 * @param[in]	user_data	The user data that contains the pointer to the address.
 *
 * @retval	false	Stop going through adv data.
 * @retval	true	Continue checking the data.
 */
static bool device_name_check(struct bt_data *data, void *user_data)
{
	int ret;
	bt_addr_le_t *addr = user_data;
	struct bt_conn *conn;

	/* We only care about LTVs with name */
	if (data->type == BT_DATA_NAME_COMPLETE) {
		size_t srch_name_size = strlen(srch_name);

		if ((data->data_len == srch_name_size) &&
		    (strncmp(srch_name, data->data, srch_name_size) == 0)) {
			LOG_DBG("Device found: %s", srch_name);

			bt_le_scan_cb_unregister(&scan_callback);
			cb_registered = false;

			ret = bt_le_scan_stop();
			if (ret) {
				LOG_ERR("Stop scan failed: %d", ret);
			}

			ret = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, CONNECTION_PARAMETERS,
						&conn);
			if (ret) {
				LOG_ERR("Could not init connection");

				ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_CONN, NULL);
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
			bt_data_parse(ad, device_name_check, (void *)info->addr);
		}
		break;
	default:
		break;
	}
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
		if (ret) {
			LOG_ERR("Failed to stop scan: %d", ret);
			return ret;
		}
	}

	/* Reset number of bonds found */
	bonded_num = 0;

	bt_foreach_bond(BT_ID_DEFAULT, bond_check, NULL);

	if (bonded_num >= CONFIG_BT_MAX_PAIRED) {
		LOG_INF("All bonded slots filled, will not accept new devices");
	}

	ret = bt_le_scan_start(scan_param, NULL);
	if (ret && ret != -EALREADY) {
		return ret;
	}

	return 0;
}
