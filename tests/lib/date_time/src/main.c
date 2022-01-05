/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <stdio.h>
#include <string.h>
#include <date_time.h>
#include <time.h>

static void reset_to_valid_time(struct tm *time)
{
	time->tm_year = 120;
	time->tm_mon = 7;
	time->tm_mday = 7;
	time->tm_hour = 15;
	time->tm_min = 11;
	time->tm_sec = 30;
}

static void test_date_time_invalid_input(void)
{
	int ret;
	struct tm date_time_dummy;

	reset_to_valid_time(&date_time_dummy);

	/** Invalid year. */
	date_time_dummy.tm_year = 2020;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	date_time_dummy.tm_year = 114;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid month. */
	date_time_dummy.tm_mon = 12;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	date_time_dummy.tm_mon = -1;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid day. */
	date_time_dummy.tm_mday = 0;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	date_time_dummy.tm_mday = 32;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid hour. */
	date_time_dummy.tm_hour = -1;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	date_time_dummy.tm_hour = 24;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid minutes. */
	date_time_dummy.tm_min = -1;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	date_time_dummy.tm_min = 60;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid seconds. */
	date_time_dummy.tm_sec = -1;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	date_time_dummy.tm_sec = 62;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(-EINVAL, ret, "date_time_set should equal -EINVAL");

	reset_to_valid_time(&date_time_dummy);

}

static void test_date_time_premature_request(void)
{
	int ret;
	int64_t ts_unix_ms = 0;

	ret = date_time_now(&ts_unix_ms);
	zassert_equal(-ENODATA, ret, "date_time_now should return -ENODATA");
	zassert_equal(0, ts_unix_ms, "ts_unix_ms should equal 0");

	ret = date_time_uptime_to_unix_time_ms(&ts_unix_ms);
	zassert_equal(-ENODATA, ret,
		"date_time_uptime_to_unix_time_ms should return -ENODATA");
	zassert_equal(0, ts_unix_ms, "ts_unix_ms should equal 0");

}

static void test_date_time_already_converted(void)
{
	int ret;

	struct tm date_time_dummy = {
		.tm_year = 120,
		.tm_mon = 7,
		.tm_mday = 7,
		.tm_hour = 15,
		.tm_min = 11,
		.tm_sec = 30
	};

	ret = date_time_set(&date_time_dummy);
	zassert_equal(0, ret, "date_time_set should equal 0");

	/** Fri Aug 07 2020 15:11:30 UTC. */
	int64_t ts_unix_ms = 1596813090000;
	int64_t ts_unix_ms_prev = 1596813090000;

	ret = date_time_uptime_to_unix_time_ms(&ts_unix_ms);
	zassert_equal(-EINVAL, ret,
		      "date_time_uptime_to_unix_time_ms should return -EINVAL");

	zassert_equal(ts_unix_ms_prev, ts_unix_ms,
		      "ts_unix_ms should equal ts_unix_ms_prev");
}

static void test_date_time_negative_uptime(void)
{
	int ret;

	int64_t ts_unix_ms = -1000;

	ret = date_time_uptime_to_unix_time_ms(&ts_unix_ms);
	zassert_equal(-EINVAL, ret,
		      "date_time_uptime_to_unix_time_ms should return -EINVAL");
}

static void test_date_time_clear(void)
{
	int ret;

	/** Fri Aug 07 2020 15:11:30 UTC. */
	int64_t ts_unix_ms = 1596813090000;

	ret = date_time_timestamp_clear(NULL);
	zassert_equal(-EINVAL, ret,
		      "date_time_timestamp_clear should return -EINVAL");

	ret = date_time_timestamp_clear(&ts_unix_ms);
	zassert_equal(0, ret, "date_time_timestamp_clear should return 0");

	zassert_equal(0, ts_unix_ms, "ts_unix_ms should equal 0");
}

