/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/settings/settings.h>

#include "fp_storage_eik.h"
#include "fp_storage_eik_priv.h"
#include "fp_storage_manager.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_storage, CONFIG_FP_STORAGE_LOG_LEVEL);

static bool provisioned;
static uint8_t ephemeral_identity_key[FP_STORAGE_EIK_LEN];

static int settings_rc;
static bool is_enabled;

int fp_storage_eik_save(const uint8_t *eik)
{
	int err;

	if (!is_enabled) {
		return -EACCES;
	}

	err = settings_save_one(SETTINGS_EIK_FULL_NAME,
				eik,
				FP_STORAGE_EIK_LEN);
	if (err) {
		LOG_ERR("FP Storage EIK: settings_save_one failed: %d", err);
		return err;
	}

	memcpy(ephemeral_identity_key, eik, sizeof(ephemeral_identity_key));
	provisioned = true;

	return 0;
}

int fp_storage_eik_delete(void)
{
	int err;

	if (!is_enabled) {
		return -EACCES;
	}

	err = settings_delete(SETTINGS_EIK_FULL_NAME);
	if (err) {
		LOG_ERR("FP Storage EIK: settings_delete failed: %d", err);
		return err;
	}

	memset(ephemeral_identity_key, 0, sizeof(ephemeral_identity_key));
	provisioned = false;

	return 0;
}

int fp_storage_eik_is_provisioned(void)
{
	if (!is_enabled) {
		return -EACCES;
	}

	return provisioned ? 1 : 0;
}

int fp_storage_eik_get(uint8_t *eik)
{
	if (!is_enabled) {
		return -EACCES;
	}

	if (!provisioned) {
		return -ENODATA;
	}

	memcpy(eik, ephemeral_identity_key, FP_STORAGE_EIK_LEN);

	return 0;
}

static int fp_settings_eik_load(size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;

	if (len != sizeof(ephemeral_identity_key)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, ephemeral_identity_key, sizeof(ephemeral_identity_key));
	if (rc < 0) {
		return rc;
	}

	if (rc != sizeof(ephemeral_identity_key)) {
		return -EINVAL;
	}

	provisioned = true;

	return 0;
}

static int fp_storage_eik_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int err;

	LOG_DBG("FP Storage EIK: the EIK node is being set by Settings");

	if (!strncmp(name, SETTINGS_EIK_KEY_NAME, sizeof(SETTINGS_EIK_KEY_NAME))) {
		err = fp_settings_eik_load(len, read_cb, cb_arg);
	} else {
		err = -ENOENT;
	}

	/* The first reported settings set error will be remembered by the module.
	 * The error will then be returned when calling fp_storage_eik_init.
	 */
	if (err && !settings_rc) {
		settings_rc = err;
	}

	return 0;
}

static int fp_storage_eik_init(void)
{
	if (is_enabled) {
		LOG_WRN("FP Storage EIK: module already initialized");
		return 0;
	}

	if (settings_rc) {
		return settings_rc;
	}

	is_enabled = true;

	return 0;
}

static int fp_storage_eik_uninit(void)
{
	is_enabled = false;

	return 0;
}

void fp_storage_eik_ram_clear(void)
{
	memset(ephemeral_identity_key, 0, sizeof(ephemeral_identity_key));
	provisioned = false;

	settings_rc = 0;

	is_enabled = false;
}

static int fp_storage_eik_reset_perform(void)
{
	int err;
	bool was_enabled = is_enabled;

	err = settings_delete(SETTINGS_EIK_FULL_NAME);
	if (err) {
		LOG_ERR("FP Storage EIK: settings_delete failed: %d", err);
		return err;
	}

	fp_storage_eik_ram_clear();
	if (was_enabled) {
		err = fp_storage_eik_init();
		if (err) {
			return err;
		}
	}

	return 0;
}

static void fp_storage_eik_reset_prepare(void)
{
	/* intentionally left empty */
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_storage_eik,
			       SETTINGS_EIK_SUBTREE_NAME,
			       NULL,
			       fp_storage_eik_set,
			       NULL,
			       NULL);

FP_STORAGE_MANAGER_MODULE_REGISTER(fp_storage_eik,
				   fp_storage_eik_reset_perform,
				   fp_storage_eik_reset_prepare,
				   fp_storage_eik_init,
				   fp_storage_eik_uninit);
