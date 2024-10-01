/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <modules/sd_card.h>

#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

#define MKFS_FS_TYPE FS_FATFS
#define MKFS_DEV_ID  "SD:"
#define MKFS_FLAGS   0

static int test_create_file(const char *path)
{
	int ret;
	struct fs_file_t file;

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (ret == 0) {
		fs_write(&file, "Testfile", 8);
		fs_close(&file);
		TC_PRINT("File written successfully\n");
	} else {
		TC_PRINT("Failed to open file\n");
		return ret;
	}

	return 0;
}

static int test_disk_populate(void)
{
	TC_PRINT("Writing files to ramdisk\n");

	int ret;

	ret = test_create_file("/SD:/testroot.lc3");
	zassert_equal(0, ret, "test_create_file should return 0, %u", ret);

	ret = fs_mkdir("/SD:/test");
	zassert_equal(0, ret, "fs_mkdir should return 0, %u", ret);

	ret = test_create_file("/SD:/test/test1.lc3");
	zassert_equal(0, ret, "test_create_file should return 0, %u", ret);

	ret = test_create_file("/SD:/test/test2.lc3");
	zassert_equal(0, ret, "test_create_file should return 0, %u", ret);

	/* ret = fs_unlink("/SD:/test/test2.lc3");
	 * zassert_equal(0, ret, "fs_unlink should return 0, %u", ret);
	 */

	TC_PRINT("Done writing files\n");
	return 0;
}

static void *suite_setup(void)
{
	TC_PRINT("Suite setup. Formatting RAM disk\n");

	int ret;

	ret = fs_mkfs(MKFS_FS_TYPE, (uintptr_t)MKFS_DEV_ID, NULL, MKFS_FLAGS);

	if (ret < 0) {
		TC_PRINT("Format failed %d\n", ret);
		zassert_equal(0, ret, "fs_mkfs should return 0, %u", ret);
		return NULL;
	}

	TC_PRINT("Format successful\n");

	return NULL;
}

#define SD_FILECOUNT_MAX 420
#define SD_PATHLEN_MAX	 190

ZTEST(sd_card, test_init)
{
	int ret;

	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};

	ret = sd_card_init();
	zassert_equal(0, ret, "sd_card_init() should return 0, %u", ret);

	ret = test_disk_populate();
	zassert_equal(0, ret, "test_disk_populate should return 0, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".lc3");

	if (ret < 0) {
		zassert_equal(0, ret, "sd_card_list_files_match returned error, %u", ret);
	}

	TC_PRINT("Files found: %u\n", ret);
}

ZTEST_SUITE(sd_card, NULL, suite_setup, NULL, NULL, NULL);
