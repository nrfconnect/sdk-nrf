/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

struct timer_data {
	int duration_count;
	int stop_count;
};
static void duration_expire(struct k_timer *timer);
static void stop_expire(struct k_timer *timer);

/** TESTPOINT: init timer via K_TIMER_DEFINE */
K_TIMER_DEFINE(ktimer, duration_expire, stop_expire);

const struct device *global_hsfll_dev = DEVICE_DT_GET(DT_NODELABEL(hsfll120));
#if CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP
const struct device *local_hsfll_dev = DEVICE_DT_GET(DT_NODELABEL(cpuapp_hsfll));
#elif defined(CONFIG_BOARD_NRF54H20DK_NRF54H20_CPURAD)
const struct device *local_hsfll_dev = DEVICE_DT_GET(DT_NODELABEL(cpurad_hsfll));
#endif

static ZTEST_BMEM struct timer_data tdata;

#define DURATION      100
#define LESS_DURATION 70

static void set_clock_frequency(uint32_t freq_mhz, const struct device *clk_dev)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	uint32_t rate;
	const struct nrf_clock_spec clk_spec = {.frequency = freq_mhz};

	TC_PRINT("Clock under test: %s\n", clk_dev->name);
	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, &clk_spec, &cli);
	TC_PRINT("nrf_clock_control_request return code: %d\n", ret);
	zassert_between_inclusive(ret, 0, 2);
	do {
		ret = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (ret == -EAGAIN);
	TC_PRINT("Clock control request return value: %d\n", ret);
	TC_PRINT("Clock control request response code: %d\n", res);
	zassert_ok(ret);
	zassert_ok(res);
	ret = clock_control_get_rate(clk_dev, NULL, &rate);
	if (ret != -ENOSYS) {
		zassert_ok(ret);
		zassert_equal(rate, clk_spec.frequency);
	}
	ret = nrf_clock_control_release(clk_dev, &clk_spec);
	zassert_equal(ret, ONOFF_STATE_ON);
}

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
static void test_ms_time_duration(void)
{
	/** cleanup environment */
	k_timer_stop(&ktimer);

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

ZTEST(clock, test_ms_time_duration_4_gd_320MHz)
{
	set_clock_frequency(MHZ(320), global_hsfll_dev);
	test_ms_time_duration();
}

ZTEST(clock, test_ms_time_duration_3_gd_256MHz)
{
	set_clock_frequency(MHZ(256), global_hsfll_dev);
	test_ms_time_duration();
}

ZTEST(clock, test_ms_time_duration_2_gd_128MHz)
{
	set_clock_frequency(MHZ(128), global_hsfll_dev);
	test_ms_time_duration();
}

ZTEST(clock, test_ms_time_duration_1_gd_64MHz)
{
	set_clock_frequency(MHZ(64), global_hsfll_dev);
	test_ms_time_duration();
}

ZTEST(clock, test_ms_time_duration)
{
	test_ms_time_duration();
}

ZTEST(clock, test_ms_time_duration_1_ld_64MHz)
{
	set_clock_frequency(MHZ(64), local_hsfll_dev);
	test_ms_time_duration();
}

ZTEST(clock, test_ms_time_duration_2_ld_128MHz)
{
	set_clock_frequency(MHZ(128), local_hsfll_dev);
	test_ms_time_duration();
}

ZTEST(clock, test_ms_time_duration_3_ld_320MHz)
{
	set_clock_frequency(MHZ(320), local_hsfll_dev);
	test_ms_time_duration();
}

static void *clock_setup(void)
{
	static bool once = false;

	/** wait for clock initialization */
	if (!once) {
		k_msleep(2000);
		once = true;
	}
	return NULL;
}

ZTEST_SUITE(clock, NULL, clock_setup, NULL, NULL, NULL);
