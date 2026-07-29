/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define CPU_THREAD_STACKSIZE (256)
#define CPU_THREAD_PRIORITY  (1)
#define CPU_THREAD_SLEEP     (100)
#define CPU_ACTIVE_TIME_US   (1000 * 1000)
