/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/zephyr.h>
#include <zephyr/init.h>

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

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(wifi_prov, CONFIG_WIFI_PROVISIONING_LOG_LEVEL);

/** @def BT_UUID_PROV_VAL
 *  @brief WiFi Provisioning Service value
 */
#define BT_UUID_PROV_VAL \
	BT_UUID_128_ENCODE(0x14387800, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
/** @def BT_UUID_PROV
 *  @brief WiFi Provisioning Service
 */
#define BT_UUID_PROV \
	BT_UUID_DECLARE_128(BT_UUID_PROV_VAL)
/** @def BT_UUID_PROV_INFO_VAL
 *  @brief WiFi Provisioning Info Characteristic value
 */
#define BT_UUID_PROV_INFO_VAL \
	BT_UUID_128_ENCODE(0x14387801, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
/** @def BT_UUID_PROV_INFO
 *  @brief WiFi Provisioning Info Characteristic
 */
#define BT_UUID_PROV_INFO \
	BT_UUID_DECLARE_128(BT_UUID_PROV_INFO_VAL)
/** @def BT_UUID_PROV_CONTROL_POINT_VAL
 *  @brief WiFi Provisioning Control Point Characteristic value
 */
#define BT_UUID_PROV_CONTROL_POINT_VAL \
	BT_UUID_128_ENCODE(0x14387802, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
/** @def BT_UUID_PROV_CONTROL_POINT
 *  @brief WiFi Provisioning Control Point Characteristic
 */
#define BT_UUID_PROV_CONTROL_POINT \
	BT_UUID_DECLARE_128(BT_UUID_PROV_CONTROL_POINT_VAL)
/** @def BT_UUID_PROV_DATA_OUT_VAL
 *  @brief WiFi Provisioning Data Out Characteristic value
 */
#define BT_UUID_PROV_DATA_OUT_VAL \
	BT_UUID_128_ENCODE(0x14387803, 0x130c, 0x49e7, 0xb877, 0x2881c89cb258)
/** @def BT_UUID_PROV_DATA_OUT
 *  @brief WiFi Provisioning Data Out Characteristic
 */
#define BT_UUID_PROV_DATA_OUT \
	BT_UUID_DECLARE_128(BT_UUID_PROV_DATA_OUT_VAL)

static struct bt_conn *default_conn;

static ssize_t read_version(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		void *buf, uint16_t len, uint16_t offset);

static ssize_t write_prov_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static void control_point_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

static void data_out_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

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

static void control_point_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool indic_enabled = (value == BT_GATT_CCC_INDICATE);

	LOG_INF("Indications %s", indic_enabled ? "enabled" : "disabled");
}

static void data_out_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("Notifications %s", notif_enabled ? "enabled" : "disabled");
}

static ssize_t read_version(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	default_conn = conn;

	NET_BUF_SIMPLE_DEFINE(info_buf, 10);

	wifi_prov_get_info(&info_buf);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, info_buf.data, info_buf.len);
}

static ssize_t write_prov_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	int rc;

	LOG_HEXDUMP_DBG(buf, len, "Control point rx: ");

	default_conn = conn;

	NET_BUF_SIMPLE_DEFINE(req_buf, 140);

	net_buf_simple_add_mem(&req_buf, buf, len);
	rc = wifi_prov_recv_req(&req_buf);

	return len;
}

int wifi_prov_send_rsp(struct net_buf_simple *rsp)
{
	int rc;
	struct bt_gatt_indicate_params params = { 0 };

	params.attr = &prov_svc.attrs[3];
	params.data = rsp->data;
	params.len = rsp->len;

	rc = bt_gatt_indicate(default_conn, &params);
	if (rc != 0) {
		LOG_ERR("Indication error: %d.", rc);
	}

	return rc;
}

int wifi_prov_send_result(struct net_buf_simple *result)
{
	return bt_gatt_notify(default_conn, &prov_svc.attrs[6],
		result->data, result->len);
}

#define MY_BT_LE_ADV_PARAM_FAST BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
						BT_GAP_ADV_FAST_INT_MIN_2, \
						BT_GAP_ADV_FAST_INT_MAX_2, NULL)

#define MY_BT_LE_ADV_PARAM_SLOW BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, \
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

