/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/sys/atomic.h>

#include "rtfw_internal.h"

static atomic_t notify_count;

void rtfw_doorbell_init(void)
{
	atomic_clear(&notify_count);
}

void rtfw_doorbell_notify(void)
{
	atomic_inc(&notify_count);
}

void rtfw_test_doorbell_reset(void)
{
	atomic_clear(&notify_count);
}

uint32_t rtfw_test_doorbell_count_get(void)
{
	return (uint32_t)atomic_get(&notify_count);
}
