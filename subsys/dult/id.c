/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <errno.h>

#include <zephyr/kernel.h>

#include "dult.h"
#include "dult_user.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_id, CONFIG_DULT_LOG_LEVEL);

static bool is_enabled;
static const struct dult_id_read_state_cb *id_read_state_cb;
static bool id_read_state;

static void id_read_state_timeout_handle(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(id_read_state_timeout_work,
			       id_read_state_timeout_handle);

int dult_id_read_state_cb_register(const struct dult_user *user,
				   const struct dult_id_read_state_cb *cb)
{
	if (dult_user_is_ready()) {
		LOG_ERR("DULT ID: module must be disabled to register callbacks");
		return -EACCES;
	}

	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	if (id_read_state_cb) {
		LOG_ERR("DULT ID: identifier read state callback already registered");
		return -EALREADY;
	}

	if (!cb || !cb->payload_get || !cb->exited) {
		return -EINVAL;
	}

	id_read_state_cb = cb;

	return 0;
}

static void id_read_state_timeout_handle(struct k_work *work)
{
	ARG_UNUSED(work);
	__ASSERT_NO_MSG(dult_user_is_ready());
	__ASSERT_NO_MSG(id_read_state);

	id_read_state = false;
	id_read_state_cb->exited();
}

int dult_id_read_state_enter(const struct dult_user *user)
{
	if (!dult_user_is_ready()) {
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
	__ASSERT_NO_MSG(dult_user_is_ready());

	return id_read_state;
}

int dult_id_payload_get(uint8_t *buf, size_t *len)
{
	__ASSERT_NO_MSG(dult_user_is_ready());
	__ASSERT_NO_MSG(id_read_state);

	return id_read_state_cb->payload_get(buf, len);
}

int dult_id_enable(void)
{
	if (is_enabled) {
		LOG_ERR("DULT ID: already enabled");
		return -EALREADY;
	}

	if (!id_read_state_cb) {
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
		id_read_state_cb->exited();
	}
	(void) k_work_cancel_delayable(&id_read_state_timeout_work);

	id_read_state_cb = NULL;
	return 0;
}