static void update_wifi_status_in_adv(void)
{
	prov_svc_data[16] = PROV_SVC_VER;

	int has_config;

	has_config = wifi_has_config();
	/** If no config or error occurs, mark it as unprovisioned. */
	if (has_config < 0 || has_config == 0) {
		prov_svc_data[17] &= ~(1UL << 0);
	} else {
		prov_svc_data[17] |= 1UL << 0;
	}



	int rc;
	struct net_if *iface = net_if_get_default();
	struct wifi_iface_status status = { 0 };

	rc = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
				sizeof(struct wifi_iface_status));
	/** If WiFi is not connected or error occurs, mark it as not connected. */
	if (rc != 0 || status.state < WIFI_STATE_ASSOCIATED) {
		prov_svc_data[17] &= ~(1UL << 1);
		prov_svc_data[19] = INT8_MIN;
	} else {
		/** WiFi is connected. */
		prov_svc_data[17] |= 1UL << 1;
		/* Currently cannot retrieve RSSI. Use a dummy number. */
		prov_svc_data[19] = INT8_MAX;
	}


	bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_WRN("Connection failed (err 0x%02x).", err);
	} else {
		LOG_INF("Connected.");
		default_conn = conn;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02x).", reason);
	bt_unpair(BT_ID_DEFAULT, NULL);
	default_conn = NULL;
	bt_le_adv_stop();

	update_wifi_status_in_adv();

	if (wifi_prov_is_provsioned() == true) {
		bt_le_adv_start(MY_BT_LE_ADV_PARAM_SLOW, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	} else {
		bt_le_adv_start(MY_BT_LE_ADV_PARAM_FAST, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	}

}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
				  const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	LOG_INF("Identity resolved %s -> %s.", addr_rpa, addr_identity);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
				 enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u.", addr, level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d.", addr, level,
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

	LOG_INF("Pairing cancelled: %s.", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_INF("Pairing Complete.");
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	LOG_INF("Pairing Failed (%d). Disconnecting.", reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static struct bt_conn_auth_info_cb auth_info_cb_display = {

	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

struct k_work_delayable update_adv_work;

void update_adv_task(struct k_work *item)
{
	update_wifi_status_in_adv();

	k_work_reschedule(&update_adv_work, K_SECONDS(1));
}

int wifi_prov_transport_layer_init(void)
{
	default_conn = NULL;

	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_WRN("Bluetooth init failed (err %d).", err);
		return err;
	}

	LOG_INF("Bluetooth initialized.");

	/* Prepare advertisement data */
	struct net_if *wifi_if = net_if_get_default();
	struct net_linkaddr *mac_addr = net_if_get_link_addr(wifi_if);

	/** ASCII Table
	 * 48 - '0'
	 * 59 - '9'
	 * 65 - 'A'
	 * 70 - 'F'
	 */
	device_name[2] = mac_addr->addr[3] / 16 > 9 ?
			((mac_addr->addr[3] / 16) + 55) : ((mac_addr->addr[3] / 16) + 48);
	device_name[3] = mac_addr->addr[3] % 16 > 9 ?
			((mac_addr->addr[3] % 16) + 55) : ((mac_addr->addr[3] % 16) + 48);
	device_name[4] = mac_addr->addr[4] / 16 > 9 ?
			((mac_addr->addr[4] / 16) + 55) : ((mac_addr->addr[4] / 16) + 48);
	device_name[5] = mac_addr->addr[4] % 16 > 9 ?
			((mac_addr->addr[4] % 16) + 55) : ((mac_addr->addr[4] % 16) + 48);
	device_name[6] = mac_addr->addr[5] / 16 > 9 ?
			((mac_addr->addr[5] / 16) + 55) : ((mac_addr->addr[5] / 16) + 48);
	device_name[7] = mac_addr->addr[5] % 16 > 9 ?
			((mac_addr->addr[5] % 16) + 55) : ((mac_addr->addr[5] % 16) + 48);

	update_wifi_status_in_adv();

	if (wifi_prov_is_provsioned() == true) {
		err = bt_le_adv_start(MY_BT_LE_ADV_PARAM_SLOW,
			ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	} else {
		err = bt_le_adv_start(MY_BT_LE_ADV_PARAM_FAST,
			ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	}
	if (err) {
		LOG_WRN("Advertising failed to start (err %d)\n", err);
		return err;
	}

	LOG_INF("Advertising successfully started.");

	bt_conn_auth_cb_register(&auth_cb_display);
	bt_conn_auth_info_cb_register(&auth_info_cb_display);

	k_work_init_delayable(&update_adv_work, update_adv_task);
	k_work_reschedule(&update_adv_work, K_SECONDS(1));

	return 0;
}
