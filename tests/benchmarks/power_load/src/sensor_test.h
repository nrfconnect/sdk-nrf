/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

#define TEMP_SENSOR_THREAD_STACKSIZE (512)
#define TEMP_SENSOR_THREAD_PRIORITY  (1)
#define TEMP_SENSOR_THREAD_SLEEP     (2000)
