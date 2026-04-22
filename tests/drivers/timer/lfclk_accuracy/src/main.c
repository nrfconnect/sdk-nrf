/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Test information
 * This test measures the GRTC timer intervals
 * with universal timer, which is clocked by precise HFCLOCK
 *
 * GRTC configuration:
 * GRTC timer is supplied from LFCLOCK
 * GRTC is operating in an interval mode
 *
 * DPPI connection setup:
 * NRF_GRTC_EVENT_COMPARE_0 -> NRF_TIMER_TASK_CAPTURE0
 *
 * Test timer operation:
 * Timer is supplied from HFCLOCK (HFXO is required)
 * Universal timer works in a timer mode and captures
 * the CC value (in 'test_timer_values') when NRF_GRTC_EVENT_COMPARE_0 event is generated.
 * Therefore the following timer CC values differences (stored in 'grtc_intervals')
 * holds the measured GRTC timer intervals.
 *
 * Test lasts for 'TEST_DURATION_MS'
 * within this time up to 'TEST_DURATION_MS / GRTC_TIMER_INTERVAL_US' intervals are measured.
 * Each interval is assesed, intervals outside
 * of the limits (MAXIMAL_ALLOWED_ABS_TIMIMG_DEVIATION_US) are counted
 * Test is marked as FAILED is there are intervals outside of the limits
 *
 * The test setup must be executed as early as possible
 * to catch possible LFCOCLK instabilities:
 * for platfroms without System Controller it is possible in the 'EARLY' stage
 * for platform with System Controller it is possible in the 'APPLICATION' stage
 */

#include "common.h"

nrfx_timer_t test_timer = NRFX_TIMER_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tst_timer)));
NRF_GRTC_Type *grtc_reg = (NRF_GRTC_Type *)DT_REG_ADDR(DT_NODELABEL(grtc));

static timers_control tst_timers_control;
static uint32_t interval_counter;
static volatile int setup_status;

uint32_t test_timer_values[TIMER_VALUES_HOLD_BUFFER_SIZE] = {0};
uint32_t grtc_intervals[TIMER_VALUES_HOLD_BUFFER_SIZE] = {0};

int check_timing(void)
{
	uint64_t min_interval = UINT64_MAX;
	uint64_t max_interval = 0;
	uint64_t current_interval;
	int64_t deviation;
	uint32_t total_intervals = 0;
	uint32_t intervals_outside_of_limits = 0;
	int status = 0;

	for (int i = 0; i < TIMER_VALUES_HOLD_BUFFER_SIZE; i++) {
		if (i < TIMER_VALUES_HOLD_BUFFER_SIZE - 1) {
			if ((test_timer_values[i + 1] > 0) && (test_timer_values[i] > 0)) {
				current_interval = (uint64_t)test_timer_values[i + 1] -
						   (uint64_t)test_timer_values[i];
				current_interval *= 1000000LLU;
				current_interval /= TEST_TIMER_FREQUENCY_HZ;
				if (current_interval > max_interval) {
					max_interval = current_interval;
				}
				if (current_interval < min_interval) {
					min_interval = current_interval;
				}
				deviation = (int64_t)(current_interval - GRTC_TIMER_INTERVAL_US);
				if ((deviation >
				     (int64_t)MAXIMAL_ALLOWED_ABS_TIMIMG_DEVIATION_US) ||
				    (deviation <
				     -(int64_t)MAXIMAL_ALLOWED_ABS_TIMIMG_DEVIATION_US)) {
					intervals_outside_of_limits++;
					status = -1;
				}
				grtc_intervals[i] = (uint32_t)current_interval;
				total_intervals++;
			}
		}
	}

	printk("Summary:\n");
	printk("Configured GRTC interval: %u us\n", GRTC_TIMER_INTERVAL_US);
	printk("Measured GRTC intervals (captured with HFCLK clocked timer operating at %d Hz)\n",
	       TEST_TIMER_FREQUENCY_HZ);
	printk("Total measured intervals: %u\n", total_intervals);
	printk("Intervals outside of the limits: %u\n", intervals_outside_of_limits);
	printk("Minimal interval [us]: %llu\n", min_interval);
	printk("Maximal interval [us]: %llu\n", max_interval);

	return status;
}

