/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

static volatile uint8_t compare_isr_call_counter;
static volatile uint64_t compare_count_value;
static volatile uint64_t reference_timer_value_us;

const struct device *const tst_timer_dev = DEVICE_DT_GET(DT_ALIAS(tst_timer));

const struct nrf_clock_spec clock_hf = {
	.frequency = MHZ(32),
	.accuracy = 30,
	.precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT,
};

#if defined(CONFIG_SOC_NRF54H20)
extern const struct device *const lfclk_dev;
extern const struct device *const fll16m_dev;
extern const struct nrf_clock_spec lfclk_lfrc_mode;
extern const struct nrf_clock_spec lfclk_synth_mode;
extern const struct nrf_clock_spec fll16m_bypass_mode;
#else
extern const struct device *const hfclock;
#endif /* CONFIG_SOC_NRF54H20 */

static void timer_compare_interrupt_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	uint32_t tst_timer_value;

	compare_count_value = z_nrf_grtc_timer_read();
	compare_isr_call_counter++;

	counter_get_value(tst_timer_dev, &tst_timer_value);
	counter_stop(tst_timer_dev);
	reference_timer_value_us = counter_ticks_to_us(tst_timer_dev, tst_timer_value);
}

static void configure_test_timer(const struct device *timer_dev, uint32_t count_time_ms)
{
	struct counter_alarm_cfg counter_cfg;

	counter_cfg.flags = 0;
	counter_cfg.ticks = counter_us_to_ticks(timer_dev, (uint64_t)count_time_ms * 1000);
	counter_cfg.user_data = &counter_cfg;
}

static int test_timer_compare(int32_t channel, const struct accuracy_test_limit *test_limit,
			      lfclk_source_change_option tst_dynamic_clk_change_option)
{
	int err;
	uint64_t test_ticks = 0;
	uint64_t compare_value = 0;
	int64_t time_delta_us = 0;

	printk("Reference timer (HFCLK sourced) is enabled\n");
	configure_test_timer(tst_timer_dev, test_limit->grtc_timer_count_time_ms);
	counter_reset(tst_timer_dev);

	reference_timer_value_us = 0;
	compare_isr_call_counter = 0;
	test_ticks = z_nrf_grtc_timer_get_ticks(K_MSEC(test_limit->grtc_timer_count_time_ms));
	err = z_nrf_grtc_timer_set(channel, test_ticks, timer_compare_interrupt_handler, NULL);
	if (err != 0) {
		printk("Unexpected error raised when setting timer, err: %d\n", err);
		return -1;
	}
	counter_start(tst_timer_dev);
	z_nrf_grtc_timer_compare_read(channel, &compare_value);
	if (K_TIMEOUT_EQ(K_TICKS(compare_value), K_TICKS(test_ticks)) == false) {
		printk("Compare register set failed\n");
		return -2;
	}

#if defined(CONFIG_SOC_NRF54H20)
	if (tst_dynamic_clk_change_option == CHANGE_TO_SYNTH) {
		printk("Changing clock source of the GRTC (LFCLK) to SYNTH while waiting for "
		       "compare callback\n");
		request_clock_spec(lfclk_dev, &lfclk_synth_mode);
	} else if (tst_dynamic_clk_change_option == CHANGE_TO_LFRC) {
		printk("Changing clock source of the GRTC (LFCLK) to LFRC while waiting for "
		       "compare callback\n");
		request_clock_spec(lfclk_dev, &lfclk_lfrc_mode);
	} else {
		printk("Clock source change not requested\n");
	}
#else /* CONFIG_SOC_NRF54H20 */
	printk("Clock source change not supported\n");
#endif

	k_sleep(K_MSEC(2 * test_limit->grtc_timer_count_time_ms));
	if (compare_isr_call_counter != 1) {
		printk("Invalid number of triggered GRTC intertupts: %u", compare_isr_call_counter);
		return -3;
	}

	printk("Reference timer value [us]: %llu\n", reference_timer_value_us);
	printk("Compare count value [ticks]: %llu\n", compare_count_value);
	time_delta_us =
		(int64_t)test_limit->grtc_timer_count_time_ms * 1000 - reference_timer_value_us;
	printk("Time delta (Specified (GRTC) - reference timer) [us]: %lld\n", time_delta_us);

	if (time_delta_us < 0) {
		time_delta_us *= -1;
	}

	if (time_delta_us > (int64_t)(test_limit->time_delta_abs_tolerance_us * DIFF_TOLERANCE)) {
		printk("Absolute time delta is over the limit\n");
		return -4;
	}

	return 0;
}

/*
 * These tests use HFCLK based timerXXX as a reference.
 * The timers start is not ideally synchronised,
 * this is taken into account in the limit
 */
