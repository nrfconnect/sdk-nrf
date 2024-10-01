/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net_buf.h>

#include <net/wifi_credentials.h>
#include <bluetooth/services/wifi_provisioning.h>

#include "wifi_prov_internal.h"

LOG_MODULE_DECLARE(wifi_prov, CONFIG_BT_WIFI_PROV_LOG_LEVEL);

#define PROV_SVC_CP_IDX               3
#define PROV_SVC_DATA_IDX             6

static struct bt_conn *default_conn;

static ssize_t read_version(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	int rc;

	NET_BUF_SIMPLE_DEFINE(info_buf, INFO_MSG_MAX_LENGTH);

	rc = wifi_prov_get_info(&info_buf);
	if (rc < 0) {
		LOG_WRN("Wi-Fi Provisioning service - info: cannot get info");
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, info_buf.data, info_buf.len);
}

static ssize_t write_prov_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	int rc;
	struct net_buf_simple req_buf;

	default_conn = conn;
	LOG_HEXDUMP_DBG(buf, len, "Control point rx: ");

	net_buf_simple_init_with_data(&req_buf, (void *)buf, len);
	rc = wifi_prov_recv_req(&req_buf);
	if (rc < 0) {
		LOG_WRN("Wi-Fi Provisioning service - control point: Cannot write request");
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}
	return len;
}

static void control_point_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool indic_enabled = (value == BT_GATT_CCC_INDICATE);

	LOG_INF("Wi-Fi Provisioning service - control point: indications %s",
		indic_enabled ? "enabled" : "disabled");
}

static void data_out_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("Wi-Fi Provisioning service - data out: notifications %s",
		notif_enabled ? "enabled" : "disabled");
}

/* Provision Service Declaration */
BT_GATT_SERVICE_DEFINE(prov_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_PROV),
	BT_GATT_CHARACTERISTIC(BT_UUID_PROV_INFO,
				BT_GATT_CHRC_READ,
				BT_GATT_PERM_READ,
				read_version, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_PROV_CONTROL_POINT,
				BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
				BT_GATT_PERM_WRITE_ENCRYPT,
				NULL, write_prov_control_point, NULL),
	BT_GATT_CCC(control_point_ccc_cfg_changed,
				BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_PROV_DATA_OUT,
				BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_NONE,
				NULL, NULL, NULL),
	BT_GATT_CCC(data_out_ccc_cfg_changed,
				BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
);

int wifi_prov_send_rsp(struct net_buf_simple *rsp)
{
	int rc;
	static struct bt_gatt_indicate_params params;

	params.attr = &prov_svc.attrs[PROV_SVC_CP_IDX];
	params.data = rsp->data;
	params.len = rsp->len;

	if (!default_conn) {
		LOG_INF("BT not connected. Ignore indication request.");
		return 0;
	}

	if (bt_gatt_is_subscribed(default_conn, &prov_svc.attrs[PROV_SVC_CP_IDX],
		BT_GATT_CCC_INDICATE)) {
		rc = bt_gatt_indicate(default_conn, &params);
		if (rc != 0) {
			LOG_ERR("Indication error: %d.", rc);
		}
	} else {
		LOG_INF("Wi-Fi Provisioning service - control point: cannot indicate");
		return -EIO;
	}
	return rc;
}

int wifi_prov_send_result(struct net_buf_simple *result)
{
	if (!default_conn) {
		LOG_INF("BT not connected. Ignore notification request.");
		return 0;
	}
	if (bt_gatt_is_subscribed(default_conn, &prov_svc.attrs[PROV_SVC_DATA_IDX],
		BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify(default_conn, &prov_svc.attrs[PROV_SVC_DATA_IDX],
			result->data, result->len);
	} else {
		LOG_INF("Wi-Fi Provisioning service - data out: cannot notify");
		return -EIO;
	}
}
