/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <hal/nrf_power.h>

#define MODULE constlat
#include "module_state_event.h"

#include "power_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE);


static bool enabled;


static void constlat_on(void)
{
	if (enabled) {
		return;
	}

	nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_CONSTLAT);
	LOG_INF("Constant latency enabled");

	enabled = true;
}

static void constlat_off(void)
{
	if (!enabled) {
		return;
	}

	nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_LOWPWR);
	LOG_INF("Constant latency disabled");

	enabled = false;
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

			constlat_on();
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONSTLAT_DISABLE_ON_STANDBY) &&
	    is_power_down_event(eh)) {
		constlat_off();
		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONSTLAT_DISABLE_ON_STANDBY) &&
	    is_wake_up_event(eh)) {
		constlat_on();
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_CONSTLAT_DISABLE_ON_STANDBY
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
#endif
