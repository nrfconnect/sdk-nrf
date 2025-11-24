/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * This file implements an NCS extension to Zephyr that allows the
 * application to set a custom LTK from a vendor-defined source for a
 * connection. This is a provisional feature that will likely be
 * replaced by a standard Bluetooth feature in a future version of the
 * specification.
 */

#include <autoconf.h>
#include <bluetooth/nrf/host_extensions.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(bt_nrf_conn_set_ltk, CONFIG_BT_KEYS_LOG_LEVEL);

/* These are Zephyr Host internal headers. These are not public headers,
 * instead we use a path relative to `zephyr/include` and escape up one
 * directory to get to the root of the Zephyr project.
 */
#include "../subsys/bluetooth/host/keys.h"
#include "../subsys/bluetooth/host/conn_internal.h"

/* This enum extends the enum used for `bt_keys.flags`.
 */
enum {
	NRF_BT_KEYS_TRANSIENT = BIT(7),
};

int bt_nrf_conn_set_ltk(struct bt_conn *conn, const struct bt_nrf_ltk *ltk, bool authenticated)
{
	struct bt_keys *keys;

	if (conn == NULL || ltk == NULL) {
		return -EINVAL;
	}

	/* This is a crude but decent prevention of any race conditions with
	 * smp.c.
	 *
	 * It should prevent a race with smp_pairing_req(),
	 * smp_security_request() and bt_smp_start_security(), which will all
	 * allocate keys early and cause the following code to find a key and
	 * return -EALREADY or conversely find the LTK we set here and use it as
	 * if it's from a bond.
	 */
	k_sched_lock();

	/* If we find keys for this address it means a pairing or encryption may
	 * already be under way, or there is a prior bond for this address. We
	 * don't allow setting the LTK in this case. This is to avoid race
	 * conditions with unintended security consequences.
	 *
	 * Even if the keys were created with this API, we might still be racing
	 * with an encryption process. To avoid this, we treat setting the LTK
	 * as creating a transient bond, and in effect don't allow setting the
	 * LTK multiple times for the same connection.
	 */
	keys = bt_keys_find(BT_KEYS_ALL, conn->id, &conn->le.dst);
	if (keys) {
		char str[BT_ADDR_LE_STR_LEN];

		k_sched_unlock();

		bt_addr_le_to_str(&conn->le.dst, str, sizeof(str));
		LOG_ERR("Cannot safely set LTK for %s. A bt_keys already exists. Please unpair "
			"first.",
			str);

		return -EALREADY;
	}

	/* We are using "get" variant here so it allocates.
	 */
	keys = bt_keys_get_addr(conn->id, &conn->le.dst);
	if (!keys) {
		char str[BT_ADDR_LE_STR_LEN];

		k_sched_unlock();

		bt_addr_le_to_str(&conn->le.dst, str, sizeof(str));
		LOG_ERR("Failed to allocate bt_keys for %s.", str);

		return -ENOMEM;
	}

	conn->le.keys = keys;

	/* Use the LESC LTK type, which is symmetric for central and peripheral.
	 */
	keys->keys = BT_KEYS_LTK_P256;

	memcpy(keys->ltk.val, ltk->val, sizeof(ltk->val));
	keys->enc_size = sizeof(ltk->val);

	keys->flags = 0;
	keys->flags |= BT_KEYS_SC;
	keys->flags |= NRF_BT_KEYS_TRANSIENT;

	if (authenticated) {
		keys->flags |= BT_KEYS_AUTHENTICATED;
	}

	k_sched_unlock();

	return 0;
}

static void _on_disconnect(struct bt_conn *conn, uint8_t reason)
{
	struct bt_keys *keys = conn->le.keys;

	ARG_UNUSED(reason);

	/* Completely deallocate the keys entry to prevent leaking keys_pool
	 * entries. `bt_smp_disconnected` does that for paired-not-bonded
	 * bt_keys, but it recognizes those by finding a zero in
	 * `bt_keys.keys`. But our `bt_keys` do have an LTK flag. So we need to
	 * do the job ourselves.
	 */
	if (keys && keys->flags & NRF_BT_KEYS_TRANSIENT) {
		conn->le.keys = NULL;
		bt_keys_clear(keys);
	}
}

static void _on_security_changed(struct bt_conn *conn, bt_security_t level,
				 enum bt_security_err err)
{
	struct bt_keys *keys = conn->le.keys;

	ARG_UNUSED(level);
	ARG_UNUSED(err);

	/* Clear the LTK so that this bt_keys looks like it belongs to a
	 * paired-not-bonded device, so the bt_keys is not saved to flash.
	 * Clearing the key is also good for security.
	 *
	 * We do not deallocate here, so that `bt_conn_get_info` can pick up
	 * info like key size and SC flag.
	 *
	 * If the encryption has failed, for example with
	 * BT_SECURITY_ERR_PIN_OR_KEY_MISSING, we clear it anyway, since it
	 * obviously doesn't work. It shouldn't happen if the application
	 * does everything correctly. We don't deallocate the keys entry
	 * because we don't want to risk any race conditions, just in case.
	 * If users want to retry (potentially with a different LTK), they
	 * have to disconnect and reconnect to do so.
	 */
	if (keys && keys->flags & NRF_BT_KEYS_TRANSIENT) {
		keys->keys = 0;
		keys->ltk = (struct bt_ltk){};
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = _on_disconnect,
	.security_changed = _on_security_changed,
};
