/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <hal/nrf_grtc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#define REQUEST_SERVING_WAIT_TIME_US 10000
#if defined(CONFIG_COVERAGE)
#define DIFF_TOLERANCE 2
#else
#define DIFF_TOLERANCE 1
#endif

typedef enum {
	NO_CHANGE = 0,
	CHANGE_TO_SYNTH = 1,
	CHANGE_TO_LFRC = 2
} lfclk_source_change_option;

struct accuracy_test_limit {
	bool is_reference_timer_enabled;
	uint32_t grtc_timer_count_time_ms;
	uint32_t time_delta_abs_tolerance_us;
	uint64_t final_max_ticks_count_difference;
};

const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));
const struct device *const lfclk_dev = DEVICE_DT_GET(DT_NODELABEL(lfclk));
const struct device *const fll16m_dev = DEVICE_DT_GET(DT_NODELABEL(fll16m));
static volatile uint8_t compare_isr_call_counter;
static volatile uint64_t compare_count_value;

const struct nrf_clock_spec lfclk_lfrc_mode = {
	.frequency = 32768,
	.accuracy = 0,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

const struct nrf_clock_spec lfclk_synth_mode = {
	.frequency = 32768,
	.accuracy = 30,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

const struct nrf_clock_spec fll16m_bypass_mode = {
	.frequency = MHZ(16),
	.accuracy = 30,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

static void request_clock_spec(const struct device *clk_dev, const struct nrf_clock_spec *clk_spec)
{
	int ret = 0;
	int res = 0;
	struct onoff_client cli;
	uint32_t rate;

	TC_PRINT("Clock: %s, requesting frequency=%d, accuracy=%d, precision=%d\n", clk_dev->name,
		 clk_spec->frequency, clk_spec->accuracy, clk_spec->precision);
	sys_notify_init_spinwait(&cli.notify);
	ret = nrf_clock_control_request(clk_dev, clk_spec, &cli);
	zassert_between_inclusive(ret, 0, 2);
	do {
		ret = sys_notify_fetch_result(&cli.notify, &res);
		k_yield();
	} while (ret == -EAGAIN);
	zassert_equal(ret, 0);
	zassert_equal(res, 0);
	ret = clock_control_get_rate(clk_dev, NULL, &rate);
	if (ret != -ENOSYS) {
		zassert_equal(ret, 0);
		zassert_equal(rate, clk_spec->frequency);
		k_busy_wait(REQUEST_SERVING_WAIT_TIME_US);
	}
}

static void timer_compare_interrupt_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	uint32_t tst_timer_value;
	uint64_t reference_timer_value_us;
	struct accuracy_test_limit *test_limit = (struct accuracy_test_limit *)user_data;

	compare_count_value = z_nrf_grtc_timer_read();
	compare_isr_call_counter++;
	if (test_limit->is_reference_timer_enabled) {
		counter_get_value(tst_timer_dev, &tst_timer_value);
		counter_stop(tst_timer_dev);
		reference_timer_value_us = counter_ticks_to_us(tst_timer_dev, tst_timer_value);
		TC_PRINT("Reference timer value [us]: %llu\n", reference_timer_value_us);
		TC_PRINT("Time delta (Specified (GRTC) - referecne timer) [us]: %lld\n",
			 (uint64_t)test_limit->grtc_timer_count_time_ms * 1000 -
				 reference_timer_value_us);
		zassert_true((uint64_t)test_limit->grtc_timer_count_time_ms * 1000 <
				     reference_timer_value_us +
					     test_limit->time_delta_abs_tolerance_us *
						     DIFF_TOLERANCE,
			     "GRTC timer count value is over upper limit");
		zassert_true((uint64_t)test_limit->grtc_timer_count_time_ms * 1000 >
				     reference_timer_value_us -
					     test_limit->time_delta_abs_tolerance_us *
						     DIFF_TOLERANCE,
			     "GRTC timer count value is under lower limit");
	}

	TC_PRINT("Compare count value [ticks]: %llu\n", compare_count_value);
}

static void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms)
{
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)count_time_ms * 1000);
	counter_cfg.user_data = &counter_cfg;
}

