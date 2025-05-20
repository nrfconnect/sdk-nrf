/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <modules/sd_card.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <math.h>

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
	} else {
		TC_PRINT("Failed to open file\n");
		return ret;
	}

	return 0;
}

static int dir_name_get(char *name, size_t name_size, uint32_t curr_level, uint32_t curr_dir_id,
			uint8_t subdirs)
{
	int ret;

	name[0] = '\0';
	strcat(name, "/SD:");

	struct dir_tree {
		uint32_t peer_id;
		uint32_t numeric_name;
	};

	struct dir_tree f_tree[5];

	uint32_t on_level = curr_level;

	f_tree[on_level].peer_id = curr_dir_id;
	f_tree[on_level].numeric_name = curr_dir_id % subdirs;

	while (on_level > 0) {
		f_tree[on_level - 1].peer_id = f_tree[on_level].peer_id / subdirs;
		f_tree[on_level - 1].numeric_name = f_tree[on_level - 1].peer_id % subdirs;
		on_level--;
	}

	for (int i = 0; i < curr_level + 1; i++) {
		char tmp[100] = {'\0'};

		ret = sprintf(tmp, "/dir_%d", f_tree[i].numeric_name);
		strcat(name, tmp);
	}

	return 0;
}

static bool directory_exists(const char *path)
{
	struct fs_dirent entry;
	int ret = fs_stat(path, &entry);

	return (ret == 0 && entry.type == FS_DIR_ENTRY_DIR);
}

static int test_disk_populate(uint8_t num_levels, uint8_t files_per_dir, uint8_t subdirs,
			      char const *const extension)
{
	TC_PRINT("Writing files. Num levels %u, files_per_dir %u, dirs_pr_level %u\n", num_levels,
		 files_per_dir, subdirs);

	int ret;
	int files_added = 0;
	char current_path[200] = {'\0'};

	for (int level = 0; level < num_levels; level++) {
		uint32_t dirs_on_level = pow(subdirs, (level + 1));

		for (int i = 0; i < dirs_on_level; i++) {
			ret = dir_name_get(current_path, 200, level, i, subdirs);
			zassert_equal(0, ret, "dir_name_get should return 0, %u", ret);
			if (!directory_exists(current_path)) {
				ret = fs_mkdir(current_path);
				zassert_equal(0, ret, "fs_mkdir should return 0, %u", ret);
			}

			for (int j = 0; j < files_per_dir; j++) {
				char filepath[1000] = {'\0'};
				char filename[100] = {'\0'};

				strcat(filepath, current_path);
				sprintf(filename, "/file_%d%s", j, extension);
				strcat(filepath, filename);
				ret = test_create_file(filepath);
				zassert_equal(0, ret, "test_create_file should return 0, %u", ret);
				files_added++;
			}
		}
	}

	TC_PRINT("--------Done writing %d files--------\n", files_added);
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
	int ret;

	ret = fs_mkfs(FS_FATFS, (uintptr_t)MKFS_DEV_ID, NULL, 0);

	if (ret < 0) {
		TC_PRINT("Format failed %d\n", ret);
		zassert_equal(0, ret, "fs_mkfs should return 0, %u", ret);
	}
}

#define SD_FILECOUNT_MAX 1500
/* Align at 4-byte boundary */
#define SD_PATHLEN_MAX	 ROUND_DOWN(CONFIG_FS_FATFS_MAX_LFN, 4)

ZTEST(sd_card, test_basic_files)
{
	int ret;

	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};

	ret = test_disk_populate(1, 2, 1, ".lc3");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".lc3");

	zassert_true(ret >= 0, "sd_card_list_files_match returned error, %u", ret);
	zassert_equal(2, ret, "Num files found mismatch, %u", ret);
	zassert_str_equal("dir_0/file_0.lc3", sd_paths_and_files[0], "File mismatch: %s",
			  sd_paths_and_files[0]);
	zassert_str_equal("dir_0/file_1.lc3", sd_paths_and_files[1], "File mismatch: %s",
			  sd_paths_and_files[1]);
}

