/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "storage_backend.h"

LOG_MODULE_REGISTER(internal_trusted_storage_settings, CONFIG_TRUSTED_STORAGE_LOG_LEVEL);

/* Storage pattern: prefix, uid low, uid high, suffix */
#define TRUSTED_STORAGE_SETTINGS_BACKEND_FILENAME_PATTERN "%s/%08x%08x"

/* Max filename length aligned with Settings File backend max length */
#define TRUSTED_STORAGE_SETTINGS_BACKEND_FILENAME_MAX_LENGTH 32

struct load_object_info {
	void *data;
	size_t size;
	int ret;
};

/* Helper to fill filename with a suffix */
static psa_status_t create_filename(char *filename, const size_t filename_size, const char *prefix,
				    const psa_storage_uid_t uid)
{
	int ret;

	ret = snprintf(filename, filename_size, TRUSTED_STORAGE_SETTINGS_BACKEND_FILENAME_PATTERN,
		       prefix, (unsigned int)((uid) >> 32), (unsigned int)((uid) & 0xffffffff));
	/* snprintf doc:
	 * Notice that only when this returned value is non-negative and less than n, the string has
	 * been completely written
	 */
	if (ret < 0 || ret >= filename_size) {
		return PSA_ERROR_STORAGE_FAILURE;
	}

	return PSA_SUCCESS;
}

/*
 * Reads the object content up to the size of object.
 */
static int storage_settings_load_object(const char *key, size_t len, settings_read_cb read_cb,
					void *cb_arg, void *param)
{
	struct load_object_info *info = param;

	info->ret = read_cb(cb_arg, info->data, MIN(info->size, len));

	/*
	 * This returned value isn't necessarily kept
	 * so also keep it in the load_object_info structure
	 */
	return info->ret;
}

static psa_status_t error_to_psa_error(int errorno)
{

	switch (errorno) {
	case 0:
		return PSA_SUCCESS;
	case -ENOSPC:
		return PSA_ERROR_INSUFFICIENT_STORAGE;
	case -ENOENT:
		return PSA_ERROR_DOES_NOT_EXIST;
	case -ENODATA:
		return PSA_ERROR_DATA_CORRUPT;
	default:
		return PSA_ERROR_STORAGE_FAILURE;
	}
}

psa_status_t storage_get_object(const psa_storage_uid_t uid, const char *prefix, void *object_data,
				const size_t object_size, size_t *object_length)
{
	char path[TRUSTED_STORAGE_SETTINGS_BACKEND_FILENAME_MAX_LENGTH + 1];
	struct load_object_info info;
	int ret;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (object_size == 0 || object_data == NULL || prefix == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = create_filename(path, TRUSTED_STORAGE_SETTINGS_BACKEND_FILENAME_MAX_LENGTH + 1,
				 prefix, uid);

	if (status != PSA_SUCCESS) {
		return status;
	}

	info.data = object_data;
	info.size = object_size;
	/* Set a fallback error if storage_settings_load_object isn't called */
	info.ret = -ENOENT;

	ret = settings_load_subtree_direct(path, storage_settings_load_object, &info);

	LOG_DBG("Get object with filename %s (max_size: %zd), ret: %d", path, object_size,
		info.ret);

	if (ret < 0) {
		return error_to_psa_error(ret);
	}

	if (info.ret < 0) {
		return error_to_psa_error(info.ret);
	}

	*object_length = info.ret;

	return PSA_SUCCESS;
}

psa_status_t storage_set_object(const psa_storage_uid_t uid, const char *prefix,
				const void *object_data, const size_t object_size)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	char path[TRUSTED_STORAGE_SETTINGS_BACKEND_FILENAME_MAX_LENGTH + 1];

	if (object_size == 0 || object_data == NULL || prefix == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = create_filename(path, TRUSTED_STORAGE_SETTINGS_BACKEND_FILENAME_MAX_LENGTH + 1,
				 prefix, uid);

	LOG_DBG("Set object with filename %s. Size: %zd", path, object_size);

	if (status != PSA_SUCCESS) {
		return status;
	}

	return error_to_psa_error(settings_save_one(path, object_data, object_size));
}

psa_status_t storage_remove_object(const psa_storage_uid_t uid, const char *prefix)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	char path[TRUSTED_STORAGE_SETTINGS_BACKEND_FILENAME_MAX_LENGTH + 1];

	if (prefix == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = create_filename(path, TRUSTED_STORAGE_SETTINGS_BACKEND_FILENAME_MAX_LENGTH + 1,
				 prefix, uid);

	if (status != PSA_SUCCESS) {
		return status;
	}

	status = error_to_psa_error(settings_delete(path));

	LOG_DBG("Remove object with filename: %s, status %d", path, status);

	return status;
}
