/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>

#include "esl.h"

LOG_MODULE_DECLARE(peripheral_esl);

/* Matches LFS_NAME_MAX */
#define MAX_PATH_LEN 255

#define PARTITION_NODE DT_NODELABEL(lfs1)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
struct fs_mount_t *mp =
#if DT_NODE_EXISTS(PARTITION_NODE)
	&FS_FSTAB_ENTRY(PARTITION_NODE)
#else
	&lfs_storage_mnt
#endif
	;
int write_img_to_storage(uint8_t img_idx, size_t len, off_t offset)
{
	struct fs_file_t file;
	int rc;
	char fname[MAX_PATH_LEN];
	struct bt_esls *esl_obj = esl_get_esl_obj();

	LOG_DBG("%s len %d offset %ld", __func__, len, offset);
	fs_file_t_init(&file);
	snprintf(fname, sizeof(fname), OBJECT_IDX_FULLNAME_TEMPLETE, mp->mnt_point, img_idx);
	rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);

	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d\n", fname, rc);
		goto out;
	}

	rc = fs_seek(&file, offset, FS_SEEK_SET);
	if (rc < 0) {
		LOG_ERR("seek to offset %ld failed", offset);
		goto out;
	}

	rc = fs_write(&file, esl_obj->img_obj_buf, len);
	if (rc < 0) {
		LOG_ERR("fs_write len %d failed %d", len, rc);
		goto out;
	}
out:
	fs_close(&file);
	return rc;
}

int read_img_from_storage(uint8_t img_idx, void *data, size_t len, off_t offset)
{
	struct fs_file_t file;
	int rc;
	char fname[MAX_PATH_LEN];

	LOG_DBG("%s len %d offset %ld", __func__, len, offset);
	fs_file_t_init(&file);
	snprintf(fname, sizeof(fname), OBJECT_IDX_FULLNAME_TEMPLETE, mp->mnt_point, img_idx);
	rc = fs_open(&file, fname, FS_O_READ);
	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d\n", fname, rc);
		goto out;
	}

	rc = fs_seek(&file, offset, FS_SEEK_SET);
	if (rc < 0) {
		LOG_ERR("seek to offset %ld failed", offset);
		goto out;
	}

	rc = fs_read(&file, data, len);
	if (rc < 0) {
		LOG_ERR("fs_read len %d failed %d", len, rc);
		goto out;
	}
out:
	fs_close(&file);
	return rc;
}

size_t read_img_size_from_storage(uint8_t img_idx)
{
	size_t size;
	struct fs_file_t file;
	struct fs_dirent dirent;

	int rc;
	char fname[MAX_PATH_LEN];

	fs_file_t_init(&file);
	snprintf(fname, sizeof(fname), OBJECT_IDX_FULLNAME_TEMPLETE, mp->mnt_point, img_idx);
	rc = fs_stat(fname, &dirent);
	LOG_DBG("%s fs_stat %d", fname, rc);
	if (!rc) {
		size = dirent.size;
	} else {
		size = 0;
	}

	LOG_DBG("img %d in Flash size %d", img_idx, size);
	return size;
}

int16_t delete_imgs_from_storage(void)
{
	struct fs_file_t file;
	int rc;
	char fname[MAX_PATH_LEN];

	LOG_DBG("%s", __func__);
	for (uint8_t img_idx = 0; img_idx < CONFIG_BT_ESL_IMAGE_MAX; img_idx++) {
		fs_file_t_init(&file);
		snprintf(fname, sizeof(fname), OBJECT_IDX_FULLNAME_TEMPLETE, mp->mnt_point,
			 img_idx);
		rc = fs_unlink(fname);
		if (rc) {
			LOG_ERR("fs_unlink delete %s failed", fname);
		}
	}

	return 0;
}

int ots_storage_init(void)
{
	char fname[MAX_PATH_LEN];
	int rc;

	/* Do not mount if auto-mount has been enabled */
#if !DT_NODE_EXISTS(PARTITION_NODE) ||                                                             \
	!(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
	unsigned int id = (uintptr_t)mp->storage_dev;

	rc = fs_mount(mp);
	if (rc < 0) {
		LOG_ERR("FAIL: mount id %" PRIuPTR " at %s: %d\n", (uintptr_t)mp->storage_dev,
			mp->mnt_point, rc);
		return rc;
	}

	LOG_DBG("%s mount: %d\n", mp->mnt_point, rc);
#else
	LOG_DBG("%s automounted\n", mp->mnt_point);
#endif
	struct fs_dirent dirent;
	struct fs_file_t file;

	fs_file_t_init(&file);
	snprintf(fname, sizeof(fname), "%s", mp->mnt_point);

	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_IMAGE_MAX; idx++) {
		snprintf(fname, sizeof(fname), OBJECT_IDX_FULLNAME_TEMPLETE, mp->mnt_point, idx);
		rc = fs_stat(fname, &dirent);
		LOG_DBG("%s fs_stat %d\n", fname, rc);
		if (rc == -ENOENT) {
			LOG_DBG("%s not exist ,create it", fname);
			rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
			if (rc < 0) {
				LOG_ERR("FAIL: open %s: %d\n", fname, rc);
			}

			esl_obj->stored_image_size[idx] = 0;
		} else {
			esl_obj->stored_image_size[idx] = dirent.size;
		}

		rc = fs_close(&file);
	}

	return rc;
}
