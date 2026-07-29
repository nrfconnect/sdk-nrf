/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#define GPIO_THREAD_STACKSIZE (512)
#define GPIO_THREAD_PRIORITY  (1)
#define GPIO_THREAD_SLEEP     (1000)
