/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "test_oom.h"
#include <zephyr.h>

void *app_evt_mgr_alloc(size_t size)
{
	void *event = k_malloc(size);

	if (unlikely(!event)) {
		oom_error_handler();
	}

	return event;
}

void app_evt_mgr_free(void *addr)
{
	k_free(addr);
}
