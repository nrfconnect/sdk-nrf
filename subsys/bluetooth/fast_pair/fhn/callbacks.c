/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fhn_callbacks, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/fast_pair/fast_pair.h>
#include <bluetooth/fast_pair/fhn/fhn.h>

#include "fp_activation.h"

static sys_slist_t fhn_info_cb_slist = SYS_SLIST_STATIC_INIT(&fhn_info_cb_slist);
static sys_slist_t fhn_info_cb_internal_slist =
	SYS_SLIST_STATIC_INIT(&fhn_info_cb_internal_slist);

void fp_fhn_callbacks_clock_synced_notify(void)
{
	sys_slist_t *slists[] = {
		&fhn_info_cb_internal_slist,
		&fhn_info_cb_slist
	};

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	for (size_t i = 0; i < ARRAY_SIZE(slists); i++) {
		struct bt_fast_pair_fhn_info_cb *listener;

		SYS_SLIST_FOR_EACH_CONTAINER(slists[i], listener, node) {
			if (listener->clock_synced) {
				listener->clock_synced();
			}
		}
	}
}

void fp_fhn_callbacks_conn_authenticated_notify(struct bt_conn *conn)
{
	sys_slist_t *slists[] = {
		&fhn_info_cb_internal_slist,
		&fhn_info_cb_slist
	};

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	for (size_t i = 0; i < ARRAY_SIZE(slists); i++) {
		struct bt_fast_pair_fhn_info_cb *listener;

		SYS_SLIST_FOR_EACH_CONTAINER(slists[i], listener, node) {
			if (listener->conn_authenticated) {
				listener->conn_authenticated(conn);
			}
		}
	}
}

void fp_fhn_callbacks_provisioning_state_changed_notify(bool provisioned)
{
	sys_slist_t *slists[] = {
		&fhn_info_cb_internal_slist,
		&fhn_info_cb_slist
	};

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	for (size_t i = 0; i < ARRAY_SIZE(slists); i++) {
		struct bt_fast_pair_fhn_info_cb *listener;

		SYS_SLIST_FOR_EACH_CONTAINER(slists[i], listener, node) {
			if (listener->provisioning_state_changed) {
				listener->provisioning_state_changed(provisioned);
			}
		}
	}
}

void fp_fhn_callbacks_dult_ownership_state_changed_notify(bool state, bool is_owner)
{
	sys_slist_t *slists[] = {
		&fhn_info_cb_internal_slist,
		&fhn_info_cb_slist
	};

	for (size_t i = 0; i < ARRAY_SIZE(slists); i++) {
		struct bt_fast_pair_fhn_info_cb *listener;

		SYS_SLIST_FOR_EACH_CONTAINER(slists[i], listener, node) {
			if (listener->dult_ownership_state_changed) {
				listener->dult_ownership_state_changed(state, is_owner);
			}
		}
	}
}

static bool dult_ownership_state_changed_cb_is_registered(void)
{
	sys_slist_t *slists[] = {
		&fhn_info_cb_internal_slist,
		&fhn_info_cb_slist
	};

	for (size_t i = 0; i < ARRAY_SIZE(slists); i++) {
		struct bt_fast_pair_fhn_info_cb *listener;

		SYS_SLIST_FOR_EACH_CONTAINER(slists[i], listener, node) {
			if (listener->dult_ownership_state_changed) {
				return true;
			}
		}
	}

	return false;
}

static int cb_register(sys_slist_t *slist, struct bt_fast_pair_fhn_info_cb *cb)
{
	if (bt_fast_pair_is_ready()) {
		return -EOPNOTSUPP;
	}

	if (!cb) {
		return -EINVAL;
	}

	if (!cb->clock_synced && !cb->provisioning_state_changed &&
	    !cb->dult_ownership_state_changed) {
		return -EINVAL;
	}

	if (cb->dult_ownership_state_changed && !IS_ENABLED(CONFIG_DULT_API_VARIANT_V2)) {
		LOG_DBG("FHN Callbacks: dult_ownership_state_changed callback is unused "
			"with the DULT v1 API; it is never invoked");
	} else if (cb->dult_ownership_state_changed && !IS_ENABLED(CONFIG_DULT_MULTI_USER)) {
		LOG_DBG("FHN Callbacks: dult_ownership_state_changed callback is optional "
			"when a single DULT user is configured");
	}

	if (sys_slist_find(slist, &cb->node, NULL)) {
		return 0;
	}

	sys_slist_append(slist, &cb->node);

	return 0;
}

int fp_fhn_callbacks_info_cb_register(struct bt_fast_pair_fhn_info_cb *cb)
{
	return cb_register(&fhn_info_cb_internal_slist, cb);
}

int bt_fast_pair_fhn_info_cb_register(struct bt_fast_pair_fhn_info_cb *cb)
{
	return cb_register(&fhn_info_cb_slist, cb);
}

static int fp_fhn_callbacks_init(void)
{
	/* dult_ownership_state_changed is mandatory whenever more than one DULT
	 * user can be registered concurrently, since it is the only way to learn
	 * about eviction and re-arbitration.
	 */
	if (IS_ENABLED(CONFIG_DULT_MULTI_USER) &&
	    !dult_ownership_state_changed_cb_is_registered()) {
		LOG_ERR("FHN Callbacks: dult_ownership_state_changed callback is mandatory "
			"when more than one DULT user is configured");
		return -EINVAL;
	}

	return 0;
}

static int fp_fhn_callbacks_uninit(void)
{
	/* Intentionally left empty. */
	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_fhn_callbacks,
			      FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      fp_fhn_callbacks_init,
			      fp_fhn_callbacks_uninit);
