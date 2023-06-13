/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/printk.h>
#include <zephyr/storage/flash_map.h>

#include "psa_eits_backend.h"

LOG_MODULE_REGISTER(psa_eits_zephyr_settings, CONFIG_PSA_EITS_BACKEND_ZEPHYR_LOG_LEVEL);

#define ROOT_KEY     "EITS"
#define MAX_PATH_LEN 32

struct read_info_ctx {
	/* A container for metadata associated with a specific uid */
	struct psa_storage_info_t *info;

	/* Operation result. */
	psa_status_t status;
};

struct read_data_ctx {
	/* Buffer for the data associated with a specific uid */
	void *buff;

	/* Buffer length. */
	size_t buff_len;

	/* Operation result. */
	psa_status_t status;
};

static int read_info_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg,
			void *param)
{
	int ret;
	const char *next;
	size_t next_len;
	struct read_info_ctx *ctx = (struct read_info_ctx *)param;

	__ASSERT(ctx != NULL, "Improper value");
	__ASSERT(ctx->info != NULL, "Improper value");

	next_len = settings_name_next(name, &next);

	if (!strncmp(name, "data", next_len)) {
		ctx->info->size = len;
		LOG_DBG("read: <%s_.size> = %d", name, len);

		return 0;
	}

	if (!strncmp(name, "flags", next_len)) {
		ret = read_cb(cb_arg, &ctx->info->flags, sizeof(ctx->info->flags));
		if (ret <= 0) {
			LOG_ERR("Failed to read the setting, ret: %d", ret);
			ctx->status = PSA_ERROR_STORAGE_FAILURE;
		}
		ctx->status = PSA_SUCCESS;
		LOG_DBG("read: <%s> = %d", name, ctx->info->flags);

		return 0;
	}

	return -ENOENT;
}

static int read_data_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg,
			void *param)
{
	int ret;
	const char *next;
	size_t next_len;
	struct read_data_ctx *ctx = (struct read_data_ctx *)param;

	__ASSERT(ctx != NULL, "Improper value");
	__ASSERT(ctx->buff != NULL, "Improper value");

	next_len = settings_name_next(name, &next);
	if (next_len == 0) {
		if (len > ctx->buff_len) {
			ctx->status = PSA_ERROR_BUFFER_TOO_SMALL;
			return -ENOENT;
		}

		ret = read_cb(cb_arg, ctx->buff, len);
		if (ret <= 0) {
			LOG_ERR("Failed to read the setting, ret: %d", ret);
			ctx->status = PSA_ERROR_STORAGE_FAILURE;
		}
		ctx->status = PSA_SUCCESS;
		LOG_DBG("read: <%s/data>", name);

		return 0;
	}
	LOG_WRN("The setting: %s is not present in the storage.", name);

	/* The settings is not found, return error and stop processing */
	return -ENOENT;
}

static psa_status_t is_write_once_set(psa_storage_uid_t uid, bool *is_write_once)
{
	psa_status_t status;
	struct psa_storage_info_t info = {
		.size = 0,
		.flags = 0,
	};

	__ASSERT(is_write_once != NULL, "Improper value");

	status = psa_its_get_info(uid, &info);
	if (status != PSA_SUCCESS) {
		return status;
	}

	*is_write_once = info.flags & PSA_STORAGE_FLAG_WRITE_ONCE;

	return PSA_SUCCESS;
}

static void create_data_path(psa_storage_uid_t uid, char *buff, size_t buff_size)
{
	__ASSERT(buff != NULL, "Impropert value");

	int ret = snprintk(buff, buff_size, "%s/%" PRIx64 "/data", ROOT_KEY, uid);
	(void)ret;

	__ASSERT(ret < buff_size, "Buffer size is too small.");
}

static void create_flags_path(psa_storage_uid_t uid, char *buff, size_t buff_size)
{
	__ASSERT(buff != NULL, "Impropert value");

	int ret = snprintk(buff, buff_size, "%s/%" PRIx64 "/flags", ROOT_KEY, uid);
	(void)ret;

	__ASSERT(ret < buff_size, "Buffer size is too small.");
}

static psa_status_t read_data(psa_storage_uid_t uid, struct read_data_ctx *read_data_ctx)
{
	int ret;
	char path[MAX_PATH_LEN];

	create_data_path(uid, path, sizeof(path));

	ret = settings_load_subtree_direct(path, read_data_cb, read_data_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to load data for uid: %" PRIx64 ", ret %d", uid, ret);
		return PSA_ERROR_STORAGE_FAILURE;
	}

	return read_data_ctx->status;
}

static psa_status_t read_data_with_offset(psa_storage_uid_t uid, uint32_t data_offset,
					  uint32_t data_length, void *p_data, size_t *p_data_length,
					  struct read_data_ctx *read_data_ctx)
{
	uint8_t buff[CONFIG_PSA_EITS_READ_BUFF_SIZE];

