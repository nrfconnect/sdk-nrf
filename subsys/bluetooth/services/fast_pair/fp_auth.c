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

#include <bluetooth/services/fast_pair/fast_pair.h>
#include "fp_activation.h"
#include "fp_auth.h"
#include "fp_keys.h"

#define EMPTY_PASSKEY	0xffffffff
#define USED_PASSKEY	0xfffffffe

static uint32_t bt_auth_peer_passkeys[CONFIG_BT_MAX_CONN];
static bool is_enabled;

#if (CONFIG_BT_MAX_CONN <= 8)
static uint8_t handled_conn_bm;
static uint8_t pairing_conn_bm;
#elif (CONFIG_BT_MAX_CONN <= 16)
static uint16_t handled_conn_bm;
static uint16_t pairing_conn_bm;
#elif (CONFIG_BT_MAX_CONN <= 32)
static uint32_t handled_conn_bm;
static uint32_t pairing_conn_bm;
#else
#error "CONFIG_BT_MAX_CONN must be <= 32"
#endif


static bool is_conn_handled(const struct bt_conn *conn)
{
	return ((handled_conn_bm & BIT(bt_conn_index(conn))) != 0);
}

static bool is_conn_in_pairing_state(const struct bt_conn *conn)
{
	return ((pairing_conn_bm & BIT(bt_conn_index(conn))) != 0);
}

static void update_conn_handling(const struct bt_conn *conn, bool handled)
{
	size_t conn_idx = bt_conn_index(conn);

	WRITE_BIT(handled_conn_bm, conn_idx, handled);
}

static void update_conn_pairing_state(const struct bt_conn *conn, bool handled)
{
	size_t conn_idx = bt_conn_index(conn);

	WRITE_BIT(pairing_conn_bm, conn_idx, handled);
}

static void clear_conn_context(const struct bt_conn *conn)
{
	update_conn_handling(conn, false);
	update_conn_pairing_state(conn, false);
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
	if (!bt_fast_pair_is_ready()) {
		return;
	}

	clear_conn_context(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

static enum bt_security_err auth_pairing_accept(struct bt_conn *conn,
						const struct bt_conn_pairing_feat *const feat)
{
	if (!bt_fast_pair_is_ready()) {
		LOG_WRN("Fast Pair not enabled");
		return BT_SECURITY_ERR_UNSPECIFIED;
	}

	if (!is_conn_handled(conn)) {
		LOG_ERR("Invalid conn (pairing accept): %p", (void *)conn);
		return BT_SECURITY_ERR_UNSPECIFIED;
	}

	update_conn_pairing_state(conn, true);

	if (feat->io_capability == BT_IO_NO_INPUT_OUTPUT) {
		LOG_ERR("Invalid IO capability (pairing accept): %p", (void *)conn);
		return BT_SECURITY_ERR_AUTH_REQUIREMENT;
	}

	fp_keys_bt_auth_progress(conn, false);

	return BT_SECURITY_ERR_SUCCESS;
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	if (!bt_fast_pair_is_ready()) {
		LOG_WRN("Fast Pair not enabled");
		(void)send_auth_cancel(conn);
		return;
	}

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
	if (!bt_fast_pair_is_ready()) {
		LOG_WRN("Fast Pair not enabled");
		(void)send_auth_cancel(conn);
		return;
	}

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
	if (!bt_fast_pair_is_ready()) {
		LOG_WRN("Fast Pair not enabled");
		return;
	}

	if (!is_conn_handled(conn)) {
		LOG_ERR("Invalid conn (authentication cancel): %p", (void *)conn);
		return;
	}

	LOG_WRN("Authentication cancel");
	clear_conn_context(conn);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	if (!bt_fast_pair_is_ready()) {
		return;
	}

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

	clear_conn_context(conn);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	if (!bt_fast_pair_is_ready()) {
		return;
	}

	if (!is_conn_handled(conn)) {
		return;
	}

	LOG_WRN("Pairing failed");
	fp_keys_drop_key(conn);
	clear_conn_context(conn);
}

static const struct bt_conn_auth_cb conn_auth_callbacks = {
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
		update_conn_pairing_state(conn, true);
	}

	return err;
}

int fp_auth_start(struct bt_conn *conn, bool send_pairing_req)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	int err = 0;
	size_t conn_idx = bt_conn_index(conn);

	if (is_conn_handled(conn)) {
		LOG_ERR("Auth started twice for connection %p", (void *)conn);
		return -EALREADY;
	}

	err = bt_conn_auth_cb_overlay(conn, &conn_auth_callbacks);
	if (err) {
		return err;
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
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

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

int fp_auth_finalize(struct bt_conn *conn)
{
	/* Accept only the standard Fast Pair procedure flow. */
	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_REQ_PAIRING)) {
		return 0;
	}

	/* Allow special Fast Pair procedure flow without Bluetooth LE pairing
	 * only for Providers that do not follow the standard flow.
	 */
	if (bt_conn_get_security(conn) != BT_SECURITY_L1) {
		return 0;
	}

	if (!is_conn_handled(conn)) {
		LOG_ERR("Invalid conn (finalizing authentication): %p", (void *)conn);
		return -EINVAL;
	}

	if (is_conn_in_pairing_state(conn)) {
		LOG_ERR("Pairing in progress for conn (finalizing authentication): %p",
			(void *)conn);
		return -EACCES;
	}

	fp_keys_bt_auth_progress(conn, true);

	clear_conn_context(conn);

	return 0;
}

static int fp_auth_init(void)
{
	if (is_enabled) {
		LOG_WRN("fp_auth module already initialized");
		return 0;
	}

	int err;

	memset(bt_auth_peer_passkeys, 0, sizeof(bt_auth_peer_passkeys));
	handled_conn_bm = 0;
	pairing_conn_bm = 0;

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		return err;
	}

	is_enabled = true;

	return 0;
}

static void handled_conn_disconnect(struct bt_conn *conn, void *data)
{
	if (is_conn_handled(conn)) {
		int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

		if (!err) {
			LOG_DBG("Disconnected handled connection");
		} else if (err == -ENOTCONN) {
			LOG_DBG("Handled connection already disconnected");
		} else {
			LOG_ERR("Failed to disconnect handled conn: err=%d", err);
		}
	}
}

static int fp_auth_uninit(void)
{
	if (!is_enabled) {
		LOG_WRN("fp_auth module already uninitialized");
		return 0;
	}

	int err;

	is_enabled = false;

	err = bt_conn_auth_info_cb_unregister(&conn_auth_info_callbacks);
	if (err) {
		return err;
	}

	bt_conn_foreach(BT_CONN_TYPE_LE, handled_conn_disconnect, NULL);

	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_auth, FP_ACTIVATION_INIT_PRIORITY_DEFAULT, fp_auth_init,
			      fp_auth_uninit);
