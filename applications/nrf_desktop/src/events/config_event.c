/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "config_event.h"

static const char * const status_name[] = {
#define X(name) STRINGIFY(name),
	CONFIG_STATUS_LIST
#undef X
};

static void log_config_event(const struct app_event_header *aeh)
{
	const struct config_event *event = cast_config_event(aeh);
	const char *status_str = "UNKNOWN";

	if (event->status < ARRAY_SIZE(status_name)) {
		status_str = status_name[event->status];
	}

	APP_EVENT_MANAGER_LOG(aeh, "%s %s rcpt: %02x id: %02x",
			status_str,
			event->is_request ? "req" : "rsp",
			event->recipient,
			event->event_id);
}

APP_EVENT_TYPE_DEFINE(config_event,
		  log_config_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_CONFIG_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
