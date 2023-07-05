/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <zboss_api.h>
#include <zb_osif_platform.h>

#define ZB_BEACON_INTERVAL_USEC 15360
#define ZB_BEACON_INTERVAL_MSEC ((ZB_BEACON_INTERVAL_USEC + 1000 - 1) / 1000)


ZTEST_SUITE(osif_test, NULL, NULL, NULL, NULL, NULL);

ZTEST(osif_test, test_zb_osif_timer)
{
	ZB_START_HW_TIMER();
	zassert_true(ZB_CHECK_TIMER_IS_ON(), "Zephyr timer not running");

	/* The system timer should not be stopped by the Zigbee stack. */
	ZB_STOP_HW_TIMER();
	zassert_true(ZB_CHECK_TIMER_IS_ON(), "Zephyr timer stopped by ZBOSS API");

	ZB_START_HW_TIMER();
	uint64_t timestamp1 = osif_transceiver_time_get_long();

	/* The ktimer is unable to detect intervals shorter than 1ms. */
	k_usleep(1000);
	uint64_t timestamp2 = osif_transceiver_time_get_long();

	zassert_true((timestamp2 > timestamp1),
		     "Timer is not incrementing");

	ZB_START_HW_TIMER();
	uint32_t time_start_bi = zb_timer_get();

	k_usleep(ZB_BEACON_INTERVAL_MSEC * 1000);
	uint32_t time_stop_bi = zb_timer_get();

	zassert_true((time_stop_bi > time_start_bi),
		     "ZBOSS time value was not increased");
	zassert_true((time_stop_bi <= time_start_bi + 2),
		     "ZBOSS time value was increased by more than 2 BI");
}
