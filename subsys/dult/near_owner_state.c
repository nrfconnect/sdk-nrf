/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include <dult/dult.h>
#include "dult_near_owner_state.h"
#include "dult_user.h"
#include "dult_user_slot.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_near_owner_state, CONFIG_DULT_LOG_LEVEL);

/* Per-user near-owner state, stored in the user slot so each registered user keeps
 * its own value and can set it during the pre-association window. The accessory uses
 * the currently associated user's value.
 */
static size_t near_owner_id = DULT_USER_SLOT_MEM_REF_ID_UNSET;
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
	enum dult_near_owner_state_mode prev_mode;
	bool effective;
	int err;

	/* Registered-gated in both builds: this is what lets a multi-user network stage
	 * its value before it becomes the associated user (previously associated-only).
	 * Single-user gating is unchanged.
	 */
	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (near_owner_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		err = dult_user_slot_mem_ref_register(&near_owner_id);
		if (err) {
			return err;
		}
	}

	/* Only the associated user drives the effective state and its listeners; a
	 * non-associated user's set is staged silently and read live once it becomes
	 * associated.
	 */
	effective = (user == dult_user_get_associated());
	prev_mode = effective ? dult_near_owner_state_get() : DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER;

	err = dult_user_slot_mem_ref_val_set(user, near_owner_id, mode);
	if (err) {
		return err;
	}

	if (effective && (prev_mode != mode)) {
		struct dult_near_owner_state_cb *listener;

		SYS_SLIST_FOR_EACH_CONTAINER(&state_cb_slist, listener, node) {
			listener->state_changed(mode);
		}
	}

	return 0;
}

enum dult_near_owner_state_mode dult_near_owner_state_get(void)
{
	uint32_t val;

	/* Effective state = the currently associated user's value; default otherwise. */
	if (near_owner_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		return DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER;
	}

	if (dult_user_slot_mem_ref_val_get(dult_user_get_associated(), near_owner_id, &val) != 0) {
		return DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER;
	}

	return (enum dult_near_owner_state_mode)val;
}
