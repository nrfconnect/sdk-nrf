/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/settings/settings.h>
#include <bluetooth/services/fast_pair.h>

#include "fp_storage_manager.h"
#include "fp_storage_manager_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_storage, CONFIG_FP_STORAGE_LOG_LEVEL);

static bool reset_in_progress;
static int settings_set_err;

static int fp_settings_load_reset_in_progress(size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;

	if (len != sizeof(reset_in_progress)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &reset_in_progress, len);
	if (rc < 0) {
		return rc;
	}

	if (rc != len) {
		return -EINVAL;
	}

	return 0;
}

static void modules_reset_prepare(void)
{
	STRUCT_SECTION_FOREACH(fp_storage_manager_module, module) {
		module->module_reset_prepare();
	}
}

static int fp_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int err = 0;
	const char *key = SETTINGS_RESET_IN_PROGRESS_NAME;

	if (!strncmp(name, key, sizeof(SETTINGS_RESET_IN_PROGRESS_NAME))) {
		err = fp_settings_load_reset_in_progress(len, read_cb, cb_arg);
	} else {
		err = -ENOENT;
	}

	if (reset_in_progress && !err && !settings_set_err) {
		modules_reset_prepare();
	}

	/* The first reported settings set error will be remembered by the module.
	 * The error will then be propagated by the commit callback.
	 * Errors returned in the settings set callback are not propagated further.
	 */
	if (err && !settings_set_err) {
		settings_set_err = err;
	}

	return err;
}

static int reset_in_progress_set(bool in_progress)
{
	int err;

	err = settings_save_one(SETTINGS_RESET_IN_PROGRESS_FULL_NAME, &in_progress,
				sizeof(in_progress));
	if (err) {
		return err;
	}

	reset_in_progress = in_progress;

	return 0;
}

static int modules_reset(void)
{
	STRUCT_SECTION_FOREACH(fp_storage_manager_module, module) {
		int err;

		err = module->module_reset_perform();
		if (err) {
			return err;
		}
	}

	return 0;
}

static int fp_settings_commit(void)
{
	int err;

	if (settings_set_err) {
		return settings_set_err;
	}

	if (reset_in_progress) {
		LOG_WRN("Fast Pair factory reset has been interrupted or aborted. Retrying");
		err = modules_reset();
		if (err) {
			LOG_ERR("Unable to reset modules (err %d)", err);
			return err;
		}

		err = reset_in_progress_set(false);
		if (err) {
			LOG_ERR("Unable to clear reset in progress flag (err %d)", err);
			return err;
		}
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_storage_manager, SETTINGS_SM_SUBTREE_NAME, NULL,
			       fp_settings_set, fp_settings_commit, NULL);

int bt_fast_pair_factory_reset(void)
{
	int err;

	err = reset_in_progress_set(true);
	if (err) {
		return err;
	}

	modules_reset_prepare();

	err = modules_reset();
	if (err) {
		LOG_ERR("Unable to reset modules (err %d)", err);
		return err;
	}

	err = reset_in_progress_set(false);
	if (err) {
		return err;
	}

	return 0;
}

void fp_storage_manager_ram_clear(void)
{
	reset_in_progress = false;
	settings_set_err = 0;
}
