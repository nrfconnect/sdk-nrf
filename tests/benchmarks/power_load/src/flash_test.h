/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>

#define MAX_TEST_BUFFER_SIZE 2048

#define FLASH_THREAD_STACKSIZE (4096)
#define FLASH_THREAD_PRIORITY  (1)
#define FLASH_THREAD_SLEEP     (100)
