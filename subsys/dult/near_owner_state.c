/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <errno.h>
#include <zephyr/sys/slist.h>

#include "dult.h"
#include "dult_near_owner_state.h"
#include "dult_user.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_near_owner_state, CONFIG_DULT_LOG_LEVEL);

static enum dult_near_owner_state_mode cur_mode = DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER;
static sys_slist_t state_cb_slist = SYS_SLIST_STATIC_INIT(&state_cb_slist);

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

void dult_near_owner_state_cb_register(struct dult_near_owner_state_cb *cb)
{
	__ASSERT_NO_MSG(cb && cb->state_changed);

	if (!node_uniqueness_validate(&state_cb_slist, &cb->node)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	sys_slist_append(&state_cb_slist, &cb->node);
}

int dult_near_owner_state_set(const struct dult_user *user, enum dult_near_owner_state_mode mode)
{
	enum dult_near_owner_state_mode prev_mode = cur_mode;

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	cur_mode = mode;
	if (prev_mode != cur_mode) {
		struct dult_near_owner_state_cb *listener;

		SYS_SLIST_FOR_EACH_CONTAINER(&state_cb_slist, listener, node) {
			listener->state_changed(cur_mode);
		}
	}

	return 0;
}

enum dult_near_owner_state_mode dult_near_owner_state_get(void)
{
	return cur_mode;
}

void dult_near_owner_state_reset(void)
{
	cur_mode = DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER;
}
