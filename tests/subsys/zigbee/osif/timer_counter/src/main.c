/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <zboss_api.h>
#include <zb_osif_platform.h>

#define ZB_BEACON_INTERVAL_USEC 15360
#define ZB_BEACON_INTERVAL_MSEC ((ZB_BEACON_INTERVAL_USEC + 1000 - 1) / 1000)

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


ZTEST_SUITE(osif_test, NULL, NULL, NULL, NULL, NULL);

ZTEST(osif_test, test_zb_osif_timer)
{
	ZB_START_HW_TIMER();
	zassert_true(ZB_CHECK_TIMER_IS_ON(), "Counter stopped");

	ZB_STOP_HW_TIMER();
	zassert_false(ZB_CHECK_TIMER_IS_ON(), "Counter running");

	ZB_START_HW_TIMER();
	zb_uint64_t timestamp1 = osif_transceiver_time_get_long();

	k_usleep(500);
	zb_uint64_t timestamp2 = osif_transceiver_time_get_long();

	zassert_true((timestamp2 > timestamp1),
		     "Timer is not incrementing");

	ZB_START_HW_TIMER();
	uint32_t alarm_count = GetAlarmCount();

	k_usleep(ZB_BEACON_INTERVAL_MSEC * 1000);
	uint32_t new_alarm_count = GetAlarmCount();

	zassert_true((alarm_count != new_alarm_count),
		     "Alarm handler has not occurred");
}
