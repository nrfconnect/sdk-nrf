/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Ensure 'strnlen' is available even with -std=c99. */
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <nrfx.h>

#include "fota_download_util.h"

#define MAX_RESOURCE_LOCATOR_LEN 500

LOG_MODULE_REGISTER(fota_download_util, CONFIG_FOTA_DOWNLOAD_LOG_LEVEL);

int fota_download_parse_dual_resource_locator(char *const file, bool s0_active,
					     const char **selected_path)
{
	if (file == NULL || selected_path == NULL) {
		LOG_ERR("Got NULL pointer");
		return -EINVAL;
	}

	if (!nrfx_is_in_ram(file)) {
		LOG_ERR("'file' pointer is not located in RAM");
		return -EFAULT;
	}

	/* Ensure that 'file' is null-terminated. */
	if (strnlen(file, MAX_RESOURCE_LOCATOR_LEN) == MAX_RESOURCE_LOCATOR_LEN) {
		LOG_ERR("Input is not null terminated");
		return -ENOTSUP;
	}

	/* We have verified that there is a null-terminator, so this is safe */
	char *delimiter = strstr(file, " ");

	if (delimiter == NULL) {
		/* Could not find delimiter in input */
		*selected_path = NULL;
		return 0;
	}

	/* Insert null-terminator to split the dual filepath into two separate filepaths */
	*delimiter = '\0';
	const char *s0_path = file;
	const char *s1_path = delimiter + 1;

	*selected_path = s0_active ? s1_path : s0_path;
	return 0;
}