static void timer_compare_interrupt_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	uint32_t timer_value = nrfx_timer_capture_get(&test_timer, NRF_TIMER_CC_CHANNEL0);

	test_timer_values[interval_counter] = timer_value;

	interval_counter++;
}

int setup_lfclk_accuracy_test(void)
{

	int ret;
	uint64_t compare_value = 0;
	uint32_t interval_ticks = 0;

	ret = configure_hfclock();
	if (ret != 0) {
		printk("HFCLOCK setup failed\n");
		setup_status = -1;
	}

	tst_timers_control.grtc_channel = z_nrf_grtc_timer_ext_chan_alloc();
	if (tst_timers_control.grtc_channel < 0) {
		printk("Failed to allocate GRTC channel, chan=%d\n",
		       tst_timers_control.grtc_channel);
		setup_status = -2;
	}

	tst_timers_control.grtc_compare_event_address =
		nrf_grtc_event_address_get(grtc_reg, NRF_GRTC_EVENT_COMPARE_0);
	tst_timers_control.timer_capture_task_address =
		configure_test_timer(&test_timer, TEST_TIMER_FREQUENCY_HZ);

	ret = nrfx_gppi_conn_alloc(tst_timers_control.grtc_compare_event_address,
				   tst_timers_control.timer_capture_task_address,
				   &tst_timers_control.gppi_handle);
	if (ret != 0) {
		printk("GPPI connection allocation failed\n");
		setup_status = -3;
	}
	nrfx_gppi_conn_enable(tst_timers_control.gppi_handle);

	/* GRTC runs at 1MHz */
	interval_ticks = GRTC_TIMER_INTERVAL_US * 1;

	ret = z_nrf_grtc_timer_interval_set(tst_timers_control.grtc_channel, interval_ticks,
					    interval_ticks, timer_compare_interrupt_handler, NULL);
	if (ret != 0) {
		printk("Unexpected error raised when setting timer, err: %d\n", ret);
		setup_status = -4;
	}

	z_nrf_grtc_timer_compare_read(tst_timers_control.grtc_channel, &compare_value);
	if (K_TIMEOUT_EQ(K_TICKS(interval_ticks), K_TICKS(interval_ticks)) == false) {
		printk("Compare register set failed\n");
		setup_status = -5;
	}
	return 0;
}

int main(void)
{
	int ret;

	printk("Test duration: %d ms\n", TEST_DURATION_MS);
	if (setup_status != 0) {
		printk("Setup status = %d\n", setup_status);
		printk("Tests status: SETUP FAIL\n");
		return -1;
	}
	k_msleep(TEST_DURATION_MS);

	z_nrf_grtc_timer_interval_stop(tst_timers_control.grtc_channel);
	z_nrf_grtc_timer_chan_free(tst_timers_control.grtc_channel);

	ret = check_timing();
	if (ret) {
		printk("Tests status: FAIL\n");
	} else {
		printk("Tests status: PASS\n");
	}

	nrfx_gppi_conn_disable(tst_timers_control.gppi_handle);

#if defined(CONFIG_SOC_NRF54H20)
	/* ToDo: Fails on Lumoses */
	nrfx_gppi_conn_free(tst_timers_control.grtc_compare_event_address,
			    tst_timers_control.timer_capture_task_address,
			    tst_timers_control.gppi_handle);
#endif
	return 0;
}

#if defined(CONFIG_TEST_NRFX_HF_CLOCK_DRIVER)
SYS_INIT(setup_lfclk_accuracy_test, EARLY, 5);
#else
SYS_INIT(setup_lfclk_accuracy_test, APPLICATION, 2);
#endif
