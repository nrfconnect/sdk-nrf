/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/settings/settings.h>

#include "fp_storage_clock.h"
#include "fp_storage_clock_priv.h"
#include "fp_storage_manager.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_storage, CONFIG_FP_STORAGE_LOG_LEVEL);

#define CLOCK_BOOT_CHECKPOINT_UNPOPULATED (UINT32_MAX)

static uint32_t clock_boot_checkpoint = CLOCK_BOOT_CHECKPOINT_UNPOPULATED;
static uint32_t clock_current_checkpoint;

static int settings_rc;
static bool is_enabled;

int fp_storage_clock_boot_checkpoint_get(uint32_t *boot_checkpoint)
{
	if (!is_enabled) {
		return -EACCES;
	}

	__ASSERT_NO_MSG(boot_checkpoint);
	__ASSERT_NO_MSG(clock_boot_checkpoint != CLOCK_BOOT_CHECKPOINT_UNPOPULATED);
	*boot_checkpoint = clock_boot_checkpoint;

	return 0;
}

int fp_storage_clock_current_checkpoint_get(uint32_t *current_checkpoint)
{
	if (!is_enabled) {
		return -EACCES;
	}

	__ASSERT_NO_MSG(current_checkpoint);
	*current_checkpoint = clock_current_checkpoint;

	return 0;
}

int fp_storage_clock_checkpoint_update(uint32_t current_clock)
{
	int err;

	if (!is_enabled) {
		return -EACCES;
	}

	err = settings_save_one(SETTINGS_CLOCK_FULL_NAME,
				&current_clock,
				sizeof(current_clock));
	if (err) {
		return err;
	}

	clock_current_checkpoint = current_clock;

	return 0;
}

static int fp_settings_clock_load(size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;

	if (len != sizeof(clock_current_checkpoint)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &clock_current_checkpoint, sizeof(clock_current_checkpoint));
	if (rc < 0) {
		return rc;
	}

	if (rc != sizeof(clock_current_checkpoint)) {
		return -EINVAL;
	}

	return 0;
}

static int fp_storage_clock_set(const char *name,
				size_t len,
				settings_read_cb read_cb,
				void *cb_arg)
{
	int err;

	LOG_DBG("FP Storage Clock: the clock node is being set by Settings");

	if (!strncmp(name, SETTINGS_CLOCK_KEY_NAME, sizeof(SETTINGS_CLOCK_KEY_NAME))) {
		err = fp_settings_clock_load(len, read_cb, cb_arg);
	} else {
		err = -ENOENT;
	}

	/* The first reported settings set error will be remembered by the module.
	 * The error will then be returned when calling fp_storage_clock_init.
	 */
	if (err && !settings_rc) {
		settings_rc = err;
	}

	return 0;
}

static int fp_storage_clock_init(void)
{
	if (is_enabled) {
		LOG_WRN("FP Storage Clock: module already initialized");
		return 0;
	}

	if (settings_rc) {
		return settings_rc;
	}

	if (clock_boot_checkpoint == CLOCK_BOOT_CHECKPOINT_UNPOPULATED) {
		clock_boot_checkpoint = clock_current_checkpoint;
	}

	LOG_DBG("FP Storage Clock: initializing clock value to: %u [s]",
		clock_boot_checkpoint);

	is_enabled = true;

	return 0;
}

static int fp_storage_clock_uninit(void)
{
	is_enabled = false;

	return 0;
}

void fp_storage_clock_ram_clear(void)
{
	clock_boot_checkpoint = CLOCK_BOOT_CHECKPOINT_UNPOPULATED;
	clock_current_checkpoint = 0;

	settings_rc = 0;

	is_enabled = false;
}

static int fp_storage_clock_reset_perform(void)
{
	int err;
	bool was_enabled = is_enabled;

	err = settings_delete(SETTINGS_CLOCK_FULL_NAME);
	if (err) {
		LOG_ERR("FP Storage Clock: settings_delete failed: %d", err);
		return err;
	}

	fp_storage_clock_ram_clear();

	if (was_enabled) {
		err = fp_storage_clock_init();
		if (err) {
			return err;
		}
	}

	return 0;
}

static void fp_storage_clock_reset_prepare(void)
{
	/* intentionally left empty */
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_storage_clock,
			       SETTINGS_CLOCK_SUBTREE_NAME,
			       NULL,
			       fp_storage_clock_set,
			       NULL,
			       NULL);

FP_STORAGE_MANAGER_MODULE_REGISTER(fp_storage_clock,
				   fp_storage_clock_reset_perform,
				   fp_storage_clock_reset_prepare,
				   fp_storage_clock_init,
				   fp_storage_clock_uninit);
