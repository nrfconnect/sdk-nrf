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
#include <zephyr/net/buf.h>

#include "wifi_provisioning.h"
#include "wifi_prov_internal.h"

LOG_MODULE_DECLARE(wifi_prov, CONFIG_WIFI_PROVISIONING_LOG_LEVEL);

/* WiFi Provisioning Service UUID */
#define BT_UUID_PROV_VAL \
	BT_UUID_128_ENCODE(0x14387800, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
#define BT_UUID_PROV \
	BT_UUID_DECLARE_128(BT_UUID_PROV_VAL)

/* Information characteristic UUID */
#define BT_UUID_PROV_INFO_VAL \
	BT_UUID_128_ENCODE(0x14387801, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
#define BT_UUID_PROV_INFO \
	BT_UUID_DECLARE_128(BT_UUID_PROV_INFO_VAL)

/* Control Point characteristic UUID */
#define BT_UUID_PROV_CONTROL_POINT_VAL \
	BT_UUID_128_ENCODE(0x14387802, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
#define BT_UUID_PROV_CONTROL_POINT \
	BT_UUID_DECLARE_128(BT_UUID_PROV_CONTROL_POINT_VAL)

/* Data out characteristic UUID */
#define BT_UUID_PROV_DATA_OUT_VAL \
	BT_UUID_128_ENCODE(0x14387803, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
#define BT_UUID_PROV_DATA_OUT \
	BT_UUID_DECLARE_128(BT_UUID_PROV_DATA_OUT_VAL)

#define PROV_SVC_CP_IDX               3
#define PROV_SVC_DATA_IDX             6

#define ADV_DATA_UPDATE_INTERVAL      5

#define ADV_DATA_VERSION_IDX          (BT_UUID_SIZE_128 + 0)
#define ADV_DATA_FLAG_IDX             (BT_UUID_SIZE_128 + 1)
#define ADV_DATA_FLAG_PROV_STATUS_BIT BIT(0)
#define ADV_DATA_FLAG_CONN_STATUS_BIT BIT(1)
#define ADV_DATA_RSSI_IDX             (BT_UUID_SIZE_128 + 3)

#define PROV_BT_LE_ADV_PARAM_FAST BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
						BT_GAP_ADV_FAST_INT_MIN_2, \
						BT_GAP_ADV_FAST_INT_MAX_2, NULL)

#define PROV_BT_LE_ADV_PARAM_SLOW BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
						BT_GAP_ADV_SLOW_INT_MIN, \
						BT_GAP_ADV_SLOW_INT_MAX, NULL)

static uint8_t device_name[] = {'P', 'V', '0', '0', '0', '0', '0', '0'};

static uint8_t prov_svc_data[] = {BT_UUID_PROV_VAL, 0x00, 0x00, 0x00, 0x00};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_PROV_VAL),
	BT_DATA(BT_DATA_NAME_COMPLETE, device_name, sizeof(device_name)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_SVC_DATA128, prov_svc_data, sizeof(prov_svc_data)),
};

static struct bt_conn *default_conn;
static struct k_work_delayable update_adv_data_work;
static struct k_work_delayable update_adv_param_work;

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

