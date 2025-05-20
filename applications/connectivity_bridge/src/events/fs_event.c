/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include <fs_event.h>
#include <zephyr/fs/fs.h>

static void log_fs_event(const struct app_event_header *aeh)
{
	const struct fs_event *event = cast_fs_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "req:%d", event->req);
}

int fs_event_helper_file_write(
	const char *mnt_point,
	const char *file_path,
	char const *file_contents,
	size_t contents_len)
{
	struct fs_file_t file;
	ssize_t bytes_written;
	char fname[128];
	int err;

	err = snprintf(fname, sizeof(fname), "%s/%s", mnt_point, file_path);
	if (err <= 0 || err > sizeof(fname)) {
		return -ENOMEM;
	}

	fs_file_t_init(&file);
	err = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (err) {
		return err;
	}

	err = fs_seek(&file, 0, FS_SEEK_END);
	if (err) {
		return err;
	}

	bytes_written = fs_write(&file, file_contents, contents_len);
	if (bytes_written != contents_len) {
		return -ENOMEM;
	}

	err = fs_close(&file);
	if (err) {
		return err;
	}

	return 0;
}

APP_EVENT_TYPE_DEFINE(fs_event,
		  log_fs_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_BRIDGE_LOG_FS_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
