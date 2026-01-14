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
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <date_time.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>
#include <mock_nrf_modem_at.h>
#include <nrf_errno.h>

#include "cmock_nrf_modem_at.h"
#include "cmock_socket.h"
#include "cmock_sntp.h"

/* NOTE: These tests run for few tens of seconds because we are waiting for the
 * date-time update and retry intervals.
 */

#define TEST_EVENT_MAX_COUNT 10
#define TEST_CB_WAIT_TIME (CONFIG_DATE_TIME_UPDATE_INTERVAL_SECONDS * 1000 + 1000)

static void date_time_callback(const struct date_time_evt *evt);

/* at_monitor_dispatch() is implemented in at_monitor library and
 * we'll call it directly to fake received AT commands/notifications
 */
extern void at_monitor_dispatch(const char *at_notif);

/* date_time_modem_on_cfun() is implemented in date_time library and
 * we'll call it directly to fake nrf_modem_lib call to this function
 */
extern void date_time_modem_on_cfun(int mode, void *ctx);

struct test_date_time_cb {
	enum date_time_evt_type type;
	int64_t uptime_start;
	int64_t uptime_current;
	int64_t time_expected;
};

static struct test_date_time_cb test_date_time_cb_data[TEST_EVENT_MAX_COUNT] = {0};

static int date_time_cb_count_occurred;
static int date_time_cb_count_expected;

static K_SEM_DEFINE(date_time_callback_sem, 0, 1);

static char at_notif[128];

/** Fri Aug 07 2020 15:11:30 UTC. */
static struct tm date_time_global = {
	.tm_year = 120,
	.tm_mon = 7,
	.tm_mday = 7,
	.tm_hour = 15,
	.tm_min = 11,
	.tm_sec = 30
};
/** UNIX timestamp equivalent to tm structure date_time_global. */
static int64_t date_time_global_unix = 1596813090000;

#if defined(CONFIG_DATE_TIME_NTP)
static struct net_sockaddr ai_addr_uio_ntp = {
	.sa_family = NET_AF_INET,
	.data = "100.101.102.103"
};

static struct zsock_addrinfo res_data_uio_ntp = {
	.ai_addr = &ai_addr_uio_ntp,
	.ai_addrlen = sizeof(struct net_sockaddr),
};

static struct zsock_addrinfo *res_uio_ntp = &res_data_uio_ntp;

static struct net_sockaddr ai_addr_google_ntp = {
	.sa_family = NET_AF_INET,
	.data = "1.2.3.4"
};

static struct zsock_addrinfo res_data_google_ntp = {
	.ai_addr = &ai_addr_google_ntp,
	.ai_addrlen = sizeof(struct net_sockaddr),
};

static struct zsock_addrinfo *res_google_ntp = &res_data_google_ntp;

static struct zsock_addrinfo hints = {
	.ai_flags = ZSOCK_AI_NUMERICSERV,
	.ai_family = NET_AF_UNSPEC /* Allow both IPv4 and IPv6 addresses */
};

static struct sntp_time sntp_time_value = {
	.seconds = 42900180919421
};
#endif

void setUp(void)
{
	mock_nrf_modem_at_Init();

	memset(test_date_time_cb_data, 0, sizeof(test_date_time_cb_data));

	date_time_cb_count_occurred = 0;
	date_time_cb_count_expected = 0;

	test_date_time_cb_data[0].uptime_start = k_uptime_get();

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

	TEST_ASSERT_MESSAGE(date_time_cb_count_occurred < TEST_EVENT_MAX_COUNT,
			    "date-time event callback called more times than TEST_EVENT_MAX_COUNT");

	TEST_ASSERT_EQUAL(test_date_time_cb_data[date_time_cb_count_occurred].type, evt->type);

	/* Check time when callback occurred compared to expected time since the test case start */
	TEST_ASSERT_INT64_WITHIN(
		100,
		test_date_time_cb_data[date_time_cb_count_occurred].time_expected,
		k_uptime_get() - test_date_time_cb_data[0].uptime_start);

	date_time_cb_count_occurred++;
	k_sem_give(&date_time_callback_sem);
}

