/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <errno.h>

#include "dult.h"
#include "dult_near_owner_state.h"
#include "dult_user.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dult_near_owner_state, CONFIG_DULT_LOG_LEVEL);

static enum dult_near_owner_state_mode cur_mode = DULT_NEAR_OWNER_STATE_MODE_NEAR_OWNER;

int dult_near_owner_state_set(const struct dult_user *user, enum dult_near_owner_state_mode mode)
{
	if (!dult_user_is_registered(user)) {
		return -EACCES;
	}

	cur_mode = mode;

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
