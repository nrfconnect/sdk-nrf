/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <hal/nrf_power.h>
#include <power/power.h>
#include <power/reboot.h>

#define MODULE power_handler
#include "module_state_event.h"
#include "power_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_POWER_MGMT_LOG_LEVEL);

static struct k_delayed_work power_down_work;
static int module_active_count;

static void power_down_handler(struct k_work *work)
{
	if (module_active_count == 0) {
		struct power_down_event *event = new_power_down_event();

		event->error = 0;
		EVENT_SUBMIT(event);

		/* Keep submitting events as they may be consumed to delay shutdown */
		k_delayed_work_submit(&power_down_work, K_SECONDS(1));
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			module_active_count = 0;

			k_delayed_work_init(&power_down_work, power_down_handler);
		} else if (event->state == MODULE_STATE_READY) {
			module_active_count += 1;
		} else if (event->state == MODULE_STATE_STANDBY) {
			module_active_count -= 1;
			__ASSERT_NO_MSG(module_active_count >= 0);

			if (module_active_count == 0) {
				k_delayed_work_submit(&power_down_work, K_SECONDS(1));
			}
		}
		return false;
	}

	if (is_power_down_event(eh)) {
		nrf_power_system_off(NRF_POWER);
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_FINAL(MODULE, power_down_event);