/** Reset to: Fri Aug 07 2020 15:11:30 UTC. */
static void reset_to_valid_time(struct tm *time)
{
	time->tm_year = 120;
	time->tm_mon = 7;
	time->tm_mday = 7;
	time->tm_hour = 15;
	time->tm_min = 11;
	time->tm_sec = 30;
}

void test_date_time_set_invalid_input_fail(void)
{
	int ret;
	struct tm date_time_dummy;

	reset_to_valid_time(&date_time_dummy);

	/** Invalid year */
	date_time_dummy.tm_year = 2020;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_year = 114;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/** Invalid month */
	date_time_dummy.tm_mon = 12;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_mon = -1;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/** Invalid day */
	date_time_dummy.tm_mday = 0;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_mday = 32;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/** Invalid hour */
	date_time_dummy.tm_hour = -1;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_hour = 24;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/** Invalid minutes. */
	date_time_dummy.tm_min = -1;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_min = 60;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);

	/** Invalid seconds */
	date_time_dummy.tm_sec = -1;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	date_time_dummy.tm_sec = 62;
	ret = date_time_set(&date_time_dummy);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	reset_to_valid_time(&date_time_dummy);
}

void test_date_time_null_input_fail(void)
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

void test_date_time_no_valid_time_fail(void)
{
	int ret;
	int64_t ts_unix_ms = 0;

	ret = date_time_now(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
	TEST_ASSERT_EQUAL(0, ts_unix_ms);

	ret = date_time_now_local(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
	TEST_ASSERT_EQUAL(0, ts_unix_ms);

	ret = date_time_uptime_to_unix_time_ms(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
	TEST_ASSERT_EQUAL(0, ts_unix_ms);
}

void test_date_time_uptime_to_unix_time_ms_already_converted(void)
{
	int ret;

	/* Wait to get uptime non-zero for date_time_uptime_to_unix_time_ms() call */
	k_sleep(K_MSEC(10));
	date_time_register_handler(NULL);
	date_time_clear();

#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CCLK=\"20/08/07,15:11:30+99\"", 0);
#endif
	ret = date_time_set(&date_time_global);
	TEST_ASSERT_EQUAL(0, ret);

	/** Fri Aug 07 2020 15:11:30 UTC. */
	int64_t ts_unix_ms = date_time_global_unix;

	ret = date_time_uptime_to_unix_time_ms(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	TEST_ASSERT_EQUAL(date_time_global_unix, ts_unix_ms);
}

void test_date_time_uptime_to_unix_time_ms_negative_fail(void)
{
	int ret;

	int64_t ts_unix_ms = -1000;

	ret = date_time_uptime_to_unix_time_ms(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_date_time_timestamp_clear(void)
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

void test_date_time_uptime_to_unix_time_ms(void)
{
	int ret;
	int64_t date_time_global_unix_origin = k_uptime_get();
	int64_t uptime = k_uptime_get();
	int64_t ts_unix_ms = 0;
	int64_t ts_expect = 0;

	date_time_register_handler(NULL);
	date_time_clear();

#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CCLK=\"20/08/07,15:11:30+99\"", 0);
#endif
	ret = date_time_set(&date_time_global);
	TEST_ASSERT_EQUAL(0, ret);

	ret = date_time_now(&ts_unix_ms);
	TEST_ASSERT_EQUAL(0, ret);

	ts_expect = date_time_global_unix - date_time_global_unix_origin + k_uptime_get();

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

	ts_expect = date_time_global_unix - date_time_global_unix_origin;

	TEST_ASSERT_INT64_WITHIN(100, ts_expect, uptime);

	uptime = k_uptime_get();

	ret = date_time_uptime_to_unix_time_ms(&uptime);
	TEST_ASSERT_EQUAL(0, ret);

	ts_expect = date_time_global_unix - date_time_global_unix_origin + k_uptime_get();

	TEST_ASSERT_INT64_WITHIN(100, ts_expect, uptime);

	k_sleep(K_SECONDS(1));

	uptime = k_uptime_get();

	ret = date_time_uptime_to_unix_time_ms(&uptime);
	TEST_ASSERT_EQUAL(0, ret);

	ts_expect = date_time_global_unix - date_time_global_unix_origin + k_uptime_get();

	TEST_ASSERT_INT64_WITHIN(100, ts_expect, uptime);
}

void test_date_time_is_valid(void)
{
	int ret;
	int64_t ts_unix_ms = 0;

	/* Wait to get uptime non-zero for last date_time_is_valid() call */
	k_sleep(K_MSEC(10));
	date_time_register_handler(NULL);
	date_time_clear();

	ret = date_time_is_valid();
	TEST_ASSERT_EQUAL(false, ret);

	/* Local time not valid here because there is no valid time */
	ret = date_time_is_valid_local();
	TEST_ASSERT_EQUAL(false, ret);

#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CCLK=\"20/08/07,15:11:30+99\"", 0);
#endif
	ret = date_time_set(&date_time_global);
	TEST_ASSERT_EQUAL(0, ret);

	ret = date_time_is_valid();
	TEST_ASSERT_EQUAL(true, ret);

	/* Local time still not valid here because timezone is not set with date_time_set()
	 * but it requires modem time so that's checked elsewhere.
	 */
	ret = date_time_is_valid_local();
	TEST_ASSERT_EQUAL(false, ret);

	ret = date_time_now(&ts_unix_ms);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_INT64_WITHIN(100, date_time_global_unix, ts_unix_ms);

	ts_unix_ms = 0;
	ret = date_time_now_local(&ts_unix_ms);
	TEST_ASSERT_EQUAL(-EAGAIN, ret);
	TEST_ASSERT_EQUAL(0, ts_unix_ms);
}

void test_date_time_xtime_subscribe_fail(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM)
	TEST_IGNORE();
#endif
	/* %XTIME subscription not sent with wrong functional mode */
	date_time_modem_on_cfun(LTE_LC_FUNC_MODE_OFFLINE, NULL);
	k_sleep(K_MSEC(1));

	/* %XTIME subscription fails */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XTIME=1", -ENOMEM);
	date_time_modem_on_cfun(LTE_LC_FUNC_MODE_NORMAL, NULL);
	k_sleep(K_MSEC(1));
}

/**
 * Test initial auto update 1 second from the boot.
 * Using modem or NTP time depending on the configuration.
 * NTP is used if both are configured because modem time is used in that case only
 * if XTIME notification has been received, which is not the case here.
 *
 * Note: This test must be before any %XTIME notification tests because it will set
 *       modem_valid_network_time and we cannot revert it back to 'false' anymore.
 */
void test_date_time_initial_auto_update_at_1s_from_boot(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM) && !defined(CONFIG_DATE_TIME_NTP)
	TEST_IGNORE();
#endif
	date_time_cb_count_expected = 1;
#if defined(CONFIG_DATE_TIME_NTP)
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_NTP;
#else /* CONFIG_DATE_TIME_MODEM */
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_MODEM;
#endif
	test_date_time_cb_data[0].time_expected = 1000;
	date_time_clear();

#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XTIME=1", 0);
	date_time_modem_on_cfun(LTE_LC_FUNC_MODE_NORMAL, NULL);

	k_sleep(K_MSEC(1));
#endif

#if defined(CONFIG_DATE_TIME_NTP)
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_REGISTERED_HOME);

	__cmock_zsock_getaddrinfo_ExpectAndReturn("ntp.uio.no", "123", &hints, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&res_uio_ntp);

	__cmock_sntp_init_ExpectAndReturn(NULL, &ai_addr_uio_ntp, sizeof(struct net_sockaddr), 0);
	__cmock_sntp_init_IgnoreArg_ctx();

	__cmock_sntp_query_ExpectAndReturn(NULL, 5000, NULL, 0);
	__cmock_sntp_query_IgnoreArg_ctx();
	__cmock_sntp_query_IgnoreArg_ts();
	__cmock_sntp_query_ReturnThruPtr_ts(&sntp_time_value);

	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();
	__cmock_sntp_close_ExpectAnyArgs();
#else /* CONFIG_DATE_TIME_MODEM */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", 7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(24);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(30);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(20);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(40);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(52);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(12);
#endif

#if defined(CONFIG_DATE_TIME_MODEM) && defined(CONFIG_DATE_TIME_NTP)
	/* Setting modem time fails */
	__mock_nrf_modem_at_printf_ExpectAndReturn("AT+CCLK=\"22/08/13,18:03:41+99\"", -ENOMEM);
#endif
	strcpy(at_notif, "+CEREG: 1,\"002F\",\"0012BEEF\",7,,,\"00000101\",\"00010011\"\r\n");
	at_monitor_dispatch(at_notif);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));
}

/** Test initial auto update from modem. */
void test_date_time_initial_auto_update_xtime_notif(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM)
	TEST_IGNORE();
#endif
	int ret;

	date_time_cb_count_expected = 1;
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_MODEM;

	date_time_clear();

	ret = date_time_is_valid();
	TEST_ASSERT_EQUAL(false, ret);

	ret = date_time_is_valid_local();
	TEST_ASSERT_EQUAL(false, ret);

	__mock_nrf_modem_at_printf_ExpectAndReturn("AT%XTIME=1", 0);
	date_time_modem_on_cfun(LTE_LC_FUNC_MODE_ACTIVATE_LTE, NULL);

	k_sleep(K_MSEC(1));

	/* Negative timezone. */
	strcpy(at_notif, "%XTIME: \"0A\",\"4290018091940A\",\"01\"\r\n");
	at_monitor_dispatch(at_notif);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	ret = date_time_is_valid();
	TEST_ASSERT_EQUAL(true, ret);

	ret = date_time_is_valid_local();
	TEST_ASSERT_EQUAL(true, ret);
}

