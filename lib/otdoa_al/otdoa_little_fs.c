/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#if CONFIG_OTDOA_LITTLE_FS

#include <stdio.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include "zephyr/logging/log.h"

LOG_MODULE_DECLARE(otdoa_al);

/* Matches LFS_NAME_MAX */
#define MAX_PATH_LEN 255

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
#if CONFIG_OTDOA_EXTERNAL_STORAGE
	.storage_dev = (void *)FLASH_AREA_ID(external_flash),
#else
	.storage_dev = (void *)FLASH_AREA_ID(littlefs_storage),
#endif
	.mnt_point = "/lfs",
};

/*
 * Mount the LittleFS file system
 */
int mount_little_fs(void)
{
	struct fs_mount_t *mp = &lfs_storage_mnt;
	unsigned int id = (uintptr_t)mp->storage_dev;
	struct fs_statvfs sbuf;
	const struct flash_area *pfa;
	int rc;

	rc = flash_area_open(id, &pfa);
	if (rc < 0) {
		LOG_ERR("FAIL: unable to find flash area %u: %d", id, rc);
		return rc;
	}
	LOG_INF("Area %u at 0x%x on %s for %u bytes", id, (unsigned int)pfa->fa_off,
		pfa->fa_dev->name, (unsigned int)pfa->fa_size);

	flash_area_close(pfa);

	rc = fs_mount(mp);
	if (rc < 0) {
		LOG_ERR("FAIL: mount id %u at %s: %d", (unsigned int)mp->storage_dev, mp->mnt_point,
			rc);
		return rc;
	}
	LOG_INF("%s mount: %d", mp->mnt_point, rc);

	rc = fs_statvfs(mp->mnt_point, &sbuf);
	if (rc < 0) {
		LOG_ERR("FAIL: statvfs: %d", rc);
		int rc1 = fs_unmount(mp);

		LOG_INF("%s unmount: %d", mp->mnt_point, rc1);
		return rc;
	}

	LOG_DBG("%s: bsize = %lu ; frsize = %lu ;"
		" blocks = %lu ; bfree = %lu",
		mp->mnt_point, sbuf.f_bsize, sbuf.f_frsize, sbuf.f_blocks, sbuf.f_bfree);
	return 0;
}

/*
 * Unmount the LittleFS File System
 */
int unmount_little_fs(void)
{
	struct fs_mount_t *mp = &lfs_storage_mnt;
	int rc = fs_unmount(mp);

	printk("%s unmount: %d\n", mp->mnt_point, rc);

	return rc;
}

/*
 * Test the LittleFS file system
 */
int test_little_fs(void)
{
	struct fs_mount_t *mp = &lfs_storage_mnt;
	struct fs_dirent dirent;
	char fname[MAX_PATH_LEN];

	snprintf(fname, sizeof(fname), "%s/boot_count", mp->mnt_point);

	int rc = fs_stat(fname, &dirent);

	printk("%s stat: %d\n", fname, rc);
	if (rc >= 0) {
		printk("\tfn '%s' siz %u\n", dirent.name, dirent.size);
	}

	struct fs_file_t file = {0};

	k_sleep(K_MSEC(100));

	rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (rc < 0) {
		printk("FAIL: open %s: %d\n", fname, rc);
		return rc;
	}

	uint32_t boot_count = 0;

	if (rc >= 0) {
		rc = fs_read(&file, &boot_count, sizeof(boot_count));
		printk("%s read count %u: %d\n", fname, boot_count, rc);
		rc = fs_seek(&file, 0, FS_SEEK_SET);
		printk("%s seek start: %d\n", fname, rc);
	}

	boot_count += 1;
	rc = fs_write(&file, &boot_count, sizeof(boot_count));
	printk("%s write new boot count %u: %d\n", fname, boot_count, rc);

	rc = fs_close(&file);
	printk("%s close: %d\n", fname, rc);

	return 0;
}

#endif /* OTDOA_LITTLE_FS == 0 */
