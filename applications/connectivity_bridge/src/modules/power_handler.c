/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <hal/nrf_power.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/reboot.h>

#define MODULE power_handler
#include "module_state_event.h"
#include "power_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_POWER_MGMT_LOG_LEVEL);

static void power_down_handler(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(power_down_work, power_down_handler);
static int module_active_count;

static void power_down_handler(struct k_work *work)
{
	if (module_active_count == 0) {
		struct power_down_event *event = new_power_down_event();

		event->error = 0;
		APP_EVENT_SUBMIT(event);

		/* Keep submitting events as they may be consumed to delay shutdown */
		k_work_reschedule(&power_down_work, K_SECONDS(1));
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			module_active_count = 0;
		} else if (event->state == MODULE_STATE_READY) {
			module_active_count += 1;
		} else if (event->state == MODULE_STATE_STANDBY) {
			module_active_count -= 1;
			__ASSERT_NO_MSG(module_active_count >= 0);

			if (module_active_count == 0) {
				k_work_reschedule(&power_down_work, K_SECONDS(1));
			}
		}
		return false;
	}

	if (is_power_down_event(aeh)) {
#if defined(CONFIG_SOC_SERIES_NRF52X)
		nrf_power_system_off(NRF_POWER);
#endif
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, power_down_event);