/** Test update from modem. */
void test_date_time_1st_update_modem(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM)
	TEST_IGNORE();
#endif
	date_time_cb_count_expected = 1;
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_MODEM;
	test_date_time_cb_data[0].time_expected = 3000;

	/* Not home or roaming so ignored. */
	strcpy(at_notif, "+CEREG: 2\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	strcpy(at_notif, "+CEREG: 5,\"002F\",\"0012BEEF\",7,,,\"00000101\",\"00010011\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	/* %XTIME notification ignored because current time is not too old */
	k_sleep(K_MSEC(300));
	strcpy(at_notif, "%XTIME: ,\"42900180919421\",\"01\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", 7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(24);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(30);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(20);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(40);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(52);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(12);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));
}

/* This is after XTIME test because we get timezone there so local time works. */
void test_date_time_now_local(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM)
	TEST_IGNORE();
#endif
	int ret;
	int64_t ts_unix_ms = 0;

	ret = date_time_is_valid_local();
	TEST_ASSERT_EQUAL(true, ret);

	ret = date_time_now_local(&ts_unix_ms);
	TEST_ASSERT_EQUAL(0, ret);

	/* TODO: Verify the time */
}

/** Test initial auto update from modem. */
void test_date_time_use_previously_obtained_time(void)
{
	/* Previously found time is available and source is the same */
	date_time_cb_count_expected = 1;
#if defined(CONFIG_DATE_TIME_MODEM)
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_MODEM;
#elif defined(CONFIG_DATE_TIME_NTP)
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_NTP;
#else
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_EXT;
#endif

	date_time_update_async(NULL);
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));
}

