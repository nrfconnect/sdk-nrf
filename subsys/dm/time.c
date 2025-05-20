/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "time.h"

uint32_t time_now(void)
{
	return nrf_rtc_counter_get(NRF_RTC0);
}

uint32_t time_distance_get(uint32_t t1, uint32_t t2)
{
	const uint32_t tmax = RTC_COUNTER_MAX;

	if (t1 > t2) {
		return t2 + (tmax - t1) + 1;
	}

	return t2 - t1;
}
