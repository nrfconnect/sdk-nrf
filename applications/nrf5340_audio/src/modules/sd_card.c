/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sd_card.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sd_card, CONFIG_MODULE_SD_CARD_LOG_LEVEL);

#define SD_ROOT_PATH	   "/SD:/"
/* Round down to closest 4-byte boundary */
#define PATH_MAX_LEN	   ROUND_DOWN(CONFIG_FS_FATFS_MAX_LFN, 4)
#define SD_CARD_LEVELS_MAX 8
#define SD_CARD_BUF_SIZE   700

static uint32_t num_files_added;
static struct k_mem_slab slab_A;
static struct k_mem_slab slab_B;

static const char *sd_root_path = "/SD:";
static FATFS fat_fs;
static bool sd_init_success;

static struct fs_mount_t mnt_pt = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

/**
 * @brief	Replaces first carriage return or line feed with null terminator.
 */
static void cr_lf_remove(char *str)
{
	char *p = str;

	while (*p != '\0') {
		if (*p == '\r' || *p == '\n') {
			*p = '\0';
			break;
		}
		p++;
	}
}

/**
 * @brief Recursively traverse the SD card tree.
 */
static int traverse_down(char const *const path, uint8_t curr_depth, uint16_t result_file_num_max,
			 uint16_t result_path_len_max, char result[][result_path_len_max],
			 char const *const search_pattern)
{
	int ret = 0;

	if (curr_depth > SD_CARD_LEVELS_MAX) {
		LOG_WRN("At tree curr_depth %u, greater than %u", curr_depth, SD_CARD_LEVELS_MAX);
		return 0;
	}

	char *slab_A_ptr;

	ret = k_mem_slab_alloc(&slab_A, (void **)&slab_A_ptr, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to alloc slab A: %d", ret);
		return ret;
	}

	char *slab_A_ptr_origin = slab_A_ptr;
	char *slab_B_ptr;

	ret = k_mem_slab_alloc(&slab_B, (void **)&slab_B_ptr, K_NO_WAIT);
	if (ret) {
		k_mem_slab_free(&slab_A, (void *)slab_A_ptr_origin);
		LOG_ERR("Failed to alloc slab B: %d", ret);
		return ret;
	}

	char *slab_B_ptr_origin = slab_B_ptr;
	size_t slab_A_ptr_size = SD_CARD_BUF_SIZE;

	/* Search for folders */
	ret = sd_card_list_files(path, slab_A_ptr, &slab_A_ptr_size, false);
	if (ret == -ENOENT) {
		/* Not able to open, hence likely not a folder */
		ret = 0;
		goto cleanup;
	} else if (ret) {
		goto cleanup;
	}

	LOG_DBG("At curr_depth %d tmp_buf is: %s", curr_depth, slab_A_ptr);

	char *token = strtok_r(slab_A_ptr, "\r\n", &slab_A_ptr);

	while (token != NULL) {
		if (strstr(token, "System Volume Information") != NULL) {
			/* Skipping System Volume Information */
			token = strtok_r(NULL, "\n", &slab_A_ptr);
			continue;
		}

		cr_lf_remove(token);
		memset(slab_B_ptr, '\0', PATH_MAX_LEN);

		if (path != NULL) {
			strcat(slab_B_ptr, path);
			cr_lf_remove(slab_B_ptr);
			strcat(slab_B_ptr, "/");
		}

		strcat(slab_B_ptr, token);

		if (strstr(token, search_pattern) != NULL) {
			if (num_files_added >= result_file_num_max) {
				LOG_WRN("Max file count reached %u", result_file_num_max);
				ret = -ENOMEM;
				goto cleanup;
			}
			strcpy(result[num_files_added], slab_B_ptr);
			num_files_added++;
			LOG_DBG("Added file num: %d at: %s", num_files_added, slab_B_ptr);

		} else {
			ret = traverse_down(slab_B_ptr, curr_depth + 1, result_file_num_max,
					    result_path_len_max, result, search_pattern);
			if (ret) {
				LOG_ERR("Failed to traverse down: %d", ret);
			}
		}

		token = strtok_r(NULL, "\n", &slab_A_ptr);
	}

cleanup:
	k_mem_slab_free(&slab_A, (void *)slab_A_ptr_origin);
	k_mem_slab_free(&slab_B, (void *)slab_B_ptr_origin);
	return ret;
}