K_SEM_DEFINE(date_time_callback_custom_sem, 0, 1);

/** Callback that date_time library will call when an event is received. */
static void date_time_callback_custom(const struct date_time_evt *evt)
{
	/* Verify that type is previously returned type */
#if defined(CONFIG_DATE_TIME_MODEM)
	TEST_ASSERT_EQUAL(DATE_TIME_OBTAINED_MODEM, evt->type);
#elif defined(CONFIG_DATE_TIME_NTP)
	TEST_ASSERT_EQUAL(DATE_TIME_OBTAINED_NTP, evt->type);
#endif
	k_sem_give(&date_time_callback_custom_sem);
}

void test_date_time_update_async_different_handler(void)
{
	/* Normal handler not used in this test case so no callbacks there */
	date_time_cb_count_expected = 0;

	/* Custom date-time callback handler */
	date_time_update_async(date_time_callback_custom);
	k_sem_take(&date_time_callback_custom_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* No date-time callback handler at all. We cannot really verify that the procedure has
	 * been executed properly but at least we can check that it doesn't crash or send events.
	 */
	date_time_register_handler(NULL);
	date_time_update_async(NULL);
	k_sleep(K_MSEC(1));
}

void test_date_time_set_dont_set_modem_time_when_it_exists(void)
{
	int ret;
	int64_t ts_unix_ms = 0;

	date_time_cb_count_expected = 1;
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_EXT;
	test_date_time_cb_data[0].time_expected = 0;

	/* Modem has time so check that modem time is not set anymore with "AT+CCLK="*/
	ret = date_time_set(&date_time_global);
	TEST_ASSERT_EQUAL(0, ret);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	ret = date_time_now(&ts_unix_ms);
	TEST_ASSERT_EQUAL(0, ret);

	TEST_ASSERT_INT64_WITHIN(100, date_time_global_unix, ts_unix_ms);
}

void test_date_time_update_cycle_and_async(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM)
	TEST_IGNORE();
#endif
	date_time_cb_count_expected = 6;
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_MODEM;
	test_date_time_cb_data[0].time_expected = 3000;
	test_date_time_cb_data[1].type = DATE_TIME_OBTAINED_MODEM;
	test_date_time_cb_data[1].time_expected = 3500;
	test_date_time_cb_data[2].type = DATE_TIME_OBTAINED_MODEM;
	test_date_time_cb_data[2].time_expected = 5000;
	test_date_time_cb_data[3].type = DATE_TIME_OBTAINED_MODEM;
	test_date_time_cb_data[3].time_expected = 8000;
	test_date_time_cb_data[4].type = DATE_TIME_OBTAINED_MODEM;
	test_date_time_cb_data[4].time_expected = 9000;
	test_date_time_cb_data[5].type = DATE_TIME_OBTAINED_MODEM;
	test_date_time_cb_data[5].time_expected = 11000;

	/* Normal cycle at 3s */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", 7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(24);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(30);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(20);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(40);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(52);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(12);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* Asynchronous update with previous time at 3.5s */
	k_sleep(K_MSEC(500));
	date_time_update_async(NULL);
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* Asynchronous update at 5s*/
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", 6);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(24);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(30);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(20);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(40);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(52);

	k_sleep(K_MSEC(1500));
	date_time_update_async(NULL);
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* Normal cycle at 8s*/
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", 7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(24);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(30);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(20);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(40);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(52);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(12);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* Time taken into use from %XTIME notification at 9s */
	k_sleep(K_MSEC(1000));
	strcpy(at_notif, "%XTIME: ,\"42900180919421\",\"01\"\r\n");
	at_monitor_dispatch(at_notif);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* XTIME doesn't delay normal cycle unlike date_time_update_async */

	/* Normal cycle at 11s */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", 7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(24);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(30);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(20);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(40);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(52);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(12);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));
}

