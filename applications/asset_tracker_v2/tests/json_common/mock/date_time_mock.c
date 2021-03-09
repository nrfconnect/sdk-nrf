/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

#include "date_time.h"

/* Mocking function that always converts the input uptime to a known timestamp. */
int date_time_uptime_to_unix_time_ms(int64_t *uptime)
{
	*uptime = 1563968747123;

	return 0;
}
