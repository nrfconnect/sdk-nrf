/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define HOSTNAME_SIZE CONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE
#define STACK_SIZE    CONFIG_DOWNLOADER_STACK_SIZE

/* Ensure that the stack size is large enough
 * to accommodate for host and file names
 */

BUILD_ASSERT(STACK_SIZE - HOSTNAME_SIZE >= 512, "Your stack size is too small");