void test_date_time_update_cycle_with_failing_async(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM)
	TEST_IGNORE();
#endif
	date_time_cb_count_expected = 5;
	test_date_time_cb_data[0].type = DATE_TIME_OBTAINED_MODEM;
	test_date_time_cb_data[0].time_expected = 3000;
	test_date_time_cb_data[1].type = DATE_TIME_NOT_OBTAINED;
	test_date_time_cb_data[1].time_expected = 4000;
	test_date_time_cb_data[2].type = DATE_TIME_NOT_OBTAINED;
	test_date_time_cb_data[2].time_expected = 5000;
	test_date_time_cb_data[3].type = DATE_TIME_NOT_OBTAINED;
	test_date_time_cb_data[3].time_expected = 6000;
	test_date_time_cb_data[4].type = DATE_TIME_OBTAINED_MODEM;
	test_date_time_cb_data[4].time_expected = 9000;

	/* Normal cycle at 3s */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", 7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(24);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(30);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(20);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(40);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(52);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(12);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* Failing asynchronous update at 4s  */
	k_sleep(K_MSEC(1000));

	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", -ENOMEM);
#if defined(CONFIG_DATE_TIME_NTP)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", -ENOMEM);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_UNKNOWN);
#endif
	date_time_update_async(NULL);
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* Failing retry after asynchronous update at 5s  */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", -ENOMEM);
#if defined(CONFIG_DATE_TIME_NTP)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", -ENOMEM);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_UNKNOWN);
#endif
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* Failing retry after asynchronous update at 6s  */
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", -ENOMEM);
#if defined(CONFIG_DATE_TIME_NTP)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", -ENOMEM);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_UNKNOWN);
#endif
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* Normal cycle at 6s*/
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", 7);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(24);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(30);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(20);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(40);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(52);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint32(12);

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));
}

