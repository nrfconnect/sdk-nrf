/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/bluetooth/conn.h>

#include <bluetooth/services/fast_pair/fast_pair.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include "fp_activation.h"
#include "fp_storage_ak_bond.h"

struct bond_find_ctx {
	bool bond_found;
	const bt_addr_le_t *bond_addr;
};

static bool is_enabled;

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	if (!bt_fast_pair_is_ready()) {
		return;
	}

	LOG_DBG("Identity resolved");

	fp_storage_ak_bond_conn_addr_update(conn, identity);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (!bt_fast_pair_is_ready()) {
		return;
	}

	fp_storage_ak_bond_conn_finalize(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
	.identity_resolved = identity_resolved,
};

static void bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	if (!bt_fast_pair_is_ready()) {
		return;
	}

	fp_storage_ak_bond_delete(peer);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.bond_deleted = bond_deleted,
};

static void bond_address_cmp(const struct bt_bond_info *info, void *user_data)
{
	struct bond_find_ctx *ctx = user_data;

	if (bt_addr_le_eq(ctx->bond_addr, &info->addr)) {
		__ASSERT_NO_MSG(!ctx->bond_found);
		ctx->bond_found = true;
	}
}

static int bond_identity_find(uint8_t *identity, const bt_addr_le_t *addr)
{
	struct bond_find_ctx ctx = {
		.bond_found = false,
		.bond_addr = addr,
	};

	for (uint8_t id = 0; id < CONFIG_BT_ID_MAX; id++) {
		bt_foreach_bond(id, bond_address_cmp, &ctx);
		if (ctx.bond_found) {
			if (identity) {
				*identity = id;
			}
		}
	}

	return ctx.bond_found ? 0 : -ESRCH;
}

static int bond_remove(const bt_addr_le_t *addr)
{
	uint8_t identity;
	int err;

	err = bond_identity_find(&identity, addr);
	if (err) {
		return err;
	}

	return bt_unpair(identity, addr);
}

static bool is_addr_bonded(const bt_addr_le_t *addr)
{
	return !bond_identity_find(NULL, addr);
}

static const struct fp_storage_ak_bond_bt_request_cb bt_request_cb = {
	.bond_remove = bond_remove,
	.is_addr_bonded = is_addr_bonded,
};

static int bt_request_cb_register(void)
{
	fp_storage_ak_bond_bt_request_cb_register(&bt_request_cb);
	return 0;
}

static int bond_manager_init(void)
{
	if (is_enabled) {
		LOG_WRN("fp_bond_manager module already initialized");
		return 0;
	}

	int err;

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		return err;
	}

	is_enabled = true;

	return 0;
}

static int bond_manager_uninit(void)
{
	if (!is_enabled) {
		LOG_WRN("fp_bond_manager module already uninitialized");
		return 0;
	}

	int err;

	is_enabled = false;

	err = bt_conn_auth_info_cb_unregister(&conn_auth_info_callbacks);
	if (err) {
		return err;
	}

	return 0;
}

/* Register fp_storage_ak_bond_bt_request_cb callback structure to be able to remove Bluetooth
 * bonds during Fast Pair storage factory reset even if the Fast Pair is not enabled.
 */
SYS_INIT(bt_request_cb_register, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

FP_ACTIVATION_MODULE_REGISTER(fp_bond_manager, FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      bond_manager_init, bond_manager_uninit);
