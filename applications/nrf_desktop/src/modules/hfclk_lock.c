/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

#define MODULE hfclk_lock
#include "module_state_event.h"

#include "power_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE);


static struct device *clk_dev;


static void hfclk_lock(void)
{
	if (clk_dev) {
		return;
	}

	clk_dev = device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL);

	if (!clk_dev) {
		module_set_state(MODULE_STATE_ERROR);
	} else {
		clock_control_on(clk_dev, CLOCK_CONTROL_NRF_SUBSYS_HF);
		module_set_state(MODULE_STATE_READY);
	}
}

static void hfclk_unlock(void)
{
	if (!clk_dev) {
		return;
	}

	clock_control_off(clk_dev, CLOCK_CONTROL_NRF_SUBSYS_HF);

	clk_dev = NULL;

	module_set_state(MODULE_STATE_OFF);
}


static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
				cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			hfclk_lock();
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		hfclk_unlock();
		return false;
	}

	if (is_wake_up_event(eh)) {
		hfclk_lock();
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
