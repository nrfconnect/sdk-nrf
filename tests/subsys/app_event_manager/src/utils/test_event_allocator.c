/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include "test_event_allocator.h"

static bool oom_expected;


void test_event_allocator_oom_expect(bool expected)
{
	oom_expected = expected;
}

void *app_event_manager_alloc(size_t size)
{
	void *event = k_malloc(size);

	if (unlikely(!event)) {
		zassert_true(oom_expected, "Unexpected OOM error");
	}

	return event;
}

void app_event_manager_free(void *addr)
{
	k_free(addr);
}
