/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 *   This file implements the nrf 802.15.4 HF Clock abstraction with Zephyr API.
 *
 * This implementation uses Zephyr API for clock management.
 */

#include <platform/clock/nrf_802154_clock.h>

#include <stddef.h>

#include <compiler_abstraction.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/clock_control.h>

static bool hfclk_is_running;
static bool lfclk_is_running;

void nrf_802154_clock_init(void)
{
	/* Intentionally empty. */
}

void nrf_802154_clock_deinit(void)
{
	/* Intentionally empty. */
}

static void hfclk_on_callback(struct device *dev, clock_control_subsys_t subsys,
			      void *user_data)
{
	hfclk_is_running = true;
	nrf_802154_clock_hfclk_ready();
}

void nrf_802154_clock_hfclk_start(void)
{
	static struct clock_control_async_data clk_data = {
		.cb = hfclk_on_callback
	};
	struct device *clk;

	clk = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)));
	__ASSERT_NO_MSG(clk != NULL);

	clock_control_async_on(clk, CLOCK_CONTROL_NRF_SUBSYS_HF, &clk_data);
}

void nrf_802154_clock_hfclk_stop(void)
{
	struct device *clk;

	clk = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)));
	__ASSERT_NO_MSG(clk != NULL);

	hfclk_is_running = false;

	clock_control_off(clk, CLOCK_CONTROL_NRF_SUBSYS_HF);
}

bool nrf_802154_clock_hfclk_is_running(void)
{
	return hfclk_is_running;
}

static void lfclk_on_callback(struct device *dev, clock_control_subsys_t subsys,
			      void *user_data)
{
	lfclk_is_running = true;
	nrf_802154_clock_lfclk_ready();
}

void nrf_802154_clock_lfclk_start(void)
{
	static struct clock_control_async_data clk_data = {
		.cb = lfclk_on_callback
	};
	struct device *clk;

	clk = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)));
	__ASSERT_NO_MSG(clk != NULL);

	clock_control_async_on(clk, CLOCK_CONTROL_NRF_SUBSYS_LF, &clk_data);
}

void nrf_802154_clock_lfclk_stop(void)
{
	struct device *clk;

	clk = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)));
	__ASSERT_NO_MSG(clk != NULL);

	lfclk_is_running = false;

	clock_control_off(clk, CLOCK_CONTROL_NRF_SUBSYS_LF);
}

bool nrf_802154_clock_lfclk_is_running(void)
{
	return lfclk_is_running;
}

__WEAK void nrf_802154_clock_hfclk_ready(void)
{
	/* Intentionally empty. */
}

__WEAK void nrf_802154_clock_lfclk_ready(void)
{
	/* Intentionally empty. */
}
