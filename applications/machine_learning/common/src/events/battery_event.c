/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <assert.h>
#include <stdio.h>

#include "battery_event.h"

static void log_battery_level_event(const struct app_event_header *aeh)
{
	const struct battery_level_event *event = cast_battery_level_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "level=%u", event->level);
}

APP_EVENT_TYPE_DEFINE(battery_level_event,
		  log_battery_level_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_ML_APP_INIT_LOG_BATTERY_LEVEL_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
