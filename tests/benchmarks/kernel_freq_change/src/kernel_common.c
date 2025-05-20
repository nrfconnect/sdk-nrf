/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/*
 * Taken from zephyr/tests/kernel/common/kernel.common
 *
 */

struct timer_data {
	int duration_count;
	int stop_count;
};
static void duration_expire(struct k_timer *timer);
static void stop_expire(struct k_timer *timer);

/** TESTPOINT: init timer via K_TIMER_DEFINE */
static K_TIMER_DEFINE(ktimer, duration_expire, stop_expire);

static ZTEST_BMEM struct timer_data tdata;

#define DURATION      100
#define LESS_DURATION 70

/*
 *help function
 */
static void duration_expire(struct k_timer *timer)
{
	tdata.duration_count++;
}

static void stop_expire(struct k_timer *timer)
{
	tdata.stop_count++;
}

static void init_data_count(void)
{
	tdata.duration_count = 0;
	tdata.stop_count = 0;
}

/**
 * @brief Test millisecond time duration
 *
 * @details initialize a timer, then providing time duration in
 * millisecond, and check the duration time whether correct.
 *
 * @see k_timer_init(), k_timer_start(), k_timer_stop(),
 * k_busy_wait()
 *
 *
 */

void test_ms_time_duration(void)
{
	init_data_count();
	k_timer_start(&ktimer, K_MSEC(DURATION), K_NO_WAIT);

	/** TESTPOINT: waiting time less than duration and check the count*/
	k_busy_wait(LESS_DURATION * 1000);
	zassert_true(tdata.duration_count == 0);
	zassert_true(tdata.stop_count == 0);

	/** TESTPOINT: proving duration in millisecond */
	init_data_count();
	k_timer_start(&ktimer, K_MSEC(100), K_MSEC(50));

	/** TESTPOINT: waiting time more than duration and check the count */
	k_usleep(1); /* align to tick */
	k_busy_wait((DURATION + 1) * 1000);
	zassert_true(tdata.duration_count == 1, "duration %u not 1", tdata.duration_count);
	zassert_true(tdata.stop_count == 0, "stop %u not 0", tdata.stop_count);

	/** cleanup environment */
	k_timer_stop(&ktimer);
}
