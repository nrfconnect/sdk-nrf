/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include "esl.h"

LOG_MODULE_DECLARE(peripheral_esl);

#define IMG_SIZE_OFFSET 0x100
static struct nvs_fs *fs;

int write_img_to_storage(uint8_t img_idx, size_t len, off_t offset)
{
	int rc;
	struct bt_esls *esl_obj = esl_get_esl_obj();

	ARG_UNUSED(offset);
	LOG_DBG("%s img %d len %d offset %ld", __func__, img_idx, len, offset);
	rc = nvs_write(fs, img_idx, esl_obj->img_obj_buf, len);
	if (rc < 0) {
		LOG_ERR("nvs_write img content len %d failed %d", len, rc);
		return rc;
	}

	rc = nvs_write(fs, img_idx + IMG_SIZE_OFFSET, &len, sizeof(size_t));
	LOG_DBG("nvs_write img size rc %d", rc);

	if (rc < 0) {
		LOG_ERR("nvs_write img size failed %d", rc);
	}

	return rc;
}

int read_img_from_storage(uint8_t img_idx, void *data, size_t len, off_t offset)
{
	int rc;

	if (offset > len) {
		LOG_ERR("offset is out of range");
		return -EINVAL;
	}

	LOG_DBG("%s img %d len %d offset %ld", __func__, img_idx, len, offset);

	rc = nvs_read(fs, img_idx, data, len);
	if (rc < 0) {
		LOG_ERR("nvs_read len %d failed %d", len, rc);
	}

	return rc;
}

size_t read_img_size_from_storage(uint8_t img_idx)
{
	size_t size;
	int rc;

	rc = nvs_read(fs, img_idx + IMG_SIZE_OFFSET, &size, sizeof(size_t));
	LOG_DBG("nvs_read img_size %d from nvs id %d rc %d", img_idx, (img_idx + IMG_SIZE_OFFSET),
		rc);
	if (rc < 0) {
		LOG_ERR("size %d", size);
		size = 0;
	}

	LOG_DBG("img %d in Flash size %d", img_idx, size);
	return size;
}

int delete_imgs_from_storage(void)
{
	int rc;

	LOG_DBG("%s", __func__);
	for (size_t img_idx = 0; img_idx < CONFIG_BT_ESL_IMAGE_MAX; img_idx++) {
		rc = nvs_delete(fs, img_idx);
		if (rc) {
			LOG_ERR("nvs_delete idx %d failed", img_idx);
			return rc;
		}

		rc = nvs_delete(fs, img_idx + IMG_SIZE_OFFSET);
		if (rc) {
			LOG_ERR("nvs_delete idx %d failed", (img_idx + IMG_SIZE_OFFSET));
			return rc;
		}
	}

	return 0;
}

int ots_storage_init(void)
{
	int rc;

	/* Image shared with settings_storage */
	rc = settings_storage_get((void **)&fs);
	if (rc) {
		LOG_ERR("settings_storage_get failed (rc %d)", rc);
		return -EINVAL;
	}

	return 0;
}
