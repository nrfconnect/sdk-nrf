/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <date_time.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>
#include <mock_nrf_modem_at.h>
#include <nrf_errno.h>

#include "cmock_nrf_modem_at.h"

#define TEST_EVENT_MAX_COUNT 10

struct test_date_time_cb {
	enum date_time_evt_type type;
};

struct test_date_time_cb test_date_time_cb_data[TEST_EVENT_MAX_COUNT] = {0};

static int date_time_cb_count_occurred;
static int date_time_cb_count_expected;

static void date_time_callback(const struct date_time_evt *evt);

/* at_monitor_dispatch() is implemented in at_monitor library and
 * we'll call it directly to fake received AT commands/notifications
 */
extern void at_monitor_dispatch(const char *at_notif);

/* date_time_modem_on_cfun() is implemented in date_time library and
 * we'll call it directly to fake nrf_modem_lib call to this function
 */
extern void date_time_modem_on_cfun(int mode, void *ctx);

K_SEM_DEFINE(date_time_callback_sem, 0, 1);

void setUp(void)
{
	mock_nrf_modem_at_Init();

	date_time_cb_count_occurred = 0;
	date_time_cb_count_expected = 0;

	date_time_register_handler(date_time_callback);
}

void tearDown(void)
{
	TEST_ASSERT_EQUAL(date_time_cb_count_expected, date_time_cb_count_occurred);

	date_time_register_handler(NULL);

	mock_nrf_modem_at_Verify();
}

/** Callback that date_time library will call when an event is received. */
static void date_time_callback(const struct date_time_evt *evt)
{
	TEST_ASSERT_MESSAGE(date_time_cb_count_occurred < date_time_cb_count_expected,
			    "date-time event callback called more times than expected");

	TEST_ASSERT_EQUAL(test_date_time_cb_data[date_time_cb_count_occurred].type, evt->type);

	date_time_cb_count_occurred++;
	k_sem_give(&date_time_callback_sem);
}

static void reset_to_valid_time(struct tm *time)
{
	time->tm_year = 120;
	time->tm_mon = 7;
	time->tm_mday = 7;
	time->tm_hour = 15;
	time->tm_min = 11;
	time->tm_sec = 30;
}

void test_date_time_invalid_input(void)
{
	int ret;
	struct tm date_time_dummy;

	reset_to_valid_time(&date_time_dummy);

	/** Invalid year. */
	date_time_dummy.tm_year = 2020;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_year = 114;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid month. */
	date_time_dummy.tm_mon = 12;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_mon = -1;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid day. */
	date_time_dummy.tm_mday = 0;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_mday = 32;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid hour. */
	date_time_dummy.tm_hour = -1;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_hour = 24;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid minutes. */
	date_time_dummy.tm_min = -1;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_min = 60;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/**********************************************************************/

	/** Invalid seconds. */
	date_time_dummy.tm_sec = -1;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_sec = 62;

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

}

