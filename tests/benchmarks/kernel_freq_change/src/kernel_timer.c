/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/types.h>

/*
 * Taken from zephyr/tests/kernel/timer/timer_api/kernel.timer
 */

struct timer_data {
	int expire_cnt;
	int stop_cnt;
	int64_t timestamp;
};

#define DURATION			   100
#define PERIOD				   50
#define EXPIRE_TIMES			   4
#define WITHIN_ERROR(var, target, epsilon) (llabs((int64_t)((target) - (var))) <= (epsilon))

/* ms can be converted precisely to ticks only when a ms is exactly
 * represented by an integral number of ticks.  If the conversion is
 * not precise, then the reverse conversion of a difference in ms can
 * end up being off by a tick depending on the relative error between
 * the first and second ms conversion, and we need to adjust the
 * tolerance interval.
 */
#define INEXACT_MS_CONVERT ((CONFIG_SYS_CLOCK_TICKS_PER_SEC % MSEC_PER_SEC) != 0)

#if CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT && CONFIG_NRF_RTC_TIMER
/* On Nordic SOCs using RTC_TIMER as the system timer and custom k_busy_wait() implementation,
 * one or both of the tick and busy-wait clocks may derive from sources that have slews that
 * sum to +/- 13%.
 */
#define BUSY_TICK_SLEW_PPM 130000U
#elif CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT && CONFIG_NRF_GRTC_TIMER
/* On Nordic SOCs using GRTC_TIMER as the system timer and custom k_busy_wait() implementation,
 * one or both of the tick and busy-wait clocks may derive from sources that have slews that
 * sum to +/- 2%.
 */
#define BUSY_TICK_SLEW_PPM 20000U
#else
/* In other cases assume the clocks are perfectly aligned. */
#define BUSY_TICK_SLEW_PPM 0U
#endif /* CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT && CONFIG_NRF_RTC_TIMER */
#define PPM_DIVISOR 1000000U

/* If the tick clock is faster or slower than the busywait clock the
 * remaining time for a partially elapsed timer in ticks will be
 * larger or smaller than expected by a value that depends on the slew
 * between the two clocks.  Produce a maximum error for a given
 * duration in microseconds.
 */
#define BUSY_SLEW_THRESHOLD_TICKS(_us)                                                             \
	k_us_to_ticks_ceil32((_us) * (uint64_t)BUSY_TICK_SLEW_PPM / PPM_DIVISOR)

static void duration_expire(struct k_timer *timer);
static void duration_stop(struct k_timer *timer);

/** TESTPOINT: init timer via K_TIMER_DEFINE */
static K_TIMER_DEFINE(ktimer, duration_expire, duration_stop);

static struct k_timer status_anytime_timer;
static struct k_timer remain_timer;

static ZTEST_BMEM struct timer_data tdata;

#define TIMER_ASSERT(exp, tmr)                                                                     \
	do {                                                                                       \
		if (!(exp)) {                                                                      \
			k_timer_stop(tmr);                                                         \
			zassert_true(exp);                                                         \
		}                                                                                  \
	} while (0)

static void init_timer_data(void)
{
	tdata.expire_cnt = 0;
	tdata.stop_cnt = 0;

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_usleep(1); /* align to tick */
	}

	tdata.timestamp = k_uptime_get();
}

static bool interval_check(int64_t interval, int64_t desired)
{
	int64_t slop = INEXACT_MS_CONVERT ? 1 : 0;

	/* Tickless kernels will advance time inside of an ISR, so it
	 * is always possible (especially with high tick rates and
	 * slow CPUs) for us to arrive at the uptime check above too
	 * late to see a full period elapse before the next period.
	 * We can alias at both sides of the interval, so two
	 * one-ticks deltas (NOT one two-tick delta!)
	 */
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		slop += 2 * k_ticks_to_ms_ceil32(1);
	}

	if (!WITHIN_ERROR(interval, desired, slop)) {
		return false;
	}

	return true;
}

/* entry routines */
static void duration_expire(struct k_timer *timer)
{
	/** TESTPOINT: expire function */
	int64_t interval = k_uptime_delta(&tdata.timestamp);

	tdata.expire_cnt++;
	if (tdata.expire_cnt == 1) {
		TIMER_ASSERT(interval_check(interval, DURATION), timer);
	} else {
		TIMER_ASSERT(interval_check(interval, PERIOD), timer);
	}

	if (tdata.expire_cnt >= EXPIRE_TIMES) {
		k_timer_stop(timer);
	}
}

static void duration_stop(struct k_timer *timer)
{
	tdata.stop_cnt++;
}

static void busy_wait_ms(int32_t ms)
{
	k_busy_wait(ms * 1000);
}

/**
 * @brief Test Timer status randomly after certain duration
 *
 * Validate timer status function using k_timer_status_get().
 *
 * It initializes the timer with k_timer_init(), then starts the timer
 * using k_timer_start() with specific initial duration and period.
 * Checks for timer status randomly after certain duration.
 * Stops the timer using k_timer_stop().
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_status_get(),
 * k_timer_stop(), k_busy_wait()
 */
void test_timer_status_get_anytime(void)
{
	init_timer_data();
	k_timer_start(&status_anytime_timer, K_MSEC(DURATION), K_MSEC(PERIOD));
	busy_wait_ms(DURATION + PERIOD * (EXPIRE_TIMES - 1) + PERIOD / 2);

	/** TESTPOINT: status get at any time */
	TIMER_ASSERT(k_timer_status_get(&status_anytime_timer) == EXPIRE_TIMES,
		     &status_anytime_timer);
	busy_wait_ms(PERIOD);
	TIMER_ASSERT(k_timer_status_get(&status_anytime_timer) == 1, &status_anytime_timer);

	/* cleanup environment */
	k_timer_stop(&status_anytime_timer);
}

