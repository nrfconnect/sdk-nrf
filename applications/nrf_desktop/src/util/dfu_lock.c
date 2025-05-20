/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dfu_lock, CONFIG_DESKTOP_DFU_LOCK_LOG_LEVEL);

#include "dfu_lock.h"

static const struct dfu_lock_owner *previous_owner;
static const struct dfu_lock_owner *current_owner;

static K_MUTEX_DEFINE(dfu_lock);

int dfu_lock_claim(const struct dfu_lock_owner *new_owner)
{
	int ret;
	int err;

	__ASSERT_NO_MSG(new_owner != NULL);

	err = k_mutex_lock(&dfu_lock, K_FOREVER);
	__ASSERT_NO_MSG(!err);
	ARG_UNUSED(err);

	if (current_owner == new_owner) {
		/* The DFU lock has already been claimed by the owner. */
		ret = 0;
	} else if (!current_owner) {
		/* The DFU lock is free and can be claimed by the new owner. */
		ret = 0;
		current_owner = new_owner;

		if (previous_owner && previous_owner->owner_changed) {
			if (previous_owner != current_owner) {
				previous_owner->owner_changed(new_owner);
			}
		}

		LOG_DBG("New DFU owner claimed: %s", new_owner->name);
	} else {
		/* The DFU lock has already been claimed by another owner. */
		ret = -EPERM;
	}

	err = k_mutex_unlock(&dfu_lock);
	__ASSERT_NO_MSG(!err);

	return ret;
}

int dfu_lock_release(const struct dfu_lock_owner *owner)
{
	int ret;
	int err;

	err = k_mutex_lock(&dfu_lock, K_FOREVER);
	__ASSERT_NO_MSG(!err);
	ARG_UNUSED(err);

	if (current_owner == owner) {
		ret = 0;

		previous_owner = current_owner;
		current_owner = NULL;

		LOG_DBG("DFU lock released by %s", owner->name);
	} else {
		ret = -EPERM;
	}

	err = k_mutex_unlock(&dfu_lock);
	__ASSERT_NO_MSG(!err);

	return ret;
}
