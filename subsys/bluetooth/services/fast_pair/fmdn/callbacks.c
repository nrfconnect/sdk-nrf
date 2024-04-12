/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_callbacks, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fmdn.h>

static sys_slist_t fmdn_info_cb_slist = SYS_SLIST_STATIC_INIT(&fmdn_info_cb_slist);
static sys_slist_t fmdn_info_cb_internal_slist =
	SYS_SLIST_STATIC_INIT(&fmdn_info_cb_internal_slist);

void fp_fmdn_callbacks_clock_synced_notify(void)
{
	sys_slist_t *slists[] = {
		&fmdn_info_cb_internal_slist,
		&fmdn_info_cb_slist
	};

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	for (size_t i = 0; i < ARRAY_SIZE(slists); i++) {
		struct bt_fast_pair_fmdn_info_cb *listener;

		SYS_SLIST_FOR_EACH_CONTAINER(slists[i], listener, node) {
			if (listener->clock_synced) {
				listener->clock_synced();
			}
		}
	}
}

void fp_fmdn_callbacks_provisioning_state_changed_notify(bool provisioned)
{
	sys_slist_t *slists[] = {
		&fmdn_info_cb_internal_slist,
		&fmdn_info_cb_slist
	};

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	for (size_t i = 0; i < ARRAY_SIZE(slists); i++) {
		struct bt_fast_pair_fmdn_info_cb *listener;

		SYS_SLIST_FOR_EACH_CONTAINER(slists[i], listener, node) {
			if (listener->provisioning_state_changed) {
				listener->provisioning_state_changed(provisioned);
			}
		}
	}
}

static bool node_uniqueness_validate(sys_slist_t *slist, sys_snode_t *new_node)
{
	sys_snode_t *current_node;

	SYS_SLIST_FOR_EACH_NODE(slist, current_node) {
		if (current_node == new_node) {
			return false;
		}
	}

	return true;
}

static int cb_register(sys_slist_t *slist, struct bt_fast_pair_fmdn_info_cb *cb)
{
	if (bt_fast_pair_is_ready()) {
		return -EOPNOTSUPP;
	}

	if (!cb) {
		return -EINVAL;
	}

	if (!cb->clock_synced && !cb->provisioning_state_changed) {
		return -EINVAL;
	}

	if (!node_uniqueness_validate(slist, &cb->node)) {
		return 0;
	}

	sys_slist_append(slist, &cb->node);

	return 0;
}

int fp_fmdn_callbacks_info_cb_register(struct bt_fast_pair_fmdn_info_cb *cb)
{
	return cb_register(&fmdn_info_cb_internal_slist, cb);
}

int bt_fast_pair_fmdn_info_cb_register(struct bt_fast_pair_fmdn_info_cb *cb)
{
	return cb_register(&fmdn_info_cb_slist, cb);
}
