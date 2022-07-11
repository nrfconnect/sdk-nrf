/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/settings/settings.h>

#include "fp_storage_pn.h"

#define SETTINGS_PN_SUBTREE_NAME "fp_pn"
#define SETTINGS_PN_KEY_NAME "pn"
#define SETTINGS_NAME_SEPARATOR_STR "/"
#define SETTINGS_PN_FULL_NAME \
	(SETTINGS_PN_SUBTREE_NAME SETTINGS_NAME_SEPARATOR_STR SETTINGS_PN_KEY_NAME)

BUILD_ASSERT(SETTINGS_NAME_SEPARATOR == '/');

static char personalized_name[FP_STORAGE_PN_BUF_LEN];

static int settings_set_err;
static atomic_t settings_loaded = ATOMIC_INIT(false);


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
	 * The error will then be propagated by the commit callback.
	 * Errors returned in the settings set callback are not propagated further.
	 */
	if (err && !settings_set_err) {
		settings_set_err = err;
	}

	return err;
}

static int fp_settings_commit(void)
{
	if (settings_set_err) {
		return settings_set_err;
	}

	atomic_set(&settings_loaded, true);

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(fast_pair_storage_pn, SETTINGS_PN_SUBTREE_NAME, NULL,
			       fp_settings_set, fp_settings_commit, NULL);

int fp_storage_pn_get(char *buf)
{
	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
	}

	strcpy(buf, personalized_name);

	return 0;
}

int fp_storage_pn_save(const char *pn_to_save)
{
	int err;
	size_t str_len;

	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
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
