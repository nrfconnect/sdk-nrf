/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "hogp_event.h"

static void log_hogp_sci_mode_changed_event(const struct app_event_header *aeh)
{
	const struct hogp_sci_mode_changed_event *event =
		cast_hogp_sci_mode_changed_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "SCI mode changed to 0x%02" PRIx8 " on conn %p",
			      (uint8_t)event->mode, (void *)event->conn);
}

APP_EVENT_TYPE_DEFINE(hogp_sci_mode_changed_event,
		      log_hogp_sci_mode_changed_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HOGP_SCI_MODE_CHANGED_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

static void log_hogp_sci_mode_req_event(const struct app_event_header *aeh)
{
	const struct hogp_sci_mode_req_event *event = cast_hogp_sci_mode_req_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "SCI mode request 0x%02" PRIx8 " for conn %p",
			      (uint8_t)event->mode, (void *)event->conn);
}

APP_EVENT_TYPE_DEFINE(hogp_sci_mode_req_event,
		      log_hogp_sci_mode_req_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HOGP_SCI_MODE_REQ_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
