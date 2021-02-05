/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

static struct onoff_manager *mgr;
static struct onoff_client cli;

static void hfclk_lock(void)
{
	if (mgr) {
		return;
	}

	mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!mgr) {
		module_set_state(MODULE_STATE_ERROR);
	} else {
		int err;

		sys_notify_init_spinwait(&cli.notify);
		err = onoff_request(mgr, &cli);
		if (err < 0) {
			mgr = NULL;
			module_set_state(MODULE_STATE_ERROR);
		} else {
			module_set_state(MODULE_STATE_READY);
		}
	}
}

static void hfclk_unlock(void)
{
	int err;

	if (!mgr) {
		return;
	}

	err = onoff_cancel_or_release(mgr, &cli);
	module_set_state((err < 0) ? MODULE_STATE_ERROR : MODULE_STATE_OFF);
	mgr = NULL;
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