static void test_date_time_conversion(void)
{
	int ret;
	struct tm date_time_dummy = {
		.tm_year = 120,
		.tm_mon = 7,
		.tm_mday = 7,
		.tm_hour = 15,
		.tm_min = 11,
		.tm_sec = 30
	};

	/** UNIX timestamp equavivalent to tm structure date_time_dummy. */
	/** Fri Aug 07 2020 15:11:30 UTC. */
	int64_t date_time_utc_unix = 1596813090000;
	int64_t date_time_utc_unix_origin = k_uptime_get();
	int64_t uptime = k_uptime_get();
	int64_t ts_unix_ms = 0;
	int64_t ts_expect = 0;

	ret = date_time_set(&date_time_dummy);
	zassert_equal(0, ret, "date_time_set should equal 0");

	ret = date_time_now(&ts_unix_ms);
	zassert_equal(0, ret, "date_time_now should equal 0");

	ts_expect = date_time_utc_unix + k_uptime_get() -
			date_time_utc_unix_origin;

	/* We cannot compare exact conversions given by the date time library due to the fact that
	 * the comparing values are based on k_uptime_get(). Use range instead and compare agains an
	 * arbitray "high" delta, just to be sure.
	 */
	zassert_within(ts_expect, ts_unix_ms, 500,
		       "Converted value should be within 500ms of the expected result");

	ret = date_time_timestamp_clear(&ts_unix_ms);
	zassert_equal(0, ret, "date_time_timestamp_clear should return 0");

	ret = date_time_uptime_to_unix_time_ms(&uptime);
	zassert_equal(0, ret,
		      "date_time_uptime_to_unix_time_ms should return 0");

	ts_expect = date_time_utc_unix_origin + date_time_utc_unix -
			date_time_utc_unix_origin;

	zassert_within(ts_expect, uptime, 500,
		       "Converted value should be within 500ms of the expected result");
}

static void test_date_time_validity(void)
{
	int ret;
	struct tm date_time_dummy = {
		.tm_year = 120,
		.tm_mon = 7,
		.tm_mday = 7,
		.tm_hour = 15,
		.tm_min = 11,
		.tm_sec = 30
	};

	ret = date_time_is_valid();
	zassert_equal(false, ret, "date_time_is_valid should equal false");

	/** UNIX timestamp equavivalent to tm structure date_time_dummy. */
	/** Fri Aug 07 2020 15:11:30 UTC. */
	ret = date_time_set(&date_time_dummy);
	zassert_equal(0, ret, "date_time_set should equal 0");

	ret = date_time_is_valid();
	zassert_equal(true, ret, "date_time_is_valid should equal true");
}

static void test_date_time_setup(void)
{
	/** */
}

static void test_date_time_teardown(void)
{
	date_time_clear();
}

void test_main(void)
{
	/* Delay to ensure that k_uptime_get returns positive non-zero value
	 * irrespective of what CONFIG_SYS_CLOCK_TICKS_PER_SEC value is.
	 * This has been an issue in QEMU tests.
	 */
	k_sleep(K_SECONDS(1));

	ztest_test_suite(test_date_time,
		ztest_unit_test_setup_teardown(
					test_date_time_invalid_input,
					test_date_time_setup,
					test_date_time_teardown),
		ztest_unit_test_setup_teardown(
					test_date_time_premature_request,
					test_date_time_setup,
					test_date_time_teardown),
		ztest_unit_test_setup_teardown(
					test_date_time_clear,
					test_date_time_setup,
					test_date_time_teardown),
		ztest_unit_test_setup_teardown(
					test_date_time_negative_uptime,
					test_date_time_setup,
					test_date_time_teardown),
		ztest_unit_test_setup_teardown(
					test_date_time_already_converted,
					test_date_time_setup,
					test_date_time_teardown),
		ztest_unit_test_setup_teardown(
					test_date_time_conversion,
					test_date_time_setup,
					test_date_time_teardown),
		ztest_unit_test_setup_teardown(
					test_date_time_validity,
					test_date_time_setup,
					test_date_time_teardown)
	);

	ztest_run_test_suite(test_date_time);
}
