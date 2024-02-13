/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define MODULE file_readme
#include "fs_event.h"

static const char file_contents[] = {
#include "readme.h"
};

#if CONFIG_FS_FATFS_LFN
#define FILE_NAME         "README.txt"
#else
#define FILE_NAME         "README.TXT"
#endif
#define FILE_CONTENTS     file_contents
#define FILE_CONTENTS_LEN strlen(file_contents)

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_fs_event(aeh)) {
		const struct fs_event *event =
			cast_fs_event(aeh);

		if (event->req == FS_REQUEST_CREATE_FILE) {
			int err;

			err = fs_event_helper_file_write(
				event->mnt_point,
				FILE_NAME,
				FILE_CONTENTS,
				FILE_CONTENTS_LEN);

			__ASSERT_NO_MSG(err == 0);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, fs_event);
