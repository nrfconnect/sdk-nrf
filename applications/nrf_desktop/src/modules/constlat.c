/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <hal/nrf_power.h>

#define MODULE constlat
#include <caf/events/module_state_event.h>

#include <caf/events/power_event.h>

#include <zephyr/logging/log.h>
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

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
				cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			constlat_on();
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONSTLAT_DISABLE_ON_STANDBY) &&
	    is_power_down_event(aeh)) {
		constlat_off();
		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONSTLAT_DISABLE_ON_STANDBY) &&
	    is_wake_up_event(aeh)) {
		constlat_on();
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_CONSTLAT_DISABLE_ON_STANDBY
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
#endif