int sd_card_list_files_match(uint16_t result_file_num_max, uint16_t result_path_len_max,
			     char result[][result_path_len_max], char *path,
			     char const *const search_pattern)
{
	int ret;

	num_files_added = 0;

	if (result == NULL) {
		return -EINVAL;
	}

	if (result_file_num_max == 0) {
		return -EINVAL;
	}

	if (result_path_len_max == 0 || (result_path_len_max > PATH_MAX_LEN)) {
		return -EINVAL;
	}

	if (search_pattern == NULL) {
		return -EINVAL;
	}

	char __aligned(4) buf_A[SD_CARD_BUF_SIZE * SD_CARD_LEVELS_MAX] = {'\0'};
	char __aligned(4) buf_B[PATH_MAX_LEN * SD_CARD_LEVELS_MAX] = {'\0'};

	ret = k_mem_slab_init(&slab_A, buf_A, SD_CARD_BUF_SIZE, SD_CARD_LEVELS_MAX);
	if (ret) {
		LOG_ERR("Failed to init slab: %d", ret);
		return ret;
	}

	ret = k_mem_slab_init(&slab_B, buf_B, PATH_MAX_LEN, SD_CARD_LEVELS_MAX);
	if (ret) {
		LOG_ERR("Failed to init slab: %d", ret);
		return ret;
	}

	ret = traverse_down(path, 0, result_file_num_max, result_path_len_max, result,
			    search_pattern);
	if (ret) {
		return ret;
	}

	return num_files_added;
}

int sd_card_list_files(char const *const path, char *buf, size_t *buf_size, bool extra_info)
{
	int ret;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;
	size_t used_buf_size = 0;

	if (!sd_init_success) {
		return -ENODEV;
	}

	fs_dir_t_init(&dirp);
	if (path == NULL) {
		ret = fs_opendir(&dirp, sd_root_path);
		if (ret) {
			LOG_ERR("Open SD card root dir failed");
			return ret;
		}
	} else {
		if (strlen(path) > CONFIG_FS_FATFS_MAX_LFN) {
			LOG_ERR("Path is too long");
			return -FR_INVALID_NAME;
		}

		strcat(abs_path_name, path);

		if (strchr(abs_path_name, '.')) {
			/* Path contains a dot. Regarded as not a folder*/
			return -ENOENT;
		}

		ret = fs_opendir(&dirp, abs_path_name);
		if (ret) {
			LOG_ERR("Open assigned path failed %d. %s", ret, abs_path_name);
			return ret;
		}
	}

	while (1) {
		ret = fs_readdir(&dirp, &entry);
		if (ret) {
			return ret;
		}

		if (entry.name[0] == 0) {
			break;
		}

		if (buf != NULL) {
			size_t remaining_buf_size = *buf_size - used_buf_size;
			ssize_t len;

			if (extra_info) {
				len = snprintk(&buf[used_buf_size], remaining_buf_size,
					       "[%s]\t%s\r\n",
					       entry.type == FS_DIR_ENTRY_DIR ? "DIR " : "FILE",
					       entry.name);
			} else {
				len = snprintk(&buf[used_buf_size], remaining_buf_size, "%s\r\n",
					       entry.name);
			}

			if (len >= remaining_buf_size) {
				LOG_ERR("Failed to append to buffer, error: %d", len);
				return -EINVAL;
			}

			used_buf_size += len;
		}

		LOG_INF("[%s] %s", entry.type == FS_DIR_ENTRY_DIR ? "DIR " : "FILE", entry.name);
	}

	ret = fs_closedir(&dirp);
	if (ret) {
		LOG_ERR("Close SD card root dir failed");
		return ret;
	}

	*buf_size = used_buf_size;
	return 0;
}

