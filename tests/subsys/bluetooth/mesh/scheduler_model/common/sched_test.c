/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <bluetooth/mesh/time.h>
#include <time.h>
#include <sched_test.h>

void expected_tm_check(struct tm *received, struct tm *expected, int steps)
{
	for (int i = 0; i < steps; i++) {
		zassert_equal(received[i].tm_year, expected[i].tm_year,
			      "real year %d is not equal to expected %d on step %d",
			      received[i].tm_year, expected[i].tm_year, i + 1);

		zassert_equal(received[i].tm_mon, expected[i].tm_mon,
			      "real month %d is not equal to expected %d on step %d",
			      received[i].tm_mon, expected[i].tm_mon, i + 1);

		zassert_equal(received[i].tm_mday, expected[i].tm_mday,
			      "real day %d is not equal to expected %d on step %d",
			      received[i].tm_mday, expected[i].tm_mday, i + 1);

		if (expected[i].tm_hour == INT_MAX) {
			zassert_within(received[i].tm_hour, 0, 23,
				       "real hour %d is not within expected 0-23 range on step %d",
				       received[i].tm_hour, i + 1);
		} else {
			zassert_equal(received[i].tm_hour, expected[i].tm_hour,
				      "real hour %d is not equal to expected %d on step %d",
				      received[i].tm_hour, expected[i].tm_hour, i + 1);
		}

		if (expected[i].tm_min == INT_MAX) {
			zassert_within(
				received[i].tm_min, 0, 59,
				"real minute %d is not within expected 0-59 range on step %d",
				received[i].tm_min, i + 1);
		} else {
			zassert_equal(received[i].tm_min, expected[i].tm_min,
				      "real minute %d is not equal to expected %d on step %d",
				      received[i].tm_min, expected[i].tm_min, i + 1);
		}

		if (expected[i].tm_sec == INT_MAX) {
			zassert_within(
				received[i].tm_sec, 0, 59,
				"real second %d is not within expected 0-59 range on step %d",
				received[i].tm_sec, i + 1);
		} else {
			zassert_equal(received[i].tm_sec, expected[i].tm_sec,
				      "real second %d is not equal to expected %d on step %d",
				      received[i].tm_sec, expected[i].tm_sec, i + 1);
		}
	}
}

void start_time_adjust(struct tm *start_tm)
{
	int64_t uptime = k_uptime_get();
	struct bt_mesh_time_tai tai;

	zassert_ok(ts_to_tai(&tai, start_tm), "cannot convert tai time");

	int64_t tmp = tai_to_ms(&tai);

	tmp -= uptime;
	tai = tai_at(tmp);
	tai.sec++; /* 1 sec is lost during recalculation */
	tai_to_ts(&tai, start_tm);
}
