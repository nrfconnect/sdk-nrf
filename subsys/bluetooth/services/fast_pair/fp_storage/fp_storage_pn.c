/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_storage, CONFIG_FP_STORAGE_LOG_LEVEL);

#include "fp_storage_pn.h"
#include "fp_storage_pn_priv.h"
#include "fp_storage_manager.h"

static char personalized_name[FP_STORAGE_PN_BUF_LEN];

static int settings_set_err;
static bool is_enabled;


static int fp_settings_load_pn(size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;

	if (len > sizeof(personalized_name)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, personalized_name, len);
	if (rc < 0) {
		return rc;
	}

	if (rc != len) {
		return -EINVAL;
	}

	if (strnlen(personalized_name, len) != (len - 1)) {
		return -EINVAL;
	}

	return 0;
}

static int fp_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int err = 0;
	const char *key = SETTINGS_PN_KEY_NAME;

	if (!strncmp(name, key, sizeof(SETTINGS_PN_KEY_NAME))) {
		err = fp_settings_load_pn(len, read_cb, cb_arg);
	} else {
		err = -ENOENT;
	}

	/* The first reported settings set error will be remembered by the module.
	 * The error will then be returned when calling fp_storage_pn_init.
	 */
	if (err && !settings_set_err) {
		settings_set_err = err;
	}

	return 0;
}

int fp_storage_pn_get(char *buf)
{
	if (!is_enabled) {
		return -EACCES;
	}

	strcpy(buf, personalized_name);

	return 0;
}

int fp_storage_pn_save(const char *pn_to_save)
{
	int err;
	size_t str_len;

	if (!is_enabled) {
		return -EACCES;
	}

	str_len = strnlen(pn_to_save, FP_STORAGE_PN_BUF_LEN);
	if (str_len == FP_STORAGE_PN_BUF_LEN) {
		return -ENOMEM;
	}

	err = settings_save_one(SETTINGS_PN_FULL_NAME, pn_to_save, str_len + 1);
	if (err) {
		return err;
	}

	strcpy(personalized_name, pn_to_save);

	return 0;
}

static int fp_storage_pn_init(void)
{
	if (is_enabled) {
		LOG_WRN("fp_storage_pn module already initialized");
		return 0;
	}

	if (settings_set_err) {
		return settings_set_err;
	}

	is_enabled = true;

	return 0;
}

static int fp_storage_pn_uninit(void)
{
	is_enabled = false;
	return 0;
}

void fp_storage_pn_ram_clear(void)
{
	memset(personalized_name, 0, sizeof(personalized_name));

	settings_set_err = 0;

	is_enabled = false;
}

static int fp_storage_pn_reset(void)
{
	int err;
	bool was_enabled = is_enabled;

	err = settings_delete(SETTINGS_PN_FULL_NAME);
	if (err) {
		return err;
	}

	fp_storage_pn_ram_clear();
	if (was_enabled) {
		err = fp_storage_pn_init();
		if (err) {
			return err;
		}
	}

	return 0;
}

static void reset_prepare(void)
{
	/* intentionally left empty */
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_storage_pn, SETTINGS_PN_SUBTREE_NAME, NULL, fp_settings_set,
			       NULL, NULL);

FP_STORAGE_MANAGER_MODULE_REGISTER(fp_storage_pn, fp_storage_pn_reset, reset_prepare,
				   fp_storage_pn_init, fp_storage_pn_uninit);