int sd_card_open_write_close(char const *const filename, char const *const data, size_t *size)
{
	int ret;
	struct fs_file_t f_entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;

	if (!sd_init_success) {
		return -ENODEV;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename is too long");
		return -ENAMETOOLONG;
	}

	strcat(abs_path_name, filename);
	fs_file_t_init(&f_entry);

	ret = fs_open(&f_entry, abs_path_name, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
	if (ret) {
		LOG_ERR("Create file failed");
		return ret;
	}

	/* If the file exists, moves the file position pointer to the end of the file */
	ret = fs_seek(&f_entry, 0, FS_SEEK_END);
	if (ret) {
		LOG_ERR("Seek file pointer failed");
		return ret;
	}

	ret = fs_write(&f_entry, data, *size);
	if (ret < 0) {
		LOG_ERR("Write file failed");
		return ret;
	}

	*size = ret;

	ret = fs_close(&f_entry);
	if (ret) {
		LOG_ERR("Close file failed");
		return ret;
	}

	return 0;
}

int sd_card_open_read_close(char const *const filename, char *const buf, size_t *size)
{
	int ret;
	struct fs_file_t f_entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;

	if (!sd_init_success) {
		return -ENODEV;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename is too long");
		return -FR_INVALID_NAME;
	}

	strcat(abs_path_name, filename);
	fs_file_t_init(&f_entry);

	ret = fs_open(&f_entry, abs_path_name, FS_O_READ);
	if (ret) {
		LOG_ERR("Open file failed");
		return ret;
	}

	ret = fs_read(&f_entry, buf, *size);
	if (ret < 0) {
		LOG_ERR("Read file failed. Ret: %d", ret);
		return ret;
	}

	*size = ret;
	if (*size == 0) {
		LOG_WRN("File is empty");
	}

	ret = fs_close(&f_entry);
	if (ret) {
		LOG_ERR("Close file failed");
		return ret;
	}

	return 0;
}

int sd_card_open(char const *const filename, struct fs_file_t *f_seg_read_entry)
{
	int ret;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;
	size_t avilable_path_space = PATH_MAX_LEN - strlen(SD_ROOT_PATH);

	if (!sd_init_success) {
		return -ENODEV;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename is too long");
		return -ENAMETOOLONG;
	}

	if ((strlen(abs_path_name) + strlen(filename)) > PATH_MAX_LEN) {
		LOG_ERR("Filepath is too long");
		return -EINVAL;
	}

	strncat(abs_path_name, filename, avilable_path_space);

	LOG_INF("abs path name:\t%s", abs_path_name);

	fs_file_t_init(f_seg_read_entry);

	ret = fs_open(f_seg_read_entry, abs_path_name, FS_O_READ);
	if (ret) {
		LOG_ERR("Open file failed: %d", ret);
		return ret;
	}

	return 0;
}

int sd_card_read(char *buf, size_t *size, struct fs_file_t *f_seg_read_entry)
{
	int ret;

	ret = fs_read(f_seg_read_entry, buf, *size);
	if (ret < 0) {
		LOG_ERR("Read file failed. Ret: %d", ret);
		return ret;
	}

	*size = ret;

	return 0;
}

int sd_card_close(struct fs_file_t *f_seg_read_entry)
{
	int ret;

	ret = fs_close(f_seg_read_entry);
	if (ret) {
		LOG_ERR("Close file failed: %d", ret);
		return ret;
	}

	return 0;
}

int sd_card_init(void)
{
	int ret;
	static const char *sd_dev = "SD";
	uint64_t sd_card_size_bytes;
	uint32_t sector_count;
	size_t sector_size;

	ret = disk_access_init(sd_dev);
	if (ret) {
		LOG_DBG("SD card init failed, please check if SD card inserted");
		return -ENODEV;
	}

	ret = disk_access_ioctl(sd_dev, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count);
	if (ret) {
		LOG_ERR("Unable to get sector count");
		return ret;
	}

	LOG_DBG("Sector count: %d", sector_count);

	ret = disk_access_ioctl(sd_dev, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size);
	if (ret) {
		LOG_ERR("Unable to get sector size");
		return ret;
	}

	LOG_DBG("Sector size: %d bytes", sector_size);

	sd_card_size_bytes = (uint64_t)sector_count * sector_size;

	LOG_INF("SD card volume size: %lld B", sd_card_size_bytes);

	mnt_pt.mnt_point = sd_root_path;

	ret = fs_mount(&mnt_pt);
	if (ret) {
		LOG_ERR("Mnt. disk failed, could be format issue. should be FAT/exFAT");
		return ret;
	}

	sd_init_success = true;

	return 0;
}