/**
 * @brief Test statically defined Timer init
 *
 * Validate statically defined timer init using K_TIMER_DEFINE
 *
 * It creates prototype of K_TIMER_DEFINE to statically define timer
 * init and starts the timer with k_timer_start() with specific initial
 * duration and period. Stops the timer using k_timer_stop() and checks
 * for proper completion of duration and period.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_start(), K_TIMER_DEFINE(), k_timer_stop()
 * k_uptime_get(), k_busy_wait()
 */
void test_timer_k_define(void)
{
	init_timer_data();
	/** TESTPOINT: init timer via k_timer_init */
	k_timer_start(&ktimer, K_MSEC(DURATION), K_MSEC(PERIOD));
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);

	/** TESTPOINT: check expire and stop times */
	TIMER_ASSERT(tdata.expire_cnt == EXPIRE_TIMES, &ktimer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &ktimer);

	/* cleanup environment */
	k_timer_stop(&ktimer);

	init_timer_data();
	/** TESTPOINT: init timer via k_timer_init */
	k_timer_start(&ktimer, K_MSEC(DURATION), K_MSEC(PERIOD));

	/* Call the k_timer_start() again to make sure that
	 * the initial timeout request gets cancelled and new
	 * one will get added.
	 */
	busy_wait_ms(DURATION / 2);
	k_timer_start(&ktimer, K_MSEC(DURATION), K_MSEC(PERIOD));
	tdata.timestamp = k_uptime_get();
	busy_wait_ms(DURATION + PERIOD * EXPIRE_TIMES + PERIOD / 2);

	/** TESTPOINT: check expire and stop times */
	TIMER_ASSERT(tdata.expire_cnt == EXPIRE_TIMES, &ktimer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &ktimer);

	/* cleanup environment */
	k_timer_stop(&ktimer);
}

/**
 * @brief Test accuracy of k_timer_remaining_get()
 *
 * Validate countdown of time to expiration
 *
 * Starts a timer, busy-waits for half the DURATION, then checks the
 * remaining time to expiration and stops the timer. The remaining time
 * should reflect the passage of at least the busy-wait interval.
 *
 * @ingroup kernel_timer_tests
 *
 * @see k_timer_init(), k_timer_start(), k_timer_stop(),
 * k_timer_remaining_get()
 */

void test_timer_remaining(void)
{
	uint32_t dur_ticks = k_ms_to_ticks_ceil32(DURATION);
	uint32_t target_rem_ticks = k_ms_to_ticks_ceil32(DURATION / 2);
	uint32_t rem_ms, rem_ticks, exp_ticks;
	int32_t delta_ticks;
	uint32_t slew_ticks;
	uint64_t now;

	init_timer_data();
	k_timer_start(&remain_timer, K_MSEC(DURATION), K_NO_WAIT);
	busy_wait_ms(DURATION / 2);
	rem_ticks = k_timer_remaining_ticks(&remain_timer);
	now = k_uptime_ticks();
	rem_ms = k_timer_remaining_get(&remain_timer);
	exp_ticks = k_timer_expires_ticks(&remain_timer);
	k_timer_stop(&remain_timer);
	TIMER_ASSERT(tdata.expire_cnt == 0, &remain_timer);
	TIMER_ASSERT(tdata.stop_cnt == 1, &remain_timer);

	/*
	 * While the busy_wait_ms() works with the maximum possible resolution,
	 * the k_timer api is limited by the system tick abstraction. As result
	 * the value obtained through k_timer_remaining_get() could be larger
	 * than actual remaining time with maximum error equal to one tick.
	 */
	zassert_true(rem_ms <= (DURATION / 2) + k_ticks_to_ms_floor64(1), NULL);

	/* Half the value of DURATION in ticks may not be the value of
	 * half DURATION in ticks, when DURATION/2 is not an integer
	 * multiple of ticks, so target_rem_ticks is used rather than
	 * dur_ticks/2.  Also set a threshold based on expected clock
	 * skew.
	 */
	delta_ticks = (int32_t)(rem_ticks - target_rem_ticks);
	slew_ticks = BUSY_SLEW_THRESHOLD_TICKS(DURATION * USEC_PER_MSEC / 2U);
	zassert_true(abs(delta_ticks) <= MAX(slew_ticks, 1U),
		     "tick/busy slew %d larger than test threshold %u", delta_ticks, slew_ticks);

	/* Note +1 tick precision: even though we're calculating in
	 * ticks, we're waiting in k_busy_wait(), not for a timer
	 * interrupt, so it's possible for that to take 1 tick longer
	 * than expected on systems where the requested microsecond
	 * delay cannot be exactly represented as an integer number of
	 * ticks.
	 * As above, use higher tolerance on platforms where the clock used
	 * by the kernel timer and the one used for busy-waiting may be skewed.
	 */
	zassert_true(((int64_t)exp_ticks - (int64_t)now) <= (dur_ticks / 2) + 1 + slew_ticks, NULL);
}

static void timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn, k_timer_stop_t stop_fn)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_object_access_grant(timer, k_current_get());
	}

	k_timer_init(timer, expiry_fn, stop_fn);
}

void *setup_timer_api(void)
{
	timer_init(&status_anytime_timer, NULL, NULL);
	timer_init(&remain_timer, duration_expire, duration_stop);

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_thread_access_grant(k_current_get(), &ktimer);
	}

	return NULL;
}
