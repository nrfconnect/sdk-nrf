/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <logging/log.h>
#include <settings/settings.h>

#include "ble_utils.h"

LOG_MODULE_REGISTER(ble_utils, CONFIG_BLE_UTILS_LOG_LEVEL);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey);
static void auth_cancel(struct bt_conn *conn);
static void pairing_confirm(struct bt_conn *conn);
static void pairing_complete(struct bt_conn *conn, bool bonded);
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason);
static void __attribute__((unused))
security_changed(struct bt_conn *conn, bt_security_t level,
		 enum bt_security_err err);

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	COND_CODE_1(CONFIG_BT_SMP, (.security_changed = security_changed), ())
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

static struct k_work on_connect_work;
static struct k_work on_disconnect_work;

static struct bt_conn *current_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	LOG_INF("Connected");
	current_conn = bt_conn_ref(conn);

	k_work_submit(&on_connect_work);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)", reason);

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;

		k_work_submit(&on_disconnect_work);
	}
}

static char *ble_addr(struct bt_conn *conn)
{
	static char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	return addr;
}

static void __attribute__((unused))
security_changed(struct bt_conn *conn, bt_security_t level,
		 enum bt_security_err err)
{
	char *addr = ble_addr(conn);

	if (!err) {
		LOG_INF("Security changed: %s level %u", log_strdup(addr),
			level);
	} else {
		LOG_INF("Security failed: %s level %u err %d", log_strdup(addr),
			level, err);
	}
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char *addr = ble_addr(conn);

	LOG_INF("Passkey for %s: %06u", log_strdup(addr), passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char *addr = ble_addr(conn);

	LOG_INF("Pairing cancelled: %s", log_strdup(addr));
}

static void pairing_confirm(struct bt_conn *conn)
{
	char *addr = ble_addr(conn);

	bt_conn_auth_pairing_confirm(conn);

	LOG_INF("Pairing confirmed: %s", log_strdup(addr));
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char *addr = ble_addr(conn);

	LOG_INF("Pairing completed: %s, bonded: %d", log_strdup(addr), bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char *addr = ble_addr(conn);

	LOG_INF("Pairing failed conn: %s, reason %d", log_strdup(addr), reason);
}

int ble_utils_init(struct bt_nus_cb *nus_clbs,
		   ble_connection_cb_t on_connect,
		   ble_disconnection_cb_t on_disconnect)
{
	int ret;

	k_work_init(&on_connect_work, on_connect);
	k_work_init(&on_disconnect_work, on_disconnect);

	bt_conn_cb_register(&conn_callbacks);

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		bt_conn_auth_cb_register(&conn_auth_callbacks);
	}

	ret = bt_enable(NULL);
	if (ret) {
		LOG_ERR("Bluetooth initialization failed (error: %d)", ret);
		goto end;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	ret = bt_nus_init(nus_clbs);
	if (ret) {
		LOG_ERR("Failed to initialize UART service (error: %d)", ret);
		goto end;
	}

	ret = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (ret) {
		LOG_ERR("Advertising failed to start (error: %d)", ret);
		goto end;
	}

end:
	return ret;
}
