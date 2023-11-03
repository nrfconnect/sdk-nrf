/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FS_EVENT_H_
#define _FS_EVENT_H_

/**
 * @brief File system event
 * @defgroup fs_event File system event
 * @{
 */

#include <string.h>
#include <zephyr/toolchain.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

enum fs_request {
	FS_REQUEST_CREATE_FILE,
	FS_REQUEST_PARSE_FILE, /* Triggered to detect user changes */
};

/** Peer connection event. */
struct fs_event {
	struct app_event_header header;

	enum fs_request req;
	const char *mnt_point;
};

APP_EVENT_TYPE_DECLARE(fs_event);

int fs_event_helper_file_write(
	const char *mnt_point,
	const char *file_path,
	char const *file_contents,
	size_t contents_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FS_EVENT_H_ */
