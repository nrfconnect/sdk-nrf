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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sd_card, CONFIG_MODULE_SD_CARD_LOG_LEVEL);

#define SD_ROOT_PATH	      "/SD:/"
/* Maximum length for path support by Windows file system */
#define PATH_MAX_LEN	      260
#define K_SEM_OPER_TIMEOUT_MS 100

K_SEM_DEFINE(m_sem_sd_oper_ongoing, 1, 1);

static const char *sd_root_path = "/SD:";
static FATFS fat_fs;
static bool sd_init_success;

static struct fs_mount_t mnt_pt = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

int sd_card_list_files(char const *const path, char *buf, size_t *buf_size)
{
	int ret;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;
	size_t used_buf_size = 0;

	ret = k_sem_take(&m_sem_sd_oper_ongoing, K_MSEC(K_SEM_OPER_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("Sem take failed. Ret: %d", ret);
		return ret;
	}

	if (!sd_init_success) {
		k_sem_give(&m_sem_sd_oper_ongoing);
		return -ENODEV;
	}

	fs_dir_t_init(&dirp);

	if (path == NULL) {
		ret = fs_opendir(&dirp, sd_root_path);
		if (ret) {
			LOG_ERR("Open SD card root dir failed");
			k_sem_give(&m_sem_sd_oper_ongoing);
			return ret;
		}
	} else {
		if (strlen(path) > CONFIG_FS_FATFS_MAX_LFN) {
			LOG_ERR("Path is too long");
			k_sem_give(&m_sem_sd_oper_ongoing);
			return -FR_INVALID_NAME;
		}

		strcat(abs_path_name, path);

		ret = fs_opendir(&dirp, abs_path_name);
		if (ret) {
			LOG_ERR("Open assigned path failed");
			k_sem_give(&m_sem_sd_oper_ongoing);
			return ret;
		}
	}

	while (1) {
		ret = fs_readdir(&dirp, &entry);
		if (ret) {
			k_sem_give(&m_sem_sd_oper_ongoing);
			return ret;
		}

		if (entry.name[0] == 0) {
			break;
		}

		if (buf != NULL) {
			size_t remaining_buf_size = *buf_size - used_buf_size;
			ssize_t len = snprintk(
				&buf[used_buf_size], remaining_buf_size, "[%s]\t%s\n",
				entry.type == FS_DIR_ENTRY_DIR ? "DIR " : "FILE", entry.name);

			if (len >= remaining_buf_size) {
				LOG_ERR("Failed to append to buffer, error: %d", len);
				k_sem_give(&m_sem_sd_oper_ongoing);
				return -EINVAL;
			}

			used_buf_size += len;
		}

		LOG_INF("[%s] %s", entry.type == FS_DIR_ENTRY_DIR ? "DIR " : "FILE", entry.name);
	}

	ret = fs_closedir(&dirp);
	if (ret) {
		LOG_ERR("Close SD card root dir failed");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	*buf_size = used_buf_size;
	k_sem_give(&m_sem_sd_oper_ongoing);
	return 0;
}

int sd_card_open_write_close(char const *const filename, char const *const data, size_t *size)
{
	int ret;
	struct fs_file_t f_entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;

	ret = k_sem_take(&m_sem_sd_oper_ongoing, K_MSEC(K_SEM_OPER_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("Sem take failed. Ret: %d", ret);
		return ret;
	}

	if (!sd_init_success) {
		k_sem_give(&m_sem_sd_oper_ongoing);
		return -ENODEV;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename is too long");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return -ENAMETOOLONG;
	}

	strcat(abs_path_name, filename);
	fs_file_t_init(&f_entry);

	ret = fs_open(&f_entry, abs_path_name, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
	if (ret) {
		LOG_ERR("Create file failed");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	/* If the file exists, moves the file position pointer to the end of the file */
	ret = fs_seek(&f_entry, 0, FS_SEEK_END);
	if (ret) {
		LOG_ERR("Seek file pointer failed");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	ret = fs_write(&f_entry, data, *size);
	if (ret < 0) {
		LOG_ERR("Write file failed");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	*size = ret;

	ret = fs_close(&f_entry);
	if (ret) {
		LOG_ERR("Close file failed");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	k_sem_give(&m_sem_sd_oper_ongoing);
	return 0;
}

int sd_card_open_read_close(char const *const filename, char *const buf, size_t *size)
{
	int ret;
	struct fs_file_t f_entry;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;

	ret = k_sem_take(&m_sem_sd_oper_ongoing, K_MSEC(K_SEM_OPER_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("Sem take failed. Ret: %d", ret);
		return ret;
	}

	if (!sd_init_success) {
		k_sem_give(&m_sem_sd_oper_ongoing);
		return -ENODEV;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename is too long");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return -FR_INVALID_NAME;
	}

	strcat(abs_path_name, filename);
	fs_file_t_init(&f_entry);

	ret = fs_open(&f_entry, abs_path_name, FS_O_READ);
	if (ret) {
		LOG_ERR("Open file failed");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	ret = fs_read(&f_entry, buf, *size);
	if (ret < 0) {
		LOG_ERR("Read file failed. Ret: %d", ret);
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	*size = ret;
	if (*size == 0) {
		LOG_WRN("File is empty");
	}

	ret = fs_close(&f_entry);
	if (ret) {
		LOG_ERR("Close file failed");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	k_sem_give(&m_sem_sd_oper_ongoing);
	return 0;
}

int sd_card_open(char const *const filename, struct fs_file_t *f_seg_read_entry)
{
	int ret;
	char abs_path_name[PATH_MAX_LEN + 1] = SD_ROOT_PATH;
	size_t avilable_path_space = PATH_MAX_LEN - strlen(SD_ROOT_PATH);

	ret = k_sem_take(&m_sem_sd_oper_ongoing, K_MSEC(K_SEM_OPER_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("Sem take failed. Ret: %d", ret);
		return ret;
	}

	if (!sd_init_success) {
		k_sem_give(&m_sem_sd_oper_ongoing);
		return -ENODEV;
	}

	if (strlen(filename) > CONFIG_FS_FATFS_MAX_LFN) {
		LOG_ERR("Filename is too long");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return -ENAMETOOLONG;
	}

	if ((strlen(abs_path_name) + strlen(filename)) > PATH_MAX_LEN) {
		LOG_ERR("Filepath is too long");
		k_sem_give(&m_sem_sd_oper_ongoing);
		return -EINVAL;
	}

	strncat(abs_path_name, filename, avilable_path_space);

	LOG_INF("abs path name:\t%s", abs_path_name);

	fs_file_t_init(f_seg_read_entry);

	ret = fs_open(f_seg_read_entry, abs_path_name, FS_O_READ);
	if (ret) {
		LOG_ERR("Open file failed: %d", ret);
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	return 0;
}

int sd_card_read(char *buf, size_t *size, struct fs_file_t *f_seg_read_entry)
{
	int ret;

	if (!(k_sem_count_get(&m_sem_sd_oper_ongoing) <= 0)) {
		LOG_ERR("SD operation not ongoing");
		return -EPERM;
	}

	ret = fs_read(f_seg_read_entry, buf, *size);
	if (ret < 0) {
		LOG_ERR("Read file failed. Ret: %d", ret);
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	*size = ret;

	return 0;
}

int sd_card_close(struct fs_file_t *f_seg_read_entry)
{
	int ret;

	if (k_sem_count_get(&m_sem_sd_oper_ongoing) != 0) {
		LOG_ERR("SD operation not ongoing");
		return -EPERM;
	}

	ret = fs_close(f_seg_read_entry);
	if (ret) {
		LOG_ERR("Close file failed: %d", ret);
		k_sem_give(&m_sem_sd_oper_ongoing);
		return ret;
	}

	k_sem_give(&m_sem_sd_oper_ongoing);
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

	LOG_INF("SD card volume size: %d MB", (uint32_t)(sd_card_size_bytes >> 20));

	mnt_pt.mnt_point = sd_root_path;

	ret = fs_mount(&mnt_pt);
	if (ret) {
		LOG_ERR("Mnt. disk failed, could be format issue. should be FAT/exFAT");
		return ret;
	}

	sd_init_success = true;

	return 0;
}