static void update_wifi_status_in_adv(void)
{
	int rc;
	struct net_if *iface = net_if_get_default();
	struct wifi_iface_status status = { 0 };

	prov_svc_data[ADV_DATA_VERSION_IDX] = PROV_SVC_VER;

	/* If no config, mark it as unprovisioned. */
	if (!wifi_has_config()) {
		prov_svc_data[ADV_DATA_FLAG_IDX] &= ~ADV_DATA_FLAG_PROV_STATUS_BIT;
	} else {
		prov_svc_data[ADV_DATA_FLAG_IDX] |= ADV_DATA_FLAG_PROV_STATUS_BIT;
	}

	rc = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
				sizeof(struct wifi_iface_status));
	/* If WiFi is not connected or error occurs, mark it as not connected. */
	if ((rc != 0) || (status.state < WIFI_STATE_ASSOCIATED)) {
		prov_svc_data[ADV_DATA_FLAG_IDX] &= ~ADV_DATA_FLAG_CONN_STATUS_BIT;
		prov_svc_data[ADV_DATA_RSSI_IDX] = INT8_MIN;
	} else {
		/* WiFi is connected. */
		prov_svc_data[ADV_DATA_FLAG_IDX] |= ADV_DATA_FLAG_CONN_STATUS_BIT;
		/* Currently cannot retrieve RSSI. Use a dummy number. */
		prov_svc_data[ADV_DATA_RSSI_IDX] = status.rssi;
	}

	rc = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (rc != 0) {
		LOG_DBG("Cannot update advertisement data, err = %d", rc);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_WRN("BT Connection failed (err 0x%02x).", err);
		return;
	}

	default_conn = conn;
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("BT Connected: %s", addr);
	k_work_cancel_delayable(&update_adv_data_work);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("BT Disconnected: %s (reason 0x%02x).", addr, reason);

	default_conn = NULL;
	k_work_reschedule(&update_adv_param_work, K_NO_WAIT);
	k_work_reschedule(&update_adv_data_work, K_SECONDS(ADV_DATA_UPDATE_INTERVAL));
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
				  const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	LOG_INF("BT Identity resolved %s -> %s.", addr_rpa, addr_identity);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
				 enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("BT Security changed: %s level %u.", addr, level);
	} else {
		LOG_WRN("BT Security failed: %s level %u err %d.", addr, level,
			   err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.identity_resolved = identity_resolved,
	.security_changed = security_changed,
};

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("BT Pairing cancelled: %s.", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("BT pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	LOG_INF("BT Pairing Failed (%d). Disconnecting.", reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static struct bt_conn_auth_info_cb auth_info_cb_display = {

	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

static void update_adv_data_task(struct k_work *item)
{
	update_wifi_status_in_adv();

	k_work_reschedule(&update_adv_data_work, K_SECONDS(ADV_DATA_UPDATE_INTERVAL));
}

static void update_adv_param_task(struct k_work *item)
{
	int rc;

	rc = bt_le_adv_stop();
	if (rc != 0) {
		LOG_INF("Cannot stop advertisement: err = %d", rc);
		return;
	}

	update_wifi_status_in_adv();

	rc = bt_le_adv_start(wifi_has_config() ?
		PROV_BT_LE_ADV_PARAM_SLOW : PROV_BT_LE_ADV_PARAM_FAST,
		ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (rc != 0) {
		LOG_INF("Cannot start advertisement: err = %d", rc);
	}
}

static void byte_to_hex(char *ptr, uint8_t byte, char base)
{
	int i, val;

	for (i = 0, val = (byte & 0xf0) >> 4; i < 2; i++, val = byte & 0x0f) {
		if (val < 10) {
			*ptr++ = (char) (val + '0');
		} else {
			*ptr++ = (char) (val - 10 + base);
		}
	}
}

static void update_dev_name(struct net_linkaddr *mac_addr)
{
	byte_to_hex(&device_name[2], mac_addr->addr[3], 'A');
	byte_to_hex(&device_name[4], mac_addr->addr[4], 'A');
	byte_to_hex(&device_name[6], mac_addr->addr[5], 'A');
}

int wifi_prov_transport_layer_init(void)
{
	int err;
	struct net_if *wifi_if = net_if_get_default();
	struct net_linkaddr *mac_addr = net_if_get_link_addr(wifi_if);
	char device_name_str[sizeof(device_name) + 1];

	bt_conn_auth_cb_register(&auth_cb_display);
	bt_conn_auth_info_cb_register(&auth_info_cb_display);

	err = bt_enable(NULL);
	if (err) {
		LOG_WRN("Bluetooth init failed (err %d).", err);
		return err;
	}

	LOG_INF("Bluetooth initialized.");

	/* Prepare advertisement data */
	if (mac_addr) {
		update_dev_name(mac_addr);
	}
	device_name_str[sizeof(device_name_str) - 1] = '\0';
	memcpy(device_name_str, device_name, sizeof(device_name));
	bt_set_name(device_name_str);

	update_wifi_status_in_adv();

	err = bt_le_adv_start(wifi_has_config() ?
		PROV_BT_LE_ADV_PARAM_SLOW : PROV_BT_LE_ADV_PARAM_FAST,
		ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_WRN("BT Advertising failed to start (err %d)\n", err);
		return err;
	}

	LOG_INF("BT Advertising successfully started.");

	k_work_init_delayable(&update_adv_data_work, update_adv_data_task);
	k_work_init_delayable(&update_adv_param_work, update_adv_param_task);

	k_work_schedule(&update_adv_data_work, K_SECONDS(ADV_DATA_UPDATE_INTERVAL));

	return 0;
}