static int test_timer_count_in_compare_mode_lfclk_source_from_lfrc(int32_t grtc_channel)
{
	int ret = 0;
	const struct accuracy_test_limit test_limit = {.grtc_timer_count_time_ms = 1000,
						       .time_delta_abs_tolerance_us = 250};

	printk("test_timer_count_in_compare_mode_lfclk_source_from_lfrc\n");
#if defined(CONFIG_SOC_NRF54H20)
	request_clock_spec(lfclk_dev, &lfclk_lfrc_mode);
	if (ret != 0) {
		printk("Clock request failed: %d\n", ret);
		return -1;
	}
#endif /* CONFIG_SOC_NRF54H20 */

	for (int i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		ret += test_timer_compare(grtc_channel, &test_limit, NO_CHANGE);
	}
	return ret;
}

#if defined(CONFIG_SOC_NRF54H20_CPUAPP)
static int test_timer_count_in_compare_mode_lfclk_source_from_synth(int32_t grtc_channel)
{
	int ret = 0;
	const struct accuracy_test_limit test_limit = {.grtc_timer_count_time_ms = 50,
						       .time_delta_abs_tolerance_us = 20};

	printk("test_timer_count_in_compare_mode_lfclk_source_from_synth\n");
	ret = request_clock_spec(lfclk_dev, &lfclk_synth_mode);
	if (ret != 0) {
		printk("Clock request failed: %d\n", ret);
		return -1;
	}

	for (int i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		ret += test_timer_compare(grtc_channel, &test_limit, NO_CHANGE);
	}
	return ret;
}

static int test_timer_count_in_compare_mode_lfclk_runtime_update_lfrc_to_synth(int32_t grtc_channel)
{
	int ret = 0;
	const struct accuracy_test_limit test_limit = {.grtc_timer_count_time_ms = 50,
						       .time_delta_abs_tolerance_us = 26};

	printk("test_timer_count_in_compare_mode_lfclk_runtime_update_lfrc_to_synth\n");
	ret = request_clock_spec(lfclk_dev, &lfclk_lfrc_mode);
	if (ret != 0) {
		printk("Clock request failed: %d\n", ret);
		return -1;
	}

	for (int i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		ret += test_timer_compare(grtc_channel, &test_limit, CHANGE_TO_SYNTH);
	}
	return ret;
}

static int test_timer_count_in_compare_mode_lfclk_runtime_update_synth_to_lfrc(int32_t grtc_channel)
{
	int ret = 0;
	const struct accuracy_test_limit test_limit = {.grtc_timer_count_time_ms = 50,
						       .time_delta_abs_tolerance_us = 26};

	printk("test_timer_count_in_compare_mode_lfclk_runtime_update_synth_to_lfrc\n");
	ret = request_clock_spec(lfclk_dev, &lfclk_synth_mode);
	if (ret != 0) {
		printk("Clock request failed: %d\n", ret);
		return -1;
	}

	for (int i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		ret += test_timer_compare(grtc_channel, &test_limit, CHANGE_TO_LFRC);
	}
	return ret;
}
#endif /* CONFIG_SOC_NRF54H20_CPUAPP */

int main(void)
{
	int32_t grtc_channel;
	int ret = 0;

#if defined(CONFIG_SOC_NRF54H20)
	ret = request_clock_spec(fll16m_dev, &fll16m_bypass_mode);
#else
	k_msleep(HFCLOCK_SETUP_TIME_MS);
	ret = clock_control_on(hfclock, CLOCK_CONTROL_NRF_SUBSYS_HF);
#endif /* CONFIG_SOC_NRF54H20 */
	if (ret != 0) {
		printk("Clock request failed: %d\n", ret);
		return -1;
	}

	grtc_channel = z_nrf_grtc_timer_chan_alloc();

	printk("Allocated GRTC channel %d\n", grtc_channel);
	if (grtc_channel < 0) {
		printk("Failed to allocate GRTC channel, chan=%d\n", grtc_channel);
		return -2;
	}

	ret = test_timer_count_in_compare_mode_lfclk_source_from_lfrc(grtc_channel);
#if defined(CONFIG_SOC_NRF54H20_CPUAPP)
	ret += test_timer_count_in_compare_mode_lfclk_source_from_synth(grtc_channel);
	ret += test_timer_count_in_compare_mode_lfclk_runtime_update_lfrc_to_synth(grtc_channel);
	ret += test_timer_count_in_compare_mode_lfclk_runtime_update_synth_to_lfrc(grtc_channel);
#endif /* CONFIG_SOC_NRF54H20_CPUAPP */

	if (ret != 0) {
		printk("Tests status: FAIL\n");
	} else {
		printk("Tests status: PASS\n");
	}

	z_nrf_grtc_timer_chan_free(grtc_channel);

	return 0;
}
