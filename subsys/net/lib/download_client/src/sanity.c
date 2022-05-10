/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define HOSTNAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE
#define FILENAME_SIZE CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE
#define STACK_SIZE CONFIG_DOWNLOAD_CLIENT_STACK_SIZE

/* Ensure that the stack size is large enough
 * to accommodate for host and file names
 */

BUILD_ASSERT(
	STACK_SIZE - (HOSTNAME_SIZE + FILENAME_SIZE) >= 512,
	"Your stack size is too small"
);