static void test_timer_compare(const struct accuracy_test_limit *test_limit,
			       lfclk_source_change_option tst_dynamic_clk_change_option)
{
	int err;
	uint64_t test_ticks = 0;
	uint64_t compare_value = 0;
	int32_t channel = z_nrf_grtc_timer_chan_alloc();

	if (test_limit->is_reference_timer_enabled) {
		TC_PRINT("Reference timer (HFCLK sourced) is enabled\n");
		configure_test_timer(tst_timer_dev, test_limit->grtc_timer_count_time_ms * 2);
	}

	TC_PRINT("Allocated GRTC channel %d\n", channel);
	if (channel < 0) {
		TC_PRINT("Failed to allocate GRTC channel, chan=%d\n", channel);
		ztest_test_fail();
	}

	compare_isr_call_counter = 0;
	test_ticks = z_nrf_grtc_timer_get_ticks(K_MSEC(test_limit->grtc_timer_count_time_ms));
	err = z_nrf_grtc_timer_set(channel, test_ticks, timer_compare_interrupt_handler,
				   (void *)test_limit);

	if (test_limit->is_reference_timer_enabled) {
		counter_start(tst_timer_dev);
	}
	zassert_equal(err, 0, "z_nrf_grtc_timer_set raised an error: %d", err);

	z_nrf_grtc_timer_compare_read(channel, &compare_value);
	zassert_true(K_TIMEOUT_EQ(K_TICKS(compare_value), K_TICKS(test_ticks)),
		     "Compare register set failed");
	zassert_equal(err, 0, "Unexpected error raised when setting timer, err: %d", err);

	if (tst_dynamic_clk_change_option == CHANGE_TO_SYNTH) {
		TC_PRINT("Changing clock source of the GRTC (LFCLK) to SYNTH while waiting for "
			 "compare callback\n");
		request_clock_spec(lfclk_dev, &lfclk_synth_mode);
	} else if (tst_dynamic_clk_change_option == CHANGE_TO_LFRC) {
		TC_PRINT("Changing clock source of the GRTC (LFCLK) to LFRC while waiting for "
			 "compare callback\n");
		request_clock_spec(lfclk_dev, &lfclk_lfrc_mode);
	} else {
		TC_PRINT("Clock source change not requested\n");
	}

	k_sleep(K_MSEC(2 * test_limit->grtc_timer_count_time_ms));
	zassert_equal(compare_isr_call_counter, 1, "Compare isr call counter: %d",
		      compare_isr_call_counter);

	TC_PRINT("Compare count - initial compare value: %lld\n",
		 compare_count_value - compare_value);

	zassert_true((compare_count_value - compare_value) <
			     test_limit->final_max_ticks_count_difference * DIFF_TOLERANCE,
		     "Maximal final ticks count difference is over the limit");
	z_nrf_grtc_timer_chan_free(channel);
}

/*
 * This test uses HFCLK based timer130 as a reference.
 * The timers start is not ideally sunchronised,
 * this is taken into account in the limit
 */
ZTEST(grtc_timer_lfrc, test_timer_count_in_compare_mode_lfclk_source_from_lfrc)
{
	const struct accuracy_test_limit test_limit = {.is_reference_timer_enabled = true,
						       .grtc_timer_count_time_ms = 1000,
						       .time_delta_abs_tolerance_us = 15,
						       .final_max_ticks_count_difference = 50};

	request_clock_spec(lfclk_dev, &lfclk_lfrc_mode);
	test_timer_compare(&test_limit, NO_CHANGE);
}

ZTEST(grtc_timer_lfrc, test_timer_count_in_compare_mode_lfclk_source_from_synth)
{
	const struct accuracy_test_limit test_limit = {.is_reference_timer_enabled = false,
						       .grtc_timer_count_time_ms = 50,
						       .time_delta_abs_tolerance_us = 0,
						       .final_max_ticks_count_difference = 50};

	request_clock_spec(lfclk_dev, &lfclk_synth_mode);
	test_timer_compare(&test_limit, NO_CHANGE);
}

ZTEST(grtc_timer_lfrc, test_timer_count_in_compare_mode_lfclk_runtime_update_lfrc_to_synth)
{
	const struct accuracy_test_limit test_limit = {.is_reference_timer_enabled = false,
						       .grtc_timer_count_time_ms = 250,
						       .time_delta_abs_tolerance_us = 0,
						       .final_max_ticks_count_difference = 50};

	request_clock_spec(lfclk_dev, &lfclk_lfrc_mode);
	test_timer_compare(&test_limit, CHANGE_TO_SYNTH);
}

ZTEST(grtc_timer_lfrc, test_timer_count_in_compare_mode_lfclk_runtime_update_synth_to_lfrc)
{
	const struct accuracy_test_limit test_limit = {.is_reference_timer_enabled = false,
						       .grtc_timer_count_time_ms = 250,
						       .time_delta_abs_tolerance_us = 0,
						       .final_max_ticks_count_difference = 50};

	request_clock_spec(lfclk_dev, &lfclk_synth_mode);
	test_timer_compare(&test_limit, CHANGE_TO_LFRC);
}

/*
 * FLL16M must be set in bypass mode
 * to achieve the HFCLK 30ppm accuracy
 */
void *test_setup(void)
{
	request_clock_spec(fll16m_dev, &fll16m_bypass_mode);

	return NULL;
}

ZTEST_SUITE(grtc_timer_lfrc, NULL, test_setup, NULL, NULL, NULL);