/** Test all retries failing with different NTP failures. */
void test_date_time_update_all_retry_ntp_fail(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM) && !defined(CONFIG_DATE_TIME_NTP)
	TEST_IGNORE();
#endif
	date_time_cb_count_expected = 3;
	test_date_time_cb_data[0].type = DATE_TIME_NOT_OBTAINED;
	test_date_time_cb_data[0].time_expected = 3000;
	test_date_time_cb_data[1].type = DATE_TIME_NOT_OBTAINED;
	test_date_time_cb_data[1].time_expected = 4000;
	test_date_time_cb_data[2].type = DATE_TIME_NOT_OBTAINED;
	test_date_time_cb_data[2].time_expected = 4500;

	/* 1st date-time update */
#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", -ENOMEM);
#endif
#if defined(CONFIG_DATE_TIME_NTP)
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_REGISTERED_HOME);

	/* NTP fails in NTP query for UIO */
	__cmock_zsock_getaddrinfo_ExpectAndReturn("ntp.uio.no", "123", &hints, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&res_uio_ntp);

	__cmock_sntp_init_ExpectAnyArgsAndReturn(0);
	__cmock_sntp_query_ExpectAndReturn(NULL, 5000, NULL, -1);
	__cmock_sntp_query_IgnoreArg_ctx();
	__cmock_sntp_query_IgnoreArg_ts();
	__cmock_sntp_query_ReturnThruPtr_ts(&sntp_time_value);

	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();
	__cmock_sntp_close_ExpectAnyArgs();

	/* NTP fails in DNS query for Google */
	__cmock_zsock_getaddrinfo_ExpectAndReturn("time.google.com", "123", &hints, NULL, -1);
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&res_google_ntp);
#endif
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* 1st retry */
#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", -ENOMEM);
#endif
#if defined(CONFIG_DATE_TIME_NTP)
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_REGISTERED_ROAMING);

	/* NTP fails in SNTP init for UIO */
	__cmock_zsock_getaddrinfo_ExpectAndReturn("ntp.uio.no", "123", &hints, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&res_uio_ntp);

	__cmock_sntp_init_ExpectAnyArgsAndReturn(-1);

	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();
	__cmock_sntp_close_ExpectAnyArgs();

	/* NTP fails in NTP query for Google */
	__cmock_zsock_getaddrinfo_ExpectAndReturn("time.google.com", "123", &hints, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&res_google_ntp);

	__cmock_sntp_init_ExpectAnyArgsAndReturn(0);
	__cmock_sntp_query_ExpectAndReturn(NULL, 5000, NULL, -1);
	__cmock_sntp_query_IgnoreArg_ctx();
	__cmock_sntp_query_IgnoreArg_ts();
	__cmock_sntp_query_ReturnThruPtr_ts(&sntp_time_value);

	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();
	__cmock_sntp_close_ExpectAnyArgs();
