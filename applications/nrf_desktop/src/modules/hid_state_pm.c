/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include "hid_event.h"

#define MODULE hid_state_pm
#include <caf/events/module_state_event.h>
#include <caf/events/keep_alive_event.h>
#include <caf/events/power_manager_event.h>

static struct module_flags pm_alive_restrict_flags;
static bool pm_alive_enforced;


static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_hid_report_event(aeh)) {
		if (!pm_alive_enforced) {
			keep_alive();
		}
		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER_EVENTS) &&
	    is_power_manager_restrict_event(aeh)) {
		const struct power_manager_restrict_event *event =
			cast_power_manager_restrict_event(aeh);

		module_flags_set_bit_to(&pm_alive_restrict_flags,
					event->module_idx,
					event->level == POWER_MANAGER_LEVEL_ALIVE);
		/* Cache the result to improve performance. */
		pm_alive_enforced = !module_flags_check_zero(&pm_alive_restrict_flags);

		return false;
	}

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_event);
#if CONFIG_CAF_POWER_MANAGER_EVENTS
APP_EVENT_SUBSCRIBE_EARLY(MODULE, power_manager_restrict_event);
#endif /* CONFIG_CAF_POWER_MANAGER_EVENTS */
