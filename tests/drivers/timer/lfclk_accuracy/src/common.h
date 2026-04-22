/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <hal/nrf_grtc.h>
#include <nrfx_timer.h>
#include <nrfx_grtc.h>
#include <helpers/nrfx_gppi.h>

#if defined(CONFIG_TEST_NRFX_HF_CLOCK_DRIVER)
#include <nrfx_clock.h>
#endif

#define REQUEST_SERVING_WAIT_TIME_US		10000
#define TEST_DURATION_MS			CONFIG_TEST_DURATION_MS
#define GRTC_TIMER_INTERVAL_US			CONFIG_TEST_GRTC_TIMER_INTERVAL_US
#define TIMER_VALUES_HOLD_BUFFER_SIZE		(TEST_DURATION_MS * 1000) / GRTC_TIMER_INTERVAL_US
#define TEST_TIMER_FREQUENCY_HZ			MHZ(8)
#define MAXIMAL_ALLOWED_ABS_TIMIMG_DEVIATION_US CONFIG_TEST_ABS_TIMIMG_DEVIATION_US

typedef struct {
	nrfx_gppi_handle_t gppi_handle;
	volatile int32_t grtc_channel;
	volatile uint32_t timer_capture_task_address;
	volatile uint32_t grtc_compare_event_address;
} timers_control;

int request_clock_spec(const struct device *clk_dev, const struct nrf_clock_spec *clk_spec);
uint32_t configure_test_timer(nrfx_timer_t *timer, uint32_t timer_frequency_Hz);
int configure_hfclock(void);