/* Same as test_basic_files. Checks that functions are reentrant */
ZTEST(sd_card, test_basic_files_again)
{
	int ret;

	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};

	ret = test_disk_populate(1, 3, 1, ".lc3");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".lc3");

	zassert_true(ret >= 0, "sd_card_list_files_match returned error, %u", ret);
	zassert_equal(3, ret, "Num files found mismatch, %u", ret);
}

ZTEST(sd_card, test_valid_files_and_dummy_files)
{
	int ret;

	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};

	ret = test_disk_populate(1, 2, 1, ".lc3");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);

	ret = test_disk_populate(1, 2, 1, ".dum");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".lc3");

	zassert_true(ret >= 0, "sd_card_list_files_match returned error, %u", ret);
	zassert_equal(2, ret, "Num files found mismatch, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".dum");

	zassert_true(ret >= 0, "sd_card_list_files_match returned error, %u", ret);
	zassert_equal(2, ret, "Num files found mismatch, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".nonexist");

	zassert_true(ret >= 0, "sd_card_list_files_match returned error, %u", ret);
	zassert_equal(0, ret, "Num files found mismatch, %u", ret);
}

ZTEST(sd_card, test_deep_structure)
{
	int ret;
	const uint8_t NUM_LEVELS = 7;
	const uint8_t FILES_PER_DIR = 1;
	const uint8_t SUBDIRS = 1;

	static char sd_paths_and_files[10][SD_PATHLEN_MAX] = {'\0'};

	ret = test_disk_populate(NUM_LEVELS, FILES_PER_DIR, SUBDIRS, ".lc3");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);
	uint32_t files_created = ret;

	ret = sd_card_list_files_match(10, SD_PATHLEN_MAX, sd_paths_and_files, NULL, ".lc3");

	zassert_true(ret >= 0, "sd_card_list_files_match returned error, %u", ret);
	zassert_equal(files_created, ret, "Num files found mismatch, %u", ret);
}

ZTEST(sd_card, test_many_files)
{
	int ret;
	const uint8_t NUM_LEVELS = 2;
	const uint8_t FILES_PER_DIR = 50;
	const uint8_t SUBDIRS = 4;

	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};

	ret = test_disk_populate(NUM_LEVELS, FILES_PER_DIR, SUBDIRS, ".lc3");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);
	uint32_t files_created = ret;

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".lc3");

	zassert_true(ret >= 0, "sd_card_list_files_match returned error, %u", ret);
	zassert_equal(files_created, ret, "Num files found mismatch, %u", ret);
}

ZTEST(sd_card, test_with_start_path)
{
	int ret;

	static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX] = {'\0'};

	ret = test_disk_populate(2, 1, 2, ".lc3");
	zassert_true(ret >= 0, "test_disk_populate returned error, %u", ret);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files,
				       "dir_1", ".lc3");

	zassert_true(ret >= 0, "sd_card_list_files_match returned error, %u", ret);
	zassert_equal(3, ret, "Num files found mismatch, %u", ret);
	zassert_str_equal("dir_1/file_0.lc3", sd_paths_and_files[0], "File mismatch: %s",
			  sd_paths_and_files[0]);
	zassert_str_equal("dir_1/dir_0/file_0.lc3", sd_paths_and_files[1], "File mismatch: %s",
			  sd_paths_and_files[1]);
	zassert_str_equal("dir_1/dir_1/file_0.lc3", sd_paths_and_files[2], "File mismatch: %s",
			  sd_paths_and_files[0]);

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files,
				       "dir_1/dir_0", ".lc3");

	zassert_true(ret >= 0, "sd_card_list_files_match returned error, %u", ret);
	zassert_equal(1, ret, "Num files found mismatch, %u", ret);

	zassert_str_equal("dir_1/dir_0/file_0.lc3", sd_paths_and_files[0], "File mismatch: %s",
			  sd_paths_and_files[0]);
}

ZTEST_SUITE(sd_card, NULL, setup_fn, before_fn, NULL, NULL);