	struct psa_storage_info_t info = {
		.size = 0,
		.flags = 0,
	};

	psa_status_t status = psa_its_get_info(uid, &info);

	if (status != PSA_SUCCESS) {
		return status;
	}

	if (data_offset >= info.size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (info.size - data_offset > data_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	__ASSERT(info.size <= CONFIG_PSA_EITS_READ_BUFF_SIZE,
		 "CONFIG_PSA_EITS_READ_BUFF_SIZE too small");

	read_data_ctx->buff = buff;
	read_data_ctx->buff_len = sizeof(buff);

	status = read_data(uid, read_data_ctx);
	if (status == PSA_SUCCESS) {
		*p_data_length = info.size - data_offset;
		memcpy(p_data, buff + data_offset, *p_data_length);
	}

	return status;
}

psa_status_t psa_its_set_backend(psa_storage_uid_t uid, uint32_t data_length, const void *p_data,
				 psa_storage_create_flags_t create_flags)
{
	char path[MAX_PATH_LEN];
	psa_status_t status;
	bool is_write_once;

	LOG_DBG("Setting uid: %" PRIx64, uid);

	/* Check that the arguments are valid */
	if (p_data == NULL || data_length == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/* Check that the create_flags does not contain any unsupported flags */
	if (create_flags & ~(PSA_STORAGE_FLAG_WRITE_ONCE)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = is_write_once_set(uid, &is_write_once);
	if (status != PSA_SUCCESS && status != PSA_ERROR_DOES_NOT_EXIST) {
		return status;
	}

	if (status == PSA_SUCCESS && is_write_once) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	/* Store flags associated with the stored data */
	create_flags_path(uid, path, sizeof(path));
	if (settings_save_one(path, (const void *)&create_flags, sizeof(create_flags))) {
		LOG_ERR("Failed to store flags for uid: %" PRIx64, uid);
		return PSA_ERROR_STORAGE_FAILURE;
	}

	/* Store data */
	create_data_path(uid, path, sizeof(path));
	if (settings_save_one(path, p_data, data_length)) {
		LOG_ERR("Failed to store data for uid: %" PRIx64, uid);
		return PSA_ERROR_STORAGE_FAILURE;
	}

	return PSA_SUCCESS;
}

psa_status_t psa_its_get_backend(psa_storage_uid_t uid, uint32_t data_offset, uint32_t data_length,
				 void *p_data, size_t *p_data_length)
{
	psa_status_t status;

	struct read_data_ctx read_data_ctx = {
		.buff = p_data,
		.buff_len = data_length,
		.status = PSA_ERROR_DOES_NOT_EXIST,
	};

	LOG_DBG("Getting data for uid: 0x%" PRIx64, uid);

	if (p_data == NULL || p_data_length == NULL || data_length == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_offset != 0) {
		return read_data_with_offset(uid, data_offset, data_length, p_data, p_data_length,
					     &read_data_ctx);
	} else {
		status = read_data(uid, &read_data_ctx);
		if (status == PSA_SUCCESS) {
			*p_data_length = data_length;
		}

		return status;
	}
}

psa_status_t psa_its_get_info_backend(psa_storage_uid_t uid, struct psa_storage_info_t *p_info)
{
	int ret;
	char path[MAX_PATH_LEN];
	struct read_info_ctx read_info_ctx = {
		.info = p_info,
		.status = PSA_ERROR_DOES_NOT_EXIST,
	};

	LOG_DBG("Getting info for uid: 0x%" PRIx64, uid);

	if (p_info == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	ret = snprintk(path, sizeof(path), "%s/%" PRIx64, ROOT_KEY, uid);
	(void)ret;
	__ASSERT(ret < sizeof(path), "Setting path buffer too small.");

	ret = settings_load_subtree_direct(path, read_info_cb, &read_info_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to load info for uid: %" PRIx64 ", ret %d", uid, ret);
		return PSA_ERROR_STORAGE_FAILURE;
	}

	return read_info_ctx.status;
}

psa_status_t psa_its_remove_backend(psa_storage_uid_t uid)
{
	psa_status_t status;
	bool is_write_once;
	char path[MAX_PATH_LEN];

	LOG_DBG("Removing data for uid: %" PRIx64, uid);

	status = is_write_once_set(uid, &is_write_once);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (is_write_once) {
		return PSA_ERROR_NOT_PERMITTED;
	}

	create_flags_path(uid, path, sizeof(path));
	if (settings_delete(path)) {
		return PSA_ERROR_STORAGE_FAILURE;
	}

	create_data_path(uid, path, sizeof(path));
	if (settings_delete(path)) {
		return PSA_ERROR_STORAGE_FAILURE;
	}

	return PSA_SUCCESS;
}

static int zephyr_settings_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret != 0) {
		LOG_ERR("settings_subsys_init failed (ret %d)", ret);
	}

	return ret;
}

SYS_INIT(zephyr_settings_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
