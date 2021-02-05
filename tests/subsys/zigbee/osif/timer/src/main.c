/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>

#include <zboss_api.h>
#include <zb_osif_platform.h>

#define ZB_BEACON_INTERVAL_USEC 15360

static zb_uint32_t alarm_counter;

/* mock for timer alarm handler */
void zb_osif_zboss_timer_tick(void)
{
	alarm_counter++;
}

zb_uint32_t GetAlarmCount(void)
{
	return alarm_counter;
}

static void test_zb_osif_timer(void)
{
	ZB_START_HW_TIMER();
	zassert_true(ZB_CHECK_TIMER_IS_ON(), "Counter stopped");

	ZB_STOP_HW_TIMER();
	zassert_false(ZB_CHECK_TIMER_IS_ON(), "Counter running");

	ZB_START_HW_TIMER();
	uint32_t timestamp1 = zb_osif_timer_get();

	k_usleep(500);
	uint32_t timestamp2 = zb_osif_timer_get();

	zassert_true((timestamp2 > timestamp1),
		     "Timer is not incrementing");

	ZB_START_HW_TIMER();
	uint32_t alarm_count = GetAlarmCount();

	k_usleep(ZB_BEACON_INTERVAL_USEC);
	uint32_t new_alarm_count = GetAlarmCount();

	zassert_true((alarm_count != new_alarm_count),
		     "Alarm handler has not occurred");
}

void test_main(void)
{
	ztest_test_suite(osif_test,
			 ztest_unit_test(test_zb_osif_timer)
			 );

	ztest_run_test_suite(osif_test);
}