void test_date_time_null_input(void)
{
	int ret;

	ret = date_time_set(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = date_time_uptime_to_unix_time_ms(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = date_time_now(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = date_time_now_local(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = date_time_timestamp_clear(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_date_time_premature_request(void)
{
	int ret;
	int64_t ts_unix_ms = 0;

	ret = date_time_now(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
	TEST_ASSERT_EQUAL(0, ts_unix_ms);

	ret = date_time_uptime_to_unix_time_ms(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
	TEST_ASSERT_EQUAL(0, ts_unix_ms);

}

void test_date_time_already_converted(void)
{
	/* Wait to get uptime non-zero for date_time_uptime_to_unix_time_ms() call */
	k_sleep(K_MSEC(10));
	date_time_register_handler(NULL);
	date_time_clear();
	int ret;

	date_time_register_handler(NULL);

	struct tm date_time_dummy = {
		.tm_year = 120,
		.tm_mon = 7,
		.tm_mday = 7,
		.tm_hour = 15,
		.tm_min = 11,
		.tm_sec = 30
	};

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CCLK=\"20/08/07,15:11:30+99\"", 0);

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(0, ret);

	/** Fri Aug 07 2020 15:11:30 UTC. */
	int64_t ts_unix_ms = 1596813090000;
	int64_t ts_unix_ms_prev = 1596813090000;

	ret = date_time_uptime_to_unix_time_ms(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	TEST_ASSERT_EQUAL(ts_unix_ms_prev, ts_unix_ms);
}

void test_date_time_negative_uptime(void)
{
	int ret;

	int64_t ts_unix_ms = -1000;

	ret = date_time_uptime_to_unix_time_ms(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_date_time_clear(void)
{
	int ret;

	/** Fri Aug 07 2020 15:11:30 UTC. */
	int64_t ts_unix_ms = 1596813090000;

	ret = date_time_timestamp_clear(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = date_time_timestamp_clear(&ts_unix_ms);
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(0, ts_unix_ms);
}

void test_date_time_conversion(void)
{
	date_time_register_handler(NULL);
	date_time_clear();

	int ret;
	struct tm date_time_dummy = {
		.tm_year = 120,
		.tm_mon = 7,
		.tm_mday = 7,
		.tm_hour = 15,
		.tm_min = 11,
		.tm_sec = 30
	};

	/** UNIX timestamp equivalent to tm structure date_time_dummy. */
	/** Fri Aug 07 2020 15:11:30 UTC. */
	int64_t date_time_utc_unix = 1596813090000;
	int64_t date_time_utc_unix_origin = k_uptime_get();
	int64_t uptime = k_uptime_get();
	int64_t ts_unix_ms = 0;
	int64_t ts_expect = 0;

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CCLK=\"20/08/07,15:11:30+99\"", 0);

	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(0, ret);

	ret = date_time_now(&ts_unix_ms);
	TEST_ASSERT_EQUAL(0, ret);

	ts_expect = date_time_utc_unix - date_time_utc_unix_origin + k_uptime_get();

	/* We cannot compare exact conversions given by the date time library due to the fact that
	 * the comparing values are based on k_uptime_get(). Use range instead and compare agains an
	 * arbitrary "high" delta, just to be sure.
	 */
	TEST_ASSERT_INT64_WITHIN(100, ts_expect, ts_unix_ms);

	ret = date_time_timestamp_clear(&ts_unix_ms);
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_EQUAL(0, ts_unix_ms);

	uptime = 0;

	ret = date_time_uptime_to_unix_time_ms(&uptime);
	TEST_ASSERT_EQUAL(0, ret);

	ts_expect = date_time_utc_unix - date_time_utc_unix_origin;

	TEST_ASSERT_INT64_WITHIN(100, ts_expect, uptime);

	uptime = k_uptime_get();

	ret = date_time_uptime_to_unix_time_ms(&uptime);
	TEST_ASSERT_EQUAL(0, ret);

	ts_expect = date_time_utc_unix - date_time_utc_unix_origin + k_uptime_get();

	TEST_ASSERT_INT64_WITHIN(100, ts_expect, uptime);

	k_sleep(K_SECONDS(1));

	uptime = k_uptime_get();

	ret = date_time_uptime_to_unix_time_ms(&uptime);
	TEST_ASSERT_EQUAL(0, ret);

	ts_expect = date_time_utc_unix - date_time_utc_unix_origin + k_uptime_get();

	TEST_ASSERT_INT64_WITHIN(100, ts_expect, uptime);
}

void test_date_time_validity(void)
{
	/* Wait to get uptime non-zero for last date_time_is_valid() call */
	k_sleep(K_MSEC(10));
	date_time_register_handler(NULL);
	date_time_clear();

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
	TEST_ASSERT_EQUAL(false, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CCLK=\"20/08/07,15:11:30+99\"", 0);

	/** UNIX timestamp equavivalent to tm structure date_time_dummy. */
	/** Fri Aug 07 2020 15:11:30 UTC. */
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(0, ret);

	ret = date_time_is_valid();
	TEST_ASSERT_EQUAL(true, ret);
}

/* This is needed because AT Monitor library is initialized in SYS_INIT. */
static int date_time_test_sys_init(void)
{
	__cmock_nrf_modem_at_notif_handler_set_ExpectAnyArgsAndReturn(0);

	return 0;
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}

SYS_INIT(date_time_test_sys_init, POST_KERNEL, 0);
