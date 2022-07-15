/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/types.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

#define MODULE fs_handler
#include "module_state_event.h"
#include "fs_event.h"
#include "power_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_MSC_LOG_LEVEL);

#define FATFS_MNTP	"/" CONFIG_MASS_STORAGE_DISK_NAME ":"

static FATFS fat_fs;

static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

static bool fs_parse_pending;

/* Initialize filesystem and leave it mounted */
static int fs_init(void)
{
	int err;

	err = fs_mount(&fatfs_mnt);
	if (err) {
		LOG_ERR("fs_mount: %d", err);
		return err;
	}

	/* The default RAM disk may contain unwanted files. Erase these. */
	struct fs_dir_t dir;

	fs_dir_t_init(&dir);
	err = fs_opendir(&dir, FATFS_MNTP "/");
	if (err) {
		LOG_ERR("fs_opendir: %d", err);
		return err;
	}

	while (true) {
		struct fs_dirent dirent;
		char name_buf[64];

		err = fs_readdir(&dir, &dirent);
		if (err) {
			LOG_ERR("fs_readdir: %d", err);
			break;
		}

		if (dirent.name[0] == 0) {
			break;
		}

		int len = snprintf(
			name_buf,
			sizeof(name_buf),
			"%s/%s",
			FATFS_MNTP,
			dirent.name);
		if (len <= 0 || len > sizeof(name_buf)) {
			LOG_ERR("Name formatting error");
			continue;
		}

		err = fs_unlink(name_buf);
		if (err) {
			LOG_ERR("fs_unlink(%s): %d", name_buf, err);
		}
	}

	err = fs_closedir(&dir);
	if (err) {
		LOG_ERR("fs_closedir: %d", err);
		return err;
	}

	return 0;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			int err;

			err = fs_init();
			if (err) {
				return false;
			}

			fs_parse_pending = false;

			/* Use READY state while actively working with filesystem */
			module_set_state(MODULE_STATE_READY);

			/* Event subscribers can now populate the disk */
			struct fs_event *event = new_fs_event();

			event->req = FS_REQUEST_CREATE_FILE;
			event->mnt_point = fatfs_mnt.mnt_point;

			APP_EVENT_SUBMIT(event);
		} else if (check_state(event, MODULE_ID(usb_cdc), MODULE_STATE_STANDBY)) {
			int err;

			/* Event subscribers should look for file changes now */

			err = fs_mount(&fatfs_mnt);
			if (err) {
				LOG_ERR("fs_mount: %d", err);
				return false;
			}

			struct fs_event *event = new_fs_event();

			event->req = FS_REQUEST_PARSE_FILE;
			event->mnt_point = fatfs_mnt.mnt_point;

			fs_parse_pending = true;

			APP_EVENT_SUBMIT(event);
		}

		return false;
	}

	if (is_fs_event(aeh)) {
		const struct fs_event *event =
			cast_fs_event(aeh);

		if (event->req == FS_REQUEST_CREATE_FILE) {
			int err;

			/* All initial files have been created at this point.
			 * There will be no more file activity,
			 * unless a USB disconnect happens.
			 */

			err = fs_unmount(&fatfs_mnt);
			if (err) {
				LOG_ERR("fs_unmount: %d", err);
			}

			module_set_state(MODULE_STATE_STANDBY);
		} else if (event->req == FS_REQUEST_PARSE_FILE) {
			int err;

			/* All files should now have finished parsing.
			 * Filesystem can be unmounted again.
			 */

			err = fs_unmount(&fatfs_mnt);
			if (err) {
				LOG_ERR("fs_unmount: %d", err);
			}

			fs_parse_pending = false;
		}

		return false;
	}

	if (is_power_down_event(aeh)) {
		if (fs_parse_pending) {
			/* Delay power down until file parsing has completed */
			return true;
		} else {
			return false;
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, fs_event);
