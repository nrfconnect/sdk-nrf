/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/mesh/time_srv.h>
#include <time.h>
#include "model_utils.h"
#include <zephyr/types.h>

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

static const uint8_t month_cfg[12] = { 31, 28, 31, 30, 31, 30,
				    31, 31, 30, 31, 30, 31 };
static const uint8_t month_leap_cfg[12] = { 31, 29, 31, 30, 31, 30,
					 31, 31, 30, 31, 30, 31 };

static inline bool is_leap_year(uint32_t year)
{
	return ((year % 4) == 0) &&
	       (((year % 100) != 0) || ((year % 400) == 0));
}

int ts_to_tai(struct bt_mesh_time_tai *tai, struct tm *timeptr)
{
	uint32_t current_year = timeptr->tm_year + TM_START_YEAR;
	uint32_t days = 0;

	if (current_year < TAI_START_YEAR) {
		return -EAGAIN;
	}

	for (int year = TAI_START_YEAR; year < current_year; year++) {
		if (is_leap_year(year)) {
			days += DAYS_LEAP_YEAR;
		} else {
			days += DAYS_YEAR;
		}
	}

	const uint8_t *months =
		is_leap_year(current_year) ? month_leap_cfg : month_cfg;

	for (int i = 0; i < timeptr->tm_mon; i++) {
		days += months[i];
	}

	days += timeptr->tm_mday - 1;

	tai->sec = (days * SEC_PER_DAY);
	tai->sec += ((uint64_t)timeptr->tm_hour * SEC_PER_HOUR);
	tai->sec += ((uint64_t)timeptr->tm_min * SEC_PER_MIN);
	tai->sec += (uint64_t)timeptr->tm_sec;
	tai->subsec = 0;
	return 0;
}

void tai_to_ts(const struct bt_mesh_time_tai *tai, struct tm *timeptr)
{
	uint64_t day_cnt = tai->sec / SEC_PER_DAY;
	bool is_leap;
	uint32_t year;

	timeptr->tm_hour = (tai->sec % SEC_PER_DAY) / SEC_PER_HOUR;
	timeptr->tm_min = (tai->sec % SEC_PER_HOUR) / SEC_PER_MIN;
	timeptr->tm_sec = (tai->sec % SEC_PER_MIN);
	timeptr->tm_wday = (TAI_START_DAY + day_cnt) % WEEKDAY_CNT;
	timeptr->tm_isdst = -1;

	for (year = TAI_START_YEAR;; year++) {
		is_leap = is_leap_year(year);

		uint32_t days_in_year = is_leap ? DAYS_LEAP_YEAR : DAYS_YEAR;

		if (days_in_year > day_cnt) {
			break;
		}

		day_cnt -= days_in_year;
	}

	timeptr->tm_yday = day_cnt;
	timeptr->tm_year = year - TM_START_YEAR;

	const uint8_t *months = is_leap ? month_leap_cfg : month_cfg;

	for (timeptr->tm_mon = 0; months[timeptr->tm_mon] < day_cnt;
	     timeptr->tm_mon++) {
		day_cnt -= months[timeptr->tm_mon];
	}

	timeptr->tm_mday = day_cnt + 1;
}
