/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include "fp_auth.h"
#include "fp_keys.h"

#define EMPTY_PASSKEY	0xffffffff
#define USED_PASSKEY	0xfffffffe

static uint32_t bt_auth_peer_passkeys[CONFIG_BT_MAX_CONN];

#if (CONFIG_BT_MAX_CONN <= 8)
static uint8_t handled_conn_bm;
#elif (CONFIG_BT_MAX_CONN <= 16)
static uint16_t handled_conn_bm;
#elif (CONFIG_BT_MAX_CONN <= 32)
static uint32_t handled_conn_bm;
#else
#error "CONFIG_BT_MAX_CONN must be <= 32"
#endif


static bool is_conn_handled(const struct bt_conn *conn)
{
	return ((handled_conn_bm & BIT(bt_conn_index(conn))) != 0);
}

static void update_conn_handling(const struct bt_conn *conn, bool handled)
{
	size_t conn_idx = bt_conn_index(conn);

	WRITE_BIT(handled_conn_bm, conn_idx, handled);
}

static int send_auth_confirm(struct bt_conn *conn)
{
	int err = bt_conn_auth_passkey_confirm(conn);

	if (err) {
		LOG_ERR("Failed to confirm passkey (err: %d)", err);
	} else {
		LOG_DBG("Confirmed passkey");
	}

	return err;
}

static int send_auth_cancel(struct bt_conn *conn)
{
	int err = bt_conn_auth_cancel(conn);

	if (err) {
		LOG_ERR("Failed to cancel authentication (err: %d)", err);
	} else {
		LOG_WRN("Authentication cancelled");
	}

	return err;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	update_conn_handling(conn, false);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

enum bt_security_err auth_pairing_accept(struct bt_conn *conn,
					 const struct bt_conn_pairing_feat *const feat)
{
	if (!is_conn_handled(conn)) {
		LOG_ERR("Invalid conn (pairing accept): %p", (void *)conn);
		return BT_SECURITY_ERR_UNSPECIFIED;
	}

	if (feat->io_capability == BT_IO_NO_INPUT_OUTPUT) {
		LOG_ERR("Invalid IO capability (pairing accept): %p", (void *)conn);
		return BT_SECURITY_ERR_AUTH_REQUIREMENT;
	}

	fp_keys_bt_auth_progress(conn, false);

	return BT_SECURITY_ERR_SUCCESS;
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	ARG_UNUSED(passkey);

	if (is_conn_handled(conn)) {
		LOG_WRN("Unexpected passkey display conn: %p", (void *)conn);
	} else {
		LOG_ERR("Invalid conn (passkey display): %p", (void *)conn);
	}

	(void)send_auth_cancel(conn);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	size_t conn_idx = bt_conn_index(conn);
	bool auth_reject = false;

	if (is_conn_handled(conn)) {
		if (bt_auth_peer_passkeys[conn_idx] == EMPTY_PASSKEY) {
			bt_auth_peer_passkeys[conn_idx] = passkey;
			fp_keys_bt_auth_progress(conn, false);
		} else {
			LOG_ERR("Unexpected passkey confirmation");
			auth_reject = true;
			bt_auth_peer_passkeys[conn_idx] = USED_PASSKEY;
		}
	} else {
		LOG_ERR("Invalid conn (passkey confirm): %p", (void *)conn);
		auth_reject = true;
	}

	if (auth_reject) {
		(void)send_auth_cancel(conn);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	if (!is_conn_handled(conn)) {
		LOG_ERR("Invalid conn (authentication cancel): %p", (void *)conn);
		return;
	}

	LOG_WRN("Authentication cancel");
	update_conn_handling(conn, false);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	if (!is_conn_handled(conn)) {
		return;
	}

	if (!bonded) {
		LOG_ERR("Bonding failed");
	} else {
		LOG_DBG("Bonding complete");
	}

	if (bt_conn_get_security(conn) >= BT_SECURITY_L4) {
		fp_keys_bt_auth_progress(conn, true);
	} else {
		LOG_ERR("Invalid conn security level after pairing: %p", (void *)conn);
	}

	update_conn_handling(conn, false);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	if (!is_conn_handled(conn)) {
		return;
	}

	LOG_WRN("Pairing failed");
	fp_keys_drop_key(conn);
	update_conn_handling(conn, false);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.pairing_accept = auth_pairing_accept,
	.passkey_display = auth_passkey_display,
	.passkey_confirm = auth_passkey_confirm,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

static int fp_auth_send_pairing_req(struct bt_conn *conn)
{
	int err = bt_conn_set_security(conn, BT_SECURITY_L4);

	if (err) {
		LOG_WRN("Initiate pairing failed: err=%d", err);
	} else {
		LOG_DBG("Initiated pairing");
	}

	return err;
}

int fp_auth_start(struct bt_conn *conn, bool send_pairing_req)
{
	int err = 0;
	size_t conn_idx = bt_conn_index(conn);
	static bool auth_info_cb_registered;

	if (is_conn_handled(conn)) {
		LOG_ERR("Auth started twice for connection %p", (void *)conn);
		return -EALREADY;
	}

	err = bt_conn_auth_cb_overlay(conn, &conn_auth_callbacks);
	if (err) {
		return err;
	}

	if (!auth_info_cb_registered) {
		err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
		if (err) {
			return err;
		}

		auth_info_cb_registered = true;
	}

	if (send_pairing_req) {
		err = fp_auth_send_pairing_req(conn);
	}

	if (!err) {
		bt_auth_peer_passkeys[conn_idx] = EMPTY_PASSKEY;
		update_conn_handling(conn, true);
	}

	return err;
}

int fp_auth_cmp_passkey(struct bt_conn *conn, uint32_t gatt_passkey, uint32_t *bt_auth_passkey)
{
	int err = 0;
	size_t conn_idx = bt_conn_index(conn);

	if (!is_conn_handled(conn)) {
		LOG_ERR("Unhandled conn (authentication compare passkey): %p", (void *)conn);
		return -EINVAL;
	}

	if (bt_auth_peer_passkeys[conn_idx] == EMPTY_PASSKEY) {
		return -ENOENT;
	} else if (bt_auth_peer_passkeys[conn_idx] == USED_PASSKEY) {
		return -EACCES;
	}

	*bt_auth_passkey = bt_auth_peer_passkeys[conn_idx];

	if (gatt_passkey == *bt_auth_passkey) {
		err = send_auth_confirm(conn);
	} else {
		err = send_auth_cancel(conn);
	}

	bt_auth_peer_passkeys[conn_idx] = USED_PASSKEY;
	fp_keys_bt_auth_progress(conn, false);

	return err;
}
