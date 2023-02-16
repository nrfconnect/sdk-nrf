/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <net/wifi_credentials.h>

#include <bluetooth/services/wifi_provisioning.h>

#ifdef WIFI_PROV_ADV_DATA_UPDATE
#define ADV_DATA_UPDATE_INTERVAL      WIFI_PROV_ADV_DATA_UPDATE_INTERVAL
#endif /* WIFI_PROV_ADV_DATA_UPDATE */

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

#define ADV_DAEMON_STACK_SIZE 4096
#define ADV_DAEMON_PRIORITY 5

K_THREAD_STACK_DEFINE(adv_daemon_stack_area, ADV_DAEMON_STACK_SIZE);

static struct k_work_q adv_daemon_work_q;

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

static struct k_work_delayable update_adv_param_work;

#ifdef WIFI_PROV_ADV_DATA_UPDATE
static struct k_work_delayable update_adv_data_work;
#endif /* WIFI_PROV_ADV_DATA_UPDATE */

static void update_wifi_status_in_adv(void)
{
	int rc;
	struct net_if *iface = net_if_get_default();
	struct wifi_iface_status status = { 0 };

	prov_svc_data[ADV_DATA_VERSION_IDX] = PROV_SVC_VER;

	/* If no config, mark it as unprovisioned. */
	if (!bt_wifi_prov_state_get()) {
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
		printk("Cannot update advertisement data, err = %d\n", rc);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("BT Connection failed (err 0x%02x).\n", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("BT Connected: %s", addr);
#ifdef WIFI_PROV_ADV_DATA_UPDATE
	k_work_cancel_delayable(&update_adv_data_work);
#endif /* WIFI_PROV_ADV_DATA_UPDATE */
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("BT Disconnected: %s (reason 0x%02x).\n", addr, reason);

	k_work_reschedule_for_queue(&adv_daemon_work_q, &update_adv_param_work, K_NO_WAIT);
#ifdef WIFI_PROV_ADV_DATA_UPDATE
	k_work_reschedule_for_queue(&adv_daemon_work_q, &update_adv_data_work,
				K_SECONDS(ADV_DATA_UPDATE_INTERVAL));
#endif /* WIFI_PROV_ADV_DATA_UPDATE */
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
				const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	printk("BT Identity resolved %s -> %s.\n", addr_rpa, addr_identity);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
				enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("BT Security changed: %s level %u.\n", addr, level);
	} else {
		printk("BT Security failed: %s level %u err %d.\n", addr, level,
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

	printk("BT Pairing cancelled: %s.\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("BT pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	printk("BT Pairing Failed (%d). Disconnecting.\n", reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static struct bt_conn_auth_info_cb auth_info_cb_display = {

	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

#ifdef WIFI_PROV_ADV_DATA_UPDATE
static void update_adv_data_task(struct k_work *item)
{
	update_wifi_status_in_adv();

	k_work_reschedule_for_queue(&adv_daemon_work_q, &update_adv_data_work,
				K_SECONDS(ADV_DATA_UPDATE_INTERVAL));
}
#endif /* WIFI_PROV_ADV_DATA_UPDATE */

static void update_adv_param_task(struct k_work *item)
{
	int rc;

	rc = bt_le_adv_stop();
	if (rc != 0) {
		printk("Cannot stop advertisement: err = %d\n", rc);
		return;
	}

	update_wifi_status_in_adv();

	rc = bt_le_adv_start(prov_svc_data[ADV_DATA_FLAG_IDX] & ADV_DATA_FLAG_PROV_STATUS_BIT ?
		PROV_BT_LE_ADV_PARAM_SLOW : PROV_BT_LE_ADV_PARAM_FAST,
		ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (rc != 0) {
		printk("Cannot start advertisement: err = %d\n", rc);
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

static void get_wifi_credential(void *cb_arg, const char *ssid, size_t ssid_len)
{
	struct wifi_credentials_personal config;

	wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, &config);
	memcpy((struct wifi_credentials_personal *)cb_arg, &config, sizeof(config));
}

void main(void)
{
	int rc;
	struct wifi_credentials_personal config = { 0 };
	struct net_if *iface = net_if_get_default();
	struct wifi_connect_req_params cnx_params = { 0 };
	struct net_linkaddr *mac_addr = net_if_get_link_addr(iface);
	char device_name_str[sizeof(device_name) + 1];
	/* Sleep 1 seconds to allow initialization of wifi driver. */
	k_sleep(K_SECONDS(1));

	bt_conn_auth_cb_register(&auth_cb_display);
	bt_conn_auth_info_cb_register(&auth_info_cb_display);

	rc = bt_enable(NULL);
	if (rc) {
		printk("Bluetooth init failed (err %d).\n", rc);
		return;
	}

	printk("Bluetooth initialized.\n");

	rc = bt_wifi_prov_init();
	if (rc == 0) {
		printk("Wi-Fi provisioning service starts successfully.\n");
	} else {
		printk("Error occurs when initializing Wi-Fi provisioning service.\n");
		return;
	}

	/* Prepare advertisement data */
	if (mac_addr) {
		update_dev_name(mac_addr);
	}
	device_name_str[sizeof(device_name_str) - 1] = '\0';
	memcpy(device_name_str, device_name, sizeof(device_name));
	bt_set_name(device_name_str);

	rc = bt_le_adv_start(prov_svc_data[ADV_DATA_FLAG_IDX] & ADV_DATA_FLAG_PROV_STATUS_BIT ?
		PROV_BT_LE_ADV_PARAM_SLOW : PROV_BT_LE_ADV_PARAM_FAST,
		ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (rc) {
		printk("BT Advertising failed to start (err %d)\n", rc);
		return;
	}
	printk("BT Advertising successfully started.\n");

	update_wifi_status_in_adv();

	k_work_queue_init(&adv_daemon_work_q);
	k_work_queue_start(&adv_daemon_work_q, adv_daemon_stack_area,
			K_THREAD_STACK_SIZEOF(adv_daemon_stack_area), ADV_DAEMON_PRIORITY,
			NULL);

	k_work_init_delayable(&update_adv_param_work, update_adv_param_task);
#ifdef WIFI_PROV_ADV_DATA_UPDATE
	k_work_init_delayable(&update_adv_data_work, update_adv_data_task);
	k_work_schedule_for_queue(&adv_daemon_work_q, &update_adv_data_work,
				K_SECONDS(ADV_DATA_UPDATE_INTERVAL));
#endif /* WIFI_PROV_ADV_DATA_UPDATE */

	/* Search for stored wifi credential and apply */
	wifi_credentials_for_each_ssid(get_wifi_credential, &config);
	if (config.header.ssid_len > 0) {
		printk("Configuration found. Try to apply.\n");

		cnx_params.ssid = config.header.ssid;
		cnx_params.ssid_length = config.header.ssid_len;
		cnx_params.security = config.header.type;

		cnx_params.psk = NULL;
		cnx_params.psk_length = 0;
		cnx_params.sae_password = NULL;
		cnx_params.sae_password_length = 0;

		if (config.header.type != WIFI_SECURITY_TYPE_NONE) {
			cnx_params.psk = config.password;
			cnx_params.psk_length = config.password_len;
		}

		cnx_params.channel = WIFI_CHANNEL_ANY;
		cnx_params.band = config.header.flags & WIFI_CREDENTIALS_FLAG_5GHz ?
				WIFI_FREQ_BAND_5_GHZ : WIFI_FREQ_BAND_2_4_GHZ;
		cnx_params.mfp = WIFI_MFP_OPTIONAL;
		rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
			&cnx_params, sizeof(struct wifi_connect_req_params));
		if (rc < 0) {
			printk("Cannot apply saved Wi-Fi configuration, err = %d.\n", rc);
		} else {
			printk("Configuration applied.\n");
		}
	}
}
