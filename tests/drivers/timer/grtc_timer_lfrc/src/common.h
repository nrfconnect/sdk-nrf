/*
 * Copyright (c) 2025, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <hal/nrf_grtc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#define REQUEST_SERVING_WAIT_TIME_US 10000
#define HFCLOCK_SETUP_TIME_MS	     500
#if defined(CONFIG_COVERAGE)
#define DIFF_TOLERANCE 2
#else
#define DIFF_TOLERANCE 1
#endif
#define NUMBER_OF_REPETITIONS 5

typedef enum {
	NO_CHANGE = 0,
	CHANGE_TO_SYNTH = 1,
	CHANGE_TO_LFRC = 2
} lfclk_source_change_option;

struct accuracy_test_limit {
	uint32_t grtc_timer_count_time_ms;
	uint32_t time_delta_abs_tolerance_us;
};

int request_clock_spec(const struct device *clk_dev, const struct nrf_clock_spec *clk_spec);
