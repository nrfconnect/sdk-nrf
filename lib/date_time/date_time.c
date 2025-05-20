/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <date_time.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/logging/log.h>

#include "date_time_core.h"

LOG_MODULE_REGISTER(date_time, CONFIG_DATE_TIME_LOG_LEVEL);

int date_time_set(const struct tm *new_date_time)
{
	int err = 0;
	int64_t date_time_ms;

	if (new_date_time == NULL) {
		LOG_ERR("The passed in pointer cannot be NULL");
		return -EINVAL;
	}

	/** Seconds after the minute. tm_sec is generally 0-59.
	 *  The extra range is to accommodate for leap seconds
	 *  in certain systems.
	 */
	if (new_date_time->tm_sec < 0 || new_date_time->tm_sec > 61) {
		LOG_ERR("Seconds in time structure not in correct format");
		err = -EINVAL;
	}

	/** Minutes after the hour. */
	if (new_date_time->tm_min < 0 || new_date_time->tm_min > 59) {
		LOG_ERR("Minutes in time structure not in correct format");
		err = -EINVAL;
	}

	/** Hours since midnight. */
	if (new_date_time->tm_hour < 0 || new_date_time->tm_hour > 23) {
		LOG_ERR("Hours in time structure not in correct format");
		err = -EINVAL;
	}

	/** Day of the month. */
	if (new_date_time->tm_mday < 1 || new_date_time->tm_mday > 31) {
		LOG_ERR("Day in time structure not in correct format");
		err = -EINVAL;
	}

	/** Months since January. */
	if (new_date_time->tm_mon < 0 || new_date_time->tm_mon > 11) {
		LOG_ERR("Month in time structure not in correct format");
		err = -EINVAL;
	}

	/** Years since 1900. 115 corresponds to the year 2015. */
	if (new_date_time->tm_year < 115 || new_date_time->tm_year > 1900) {
		LOG_ERR("Year in time structure not in correct format");
		err = -EINVAL;
	}

	if (err) {
		return err;
	}

	date_time_ms = (int64_t)timeutil_timegm64(new_date_time) * 1000;

	date_time_core_store(date_time_ms, DATE_TIME_OBTAINED_EXT);

	return 0;
}

int date_time_uptime_to_unix_time_ms(int64_t *uptime)
{
	int64_t uptime_now;
	int64_t date_time_ms;
	int err;

	if (uptime == NULL) {
		LOG_ERR("The passed in pointer cannot be NULL");
		return -EINVAL;
	}

	if (*uptime < 0) {
		LOG_ERR("Uptime cannot be negative");
		return -EINVAL;
	}

	uptime_now = k_uptime_get();
	err = date_time_now(&date_time_ms);
	if (err) {
		return err;
	}

	/* Check if the passed in uptime was already converted. */
	if (*uptime > uptime_now) {
		LOG_WRN("Uptime too large or previously converted");
		LOG_WRN("Clear variable or set a new uptime");
		return -EINVAL;
	}

	*uptime += date_time_ms - uptime_now;

	return 0;
}

int date_time_now(int64_t *unix_time_ms)
{
	int err;

	if (unix_time_ms == NULL) {
		LOG_ERR("The passed in pointer cannot be NULL");
		return -EINVAL;
	}
	if (!date_time_is_valid()) {
		LOG_WRN("Valid time not currently available");
		return -ENODATA;
	}

	err = date_time_core_now(unix_time_ms);

	return err;
}

int date_time_now_local(int64_t *local_time_ms)
{
	if (local_time_ms == NULL) {
		LOG_ERR("The passed in pointer cannot be NULL");
		return -EINVAL;
	}
	if (!date_time_is_valid()) {
		LOG_WRN("Valid time not currently available");
		return -ENODATA;
	}

	return date_time_core_now_local(local_time_ms);
}

bool date_time_is_valid(void)
{
	return date_time_core_is_valid();
}

bool date_time_is_valid_local(void)
{
	return date_time_core_is_valid_local();
}

void date_time_register_handler(date_time_evt_handler_t evt_handler)
{
	date_time_core_register_handler(evt_handler);
}

int date_time_update_async(date_time_evt_handler_t evt_handler)
{
	return date_time_core_update_async(evt_handler);
}

void date_time_clear(void)
{
	date_time_core_clear();
}

int date_time_timestamp_clear(int64_t *unix_timestamp)
{
	if (unix_timestamp == NULL) {
		LOG_ERR("The passed in pointer cannot be NULL");
		return -EINVAL;
	}

	*unix_timestamp = 0;

	return 0;
}

static int date_time_init(void)
{
	date_time_core_init();

	return 0;
}

SYS_INIT(date_time_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
