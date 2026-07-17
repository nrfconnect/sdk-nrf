/*
 * Copyright (c) 2024-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <errno.h>

#include <zephyr/kernel.h>

#include <dult/dult.h>
#include "dult_user.h"
#include "dult_user_slot.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_id, CONFIG_DULT_LOG_LEVEL);

static bool is_enabled;
static bool id_read_state;

/* Per-user memory reference holding the user's identifier read state callback. */
static size_t id_cb_id = DULT_USER_SLOT_MEM_REF_ID_UNSET;

/* Resolve the identifier read state callback of the currently associated user. */
static const struct dult_id_read_state_cb *id_cb_get(void)
{
	void *ref = NULL;

	if (id_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		return NULL;
	}

	(void) dult_user_slot_mem_ref_get(dult_user_get_associated(), id_cb_id, &ref);

	return ref;
}

static void id_read_state_timeout_handle(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(id_read_state_timeout_work,
			       id_read_state_timeout_handle);

int dult_id_read_state_cb_register(const struct dult_user *user,
				   const struct dult_id_read_state_cb *cb)
{
	int err;
	void *ref = NULL;

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (!cb || !cb->payload_get || !cb->exited) {
		return -EINVAL;
	}

	if (id_cb_id == DULT_USER_SLOT_MEM_REF_ID_UNSET) {
		err = dult_user_slot_mem_ref_register(&id_cb_id);
		if (err) {
			return err;
		}
	}

	/* Stored per user, persists across dult_reset(); cleared by
	 * dult_user_unregister(). Registering twice for the same user is rejected.
	 */

	(void) dult_user_slot_mem_ref_get(user, id_cb_id, &ref);
	if (ref) {
		LOG_ERR("DULT ID: identifier read state callback already registered");
		return -EALREADY;
	}

	return dult_user_slot_mem_ref_set(user, id_cb_id, (void *)cb);
}

static void id_read_state_timeout_handle(struct k_work *work)
{
	ARG_UNUSED(work);
	__ASSERT_NO_MSG(dult_is_any_associated());
	__ASSERT_NO_MSG(id_read_state);

	id_read_state = false;
	id_cb_get()->exited();
}

int dult_id_read_state_enter(const struct dult_user *user)
{
	if (!dult_is_any_associated()) {
		LOG_ERR("DULT ID: module is not enabled");
		return -EACCES;
	}

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	id_read_state = true;
	(void) k_work_reschedule(&id_read_state_timeout_work,
				 K_MINUTES(CONFIG_DULT_ID_READ_STATE_TIMEOUT));

	return 0;
}

bool dult_id_is_in_read_state(void)
{
	__ASSERT_NO_MSG(dult_is_any_associated());

	return id_read_state;
}

int dult_id_payload_get(uint8_t *buf, size_t *len)
{
	__ASSERT_NO_MSG(dult_is_any_associated());
	__ASSERT_NO_MSG(id_read_state);

	return id_cb_get()->payload_get(buf, len);
}

int dult_id_enable(void)
{
	if (is_enabled) {
		LOG_ERR("DULT ID: already enabled");
		return -EALREADY;
	}

	if (!id_cb_get()) {
		LOG_ERR("DULT ID: callbacks must be registered at this point");
		return -EINVAL;
	}

	is_enabled = true;

	return 0;
}

int dult_id_reset(void)
{
	if (!is_enabled) {
		LOG_ERR("DULT ID: is not enabled");
		return -EALREADY;
	}

	is_enabled = false;
	if (id_read_state) {
		id_read_state = false;
		id_cb_get()->exited();
	}
	(void) k_work_cancel_delayable(&id_read_state_timeout_work);

	return 0;
}
