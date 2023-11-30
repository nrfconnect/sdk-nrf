/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>

#include "fp_storage.h"

#include "fp_storage_manager.h"
#include "fp_storage_manager_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_storage, CONFIG_FP_STORAGE_LOG_LEVEL);

static bool reset_in_progress;
static int settings_set_err;
static bool is_enabled;
static atomic_t settings_loaded = ATOMIC_INIT(false);

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

	/* The first reported settings set error will be remembered by the module.
	 * The error will then be returned when calling fp_storage_init.
	 */
	if (err && !settings_set_err) {
		settings_set_err = err;
	}

	return 0;
}

static int reset_in_progress_set(bool in_progress)
{
	int err;

	if (in_progress) {
		err = settings_save_one(SETTINGS_RESET_IN_PROGRESS_FULL_NAME, &in_progress,
					sizeof(in_progress));
	} else {
		err = settings_delete(SETTINGS_RESET_IN_PROGRESS_FULL_NAME);
	}

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
	/* Potential error will be reported when calling fp_storage_init. */
	atomic_set(&settings_loaded, true);

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_storage_manager, SETTINGS_SM_SUBTREE_NAME, NULL,
			       fp_settings_set, fp_settings_commit, NULL);

int fp_storage_factory_reset(void)
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
	atomic_set(&settings_loaded, false);

	is_enabled = false;
}

bool fp_storage_settings_loaded(void)
{
	return atomic_get(&settings_loaded);
}

static int modules_init(void)
{
	int err = 0;
	size_t module_idx = 0;

	STRUCT_SECTION_FOREACH(fp_storage_manager_module, module) {
		if (module->module_init) {
			err = module->module_init();
			if (err) {
				break;
			}
		}

		module_idx++;
	}

	if (err) {
		for (int i = module_idx; i >= 0; i--) {
			struct fp_storage_manager_module *module;

			STRUCT_SECTION_GET(fp_storage_manager_module, i, &module);

			if (module->module_uninit) {
				(void)module->module_uninit();
			}
		}
	}

	return err;
}

static int modules_uninit(void)
{
	int first_err = 0;
	int module_count;

	STRUCT_SECTION_COUNT(fp_storage_manager_module, &module_count);

	for (int i = module_count - 1; i >= 0; i--) {
		int err;
		struct fp_storage_manager_module *module;

		STRUCT_SECTION_GET(fp_storage_manager_module, i, &module);
		if (module->module_uninit) {
			err = module->module_uninit();
			if (err && !first_err) {
				first_err = err;
			}
		}
	}

	return first_err;
}

int fp_storage_init(void)
{
	int err;

	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
	}

	if (is_enabled) {
		LOG_WRN("fp_storage_manager module already initialized");
		return 0;
	}

	if (settings_set_err) {
		return settings_set_err;
	}

	if (reset_in_progress) {
		LOG_WRN("Fast Pair factory reset has been interrupted or aborted. Retrying");
		err = fp_storage_factory_reset();
		if (err) {
			return err;
		}
	}

	err = modules_init();
	if (err) {
		return err;
	}

	is_enabled = true;

	return 0;
}

int fp_storage_uninit(void)
{
	is_enabled = false;

	return modules_uninit();
}
