/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Provides minimal implementation of fp_storage_ak.h header that allows for storing only one
 * Account Key, that is assumed to be an Owner Account Key. Either this file or fp_storage_ak.c file
 * (but not both) should be compiled to be able to use fp_storage_ak.h API.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/settings/settings.h>

#include "fp_common.h"
#include "fp_storage_ak.h"
#include "fp_storage_ak_priv.h"
#include "fp_storage_manager.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_storage, CONFIG_FP_STORAGE_LOG_LEVEL);

static struct fp_account_key owner_account_key;
static bool owner_account_key_is_stored;

static int settings_set_err;
static bool is_enabled;

static int fp_settings_load_owner_ak(size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;
	struct account_key_data data;

	if (len != sizeof(data)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &data, sizeof(data));
	if (rc < 0) {
		return rc;
	}

	if (rc != sizeof(data)) {
		return -EINVAL;
	}

	if (ACCOUNT_KEY_METADATA_FIELD_GET(data.account_key_metadata, ID) != ACCOUNT_KEY_MIN_ID) {
		return -EINVAL;
	}

	if (!ACCOUNT_KEY_METADATA_FIELD_GET(data.account_key_metadata, OWNER)) {
		return -EINVAL;
	}

	if (owner_account_key_is_stored) {
		return -EALREADY;
	}

	owner_account_key = data.account_key;
	owner_account_key_is_stored = true;

	return 0;
}

static int fp_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int err = 0;

	if (!strncmp(name, SETTINGS_OWNER_AK_NAME, sizeof(SETTINGS_OWNER_AK_NAME))) {
		err = fp_settings_load_owner_ak(len, read_cb, cb_arg);
	} else {
		err = -ENOENT;
	}

	/* The first reported settings set error will be remembered by the module.
	 * The error will then be returned when calling fp_storage_ak_init.
	 */
	if (err && !settings_set_err) {
		settings_set_err = err;
	}

	return 0;
}

int fp_storage_ak_count(void)
{
	if (!is_enabled) {
		return -EACCES;
	}

	return owner_account_key_is_stored ? 1 : 0;
}

int fp_storage_ak_get(struct fp_account_key *buf, size_t *key_count)
{
	int ret;

	ret = fp_storage_ak_count();
	if (ret < 0) {
		return ret;
	}

	if (*key_count < ret) {
		return -EINVAL;
	}

	*key_count = ret;

	if (ret == 0) {
		return 0;
	}

	*buf = owner_account_key;

	return 0;
}

int fp_storage_ak_find(struct fp_account_key *account_key,
		       fp_storage_ak_check_cb account_key_check_cb, void *context)
{
	if (!is_enabled) {
		return -EACCES;
	}

	if (!account_key_check_cb) {
		return -EINVAL;
	}

	if (owner_account_key_is_stored) {
		if (account_key_check_cb(&owner_account_key, context)) {
			if (account_key) {
				*account_key = owner_account_key;
			}

			return 0;
		}
	}

	return -ESRCH;
}

int fp_storage_ak_save(const struct fp_account_key *account_key)
{
	int err;
	struct account_key_data data;

	if (!is_enabled) {
		return -EACCES;
	}

	if (owner_account_key_is_stored) {
		return -EALREADY;
	}

	data.account_key_metadata = 0;
	ACCOUNT_KEY_METADATA_FIELD_SET(data.account_key_metadata, ID, ACCOUNT_KEY_MIN_ID);
	ACCOUNT_KEY_METADATA_FIELD_SET(data.account_key_metadata, OWNER, 1);
	data.account_key = *account_key;

	err = settings_save_one(SETTINGS_OWNER_AK_FULL_NAME, &data, sizeof(data));
	if (err) {
		return err;
	}

	owner_account_key = *account_key;
	owner_account_key_is_stored = true;

	return 0;
}

int fp_storage_ak_is_owner(const struct fp_account_key *account_key)
{
	if (!is_enabled) {
		return -EACCES;
	}

	if (!owner_account_key_is_stored) {
		return -ESRCH;
	}

	return !memcmp(account_key->key, owner_account_key.key, sizeof(owner_account_key.key)) ?
	       1 : 0;
}

void fp_storage_ak_ram_clear(void)
{
	memset(&owner_account_key, 0, sizeof(owner_account_key));
	owner_account_key_is_stored = false;

	settings_set_err = 0;

	is_enabled = false;
}

bool fp_storage_ak_has_account_key(void)
{
	return (fp_storage_ak_count() > 0);
}

static int fp_storage_ak_init(void)
{
	if (is_enabled) {
		LOG_WRN("FP Storage AK: module already initialized");
		return 0;
	}

	if (settings_set_err) {
		return settings_set_err;
	}

	is_enabled = true;

	return 0;
}

static int fp_storage_ak_uninit(void)
{
	is_enabled = false;

	return 0;
}

static int fp_storage_owner_ak_delete(void)
{
	return settings_delete(SETTINGS_OWNER_AK_FULL_NAME);
}

static int fp_storage_ak_reset(void)
{
	int err;
	bool was_enabled = is_enabled;

	err = fp_storage_owner_ak_delete();
	if (err) {
		return err;
	}

	fp_storage_ak_ram_clear();
	if (was_enabled) {
		err = fp_storage_ak_init();
		if (err) {
			return err;
		}
	}

	return 0;
}

static void reset_prepare_set(void)
{
	/* intentionally left empty */
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_storage_ak_minimal,
			       SETTINGS_AK_SUBTREE_NAME,
			       NULL,
			       fp_settings_set,
			       NULL,
			       NULL);

FP_STORAGE_MANAGER_MODULE_REGISTER(fp_storage_ak_minimal,
				   fp_storage_ak_reset,
				   reset_prepare_set,
				   fp_storage_ak_init,
				   fp_storage_ak_uninit);