#endif
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* 2nd retry with asynchronous update at 4.5s */
#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", -ENOMEM);
#endif
#if defined(CONFIG_DATE_TIME_NTP)
	/* NTP not even tried because no connection to LTE network */
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_REGISTERED_HOME);

	/* NTP fails in NTP query for UIO */
	__cmock_zsock_getaddrinfo_ExpectAndReturn("ntp.uio.no", "123", &hints, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&res_uio_ntp);

	__cmock_sntp_init_ExpectAnyArgsAndReturn(0);
	__cmock_sntp_query_ExpectAndReturn(NULL, 5000, NULL, -1);
	__cmock_sntp_query_IgnoreArg_ctx();
	__cmock_sntp_query_IgnoreArg_ts();
	__cmock_sntp_query_ReturnThruPtr_ts(&sntp_time_value);

	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();
	__cmock_sntp_close_ExpectAnyArgs();

	/* NTP fails in NTP query for Google */
	__cmock_zsock_getaddrinfo_ExpectAndReturn("time.google.com", "123", &hints, NULL, 0);
	__cmock_zsock_getaddrinfo_IgnoreArg_res();
	__cmock_zsock_getaddrinfo_ReturnThruPtr_res(&res_google_ntp);

	__cmock_sntp_init_ExpectAnyArgsAndReturn(0);
	__cmock_sntp_query_ExpectAndReturn(NULL, 5000, NULL, -1);
	__cmock_sntp_query_IgnoreArg_ctx();
	__cmock_sntp_query_IgnoreArg_ts();
	__cmock_sntp_query_ReturnThruPtr_ts(&sntp_time_value);

	__cmock_zsock_freeaddrinfo_ExpectAnyArgs();
	__cmock_sntp_close_ExpectAnyArgs();
#endif
	k_sleep(K_MSEC(500));
	date_time_update_async(NULL);
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));
}

/* Test all retries failing due to LTE status checked by NTP procedure. */
void test_date_time_update_all_retry_ntp_ltestatus_fail(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM) && !defined(CONFIG_DATE_TIME_NTP)
	TEST_IGNORE();
#endif
	date_time_cb_count_expected = 3;
	test_date_time_cb_data[0].type = DATE_TIME_NOT_OBTAINED;
	test_date_time_cb_data[0].time_expected = 3000;
	test_date_time_cb_data[1].type = DATE_TIME_NOT_OBTAINED;
	test_date_time_cb_data[1].time_expected = 4000;
	test_date_time_cb_data[2].type = DATE_TIME_NOT_OBTAINED;
	test_date_time_cb_data[2].time_expected = 5000;

	/* 1st trial */
#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", -ENOMEM);
#endif
#if defined(CONFIG_DATE_TIME_NTP)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", -ENOMEM);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_UNKNOWN);
#endif
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* 1st retry */
#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", -ENOMEM);
#endif
#if defined(CONFIG_DATE_TIME_NTP)
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_UNKNOWN);
#endif
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));

	/* 2nd retry */
#if defined(CONFIG_DATE_TIME_MODEM)
	__mock_nrf_modem_at_scanf_ExpectAndReturn(
		"AT+CCLK?", "+CCLK: \"%u/%u/%u,%u:%u:%u%d", -ENOMEM);
#endif
#if defined(CONFIG_DATE_TIME_NTP)
	__mock_nrf_modem_at_scanf_ExpectAndReturn("AT+CEREG?", "+CEREG: %*u,%hu,%*[^,],\"%x\",", 1);
	__mock_nrf_modem_at_scanf_ReturnVarg_uint16(LTE_LC_NW_REG_SEARCHING);
#endif
	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));
}

/* Test that the scheduled updates are not done when modem and NTP time are disabled. */
void test_date_time_update_cycle_no_ntp_no_modem(void)
{
#if defined(CONFIG_DATE_TIME_MODEM) || defined(CONFIG_DATE_TIME_NTP)
	TEST_IGNORE();
#endif
	date_time_cb_count_expected = 0;

	k_sem_take(&date_time_callback_sem, K_MSEC(TEST_CB_WAIT_TIME));
}

/** Test initial auto update from modem. */
void test_date_time_xtime_notif_fail(void)
{
#if !defined(CONFIG_DATE_TIME_MODEM)
	TEST_IGNORE();
#endif
	/* No comma */
	strcpy(at_notif, "%XTIME: \"42900180919421\" \"01\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	/* Too short */
	strcpy(at_notif, "%XTIME: ,\"321\",\"01\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	/* Time value contains letters */
	strcpy(at_notif, "%XTIME: ,\"42900180moikka\",\"01\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));

	/* No double quotes for time value */
	strcpy(at_notif, "%XTIME: ,42900180919421,\"01\"\r\n");
	at_monitor_dispatch(at_notif);
	k_sleep(K_MSEC(1));
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
