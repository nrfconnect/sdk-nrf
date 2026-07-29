/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>

#define PWM_THREAD_STACKSIZE (512)
#define PWM_THREAD_PRIORITY  (1)
#define PWM_THREAD_SLEEP     (250)
