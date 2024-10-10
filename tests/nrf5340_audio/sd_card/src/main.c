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

#define MKFS_DEV_ID "SD:"

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

static int test_disk_populate(uint8_t files_on_root, uint8_t num_levels, uint8_t files_per_folder,
			      uint8_t folders_per_level, char const *const extension)
{
	TC_PRINT("Writing files to ramdisk\n");

	int ret;
	int files_added = 0;
	char path[1000];

	for (int i = 0; i < files_on_root; i++) {
		sprintf(path, "/SD:/file%d", i);
		strcat(path, extension);
		ret = test_create_file(path);
		zassert_equal(0, ret, "test_create_file should return 0, %u", ret);
		files_added++;
	}

	for (int i = 0; i < num_levels; i++) {
		for (int j = 0; j < folders_per_level; j++) {
			sprintf(path, "/SD:/level%d/folder%d", i, j);
			ret = fs_mkdir(path);
			if (ret != -EEXIST) {
				zassert_equal(0, ret, "fs_mkdir should return 0, %u", ret);
			}

			for (int k = 0; k < files_per_folder; k++) {
				sprintf(path, "/SD:/level%d/folder%d/file%d", i, j, k);
				strcat(path, extension);
				ret = test_create_file(path);
				zassert_equal(0, ret, "test_create_file should return 0, %u", ret);
				files_added++;
			}
		}
	}

	TC_PRINT("Done writing %d files\n", files_added);
	return files_added;
}

static void *setup_fn(void)
{
	int ret;

	ret = sd_card_init();
	zassert_equal(0, ret, "sd_card_init() should return 0, %u", ret);

	return NULL;
}

static void before_fn(void *fixture)
{
	TC_PRINT("Suite setup. Formatting RAM disk\n");

	int ret;

	ret = fs_mkfs(FS_FATFS, (uintptr_t)MKFS_DEV_ID, NULL, 0);

	if (ret < 0) {
		TC_PRINT("Format failed %d\n", ret);
		zassert_equal(0, ret, "fs_mkfs should return 0, %u", ret);
		return;
	}

	TC_PRINT("Format successful\n");

	return;
}

#define SD_FILECOUNT_MAX 420
#define SD_PATHLEN_MAX	 190

ZTEST(sd_card, test_basic_files)
{
	int ret;
	static const uint8_t FILES_ON_ROOT = 2;

	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};

	ret = test_disk_populate(FILES_ON_ROOT, 0, 0, 0, ".lc3");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".lc3");

	if (ret < 0) {
		zassert_equal(0, ret, "sd_card_list_files_match returned error, %u", ret);
	}

	TC_PRINT("Files found: %u\n", ret);

	zassert_equal(FILES_ON_ROOT, ret, "Num files found mismatch, %u", ret);
	zassert_str_equal("file0.lc3", sd_paths_and_files[0], "File mismatch: %s",
			  sd_paths_and_files[0]);
	zassert_str_equal("file1.lc3", sd_paths_and_files[1], "File mismatch: %s",
			  sd_paths_and_files[1]);
}

/* Same as test_basic_files. Checks that functions are reentrant */
ZTEST(sd_card, test_basic_files_again)
{
	int ret;
	static const uint8_t FILES_ON_ROOT = 3;

	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};

	ret = test_disk_populate(FILES_ON_ROOT, 0, 0, 0, ".lc3");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".lc3");

	if (ret < 0) {
		zassert_equal(0, ret, "sd_card_list_files_match returned error, %u", ret);
	}

	TC_PRINT("Files found: %u\n", ret);

	zassert_equal(FILES_ON_ROOT, ret, "Num files found mismatch, %u", ret);
}

ZTEST(sd_card, test_valid_files_and_dummy_files)
{
	int ret;
	static const uint8_t FILES_ON_ROOT = 3;

	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};

	ret = test_disk_populate(FILES_ON_ROOT, 0, 0, 0, ".lc3");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);

	ret = test_disk_populate(FILES_ON_ROOT, 0, 0, 0, ".dum");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".lc3");

	if (ret < 0) {
		zassert_equal(0, ret, "sd_card_list_files_match returned error, %u", ret);
	}

	zassert_equal(FILES_ON_ROOT, ret, "Num files found mismatch, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".dum");

	if (ret < 0) {
		zassert_equal(0, ret, "sd_card_list_files_match returned error, %u", ret);
	}

	zassert_equal(FILES_ON_ROOT, ret, "Num files found mismatch, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".nonexist");

	if (ret < 0) {
		zassert_equal(0, ret, "sd_card_list_files_match returned error, %u", ret);
	}

	zassert_equal(0, ret, "Num files found mismatch, %u", ret);
}

/*
 * ZTEST(sd_card, test_several_levels)
 * {
 *	int ret;
 *	static const uint8_t FILES_ON_ROOT = 0;
 *	static const uint8_t NUM_LEVELS = 4;
 *	static const uint8_t FILES_PER_FOLDER = 1;
 *	static const uint8_t FOLDERS_PER_LEVEL = 1;
 *
 *	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};
 *
 *	ret = test_disk_populate(FILES_ON_ROOT, NUM_LEVELS, FILES_PER_FOLDER, FOLDERS_PER_LEVEL,
 *				 false);
 *	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);
 *
 *	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
 *				       ".lc3");
 *
 *	if (ret < 0) {
 *		zassert_equal(0, ret, "sd_card_list_files_match returned error, %u", ret);
 *	}
 *
 *	TC_PRINT("Files found: %u\n", ret);
 *
 *	zassert_equal(FILES_ON_ROOT + (FILES_PER_FOLDER * FOLDERS_PER_LEVEL * NUM_LEVELS), ret,
 *		      "Num files found mismatch, %u", ret);
 *}
 *
 */

ZTEST_SUITE(sd_card, NULL, setup_fn, before_fn, NULL, NULL);
