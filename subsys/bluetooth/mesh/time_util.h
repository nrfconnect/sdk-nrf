/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_MESH_TIME_UTIL
#define BT_MESH_TIME_UTIL

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAI_START_YEAR 2000
#define TM_START_YEAR 1900
#define TAI_START_DAY 6

#define DAYS_YEAR 365ULL
#define DAYS_LEAP_YEAR 366ULL
#define SEC_PER_MIN (60ULL)
#define SEC_PER_HOUR (60ULL * SEC_PER_MIN)
#define SEC_PER_DAY (24ULL * SEC_PER_HOUR)

#define SEC_PER_YEAR (DAYS_YEAR * SEC_PER_DAY)
#define SEC_PER_LEAP_YEAR (DAYS_LEAP_YEAR * SEC_PER_DAY)
#define FEB_DAYS 28
#define FEB_LEAP_DAYS 29
#define WEEKDAY_CNT 7

static inline bool is_leap_year(uint32_t year)
{
	return ((year % 4) == 0) &&
	       (((year % 100) != 0) || ((year % 400) == 0));
}

void tai_to_ts(const struct bt_mesh_time_tai *tai, struct tm *timeptr);
int ts_to_tai(struct bt_mesh_time_tai *tai, const struct tm *timeptr);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_TIME_UTIL */

/** @} */
