/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "hids_event.h"

static void log_hid_notification_event(const struct event_header *eh)
{
	const struct hid_notification_event *event =
		cast_hid_notification_event(eh);

	EVENT_MANAGER_LOG(eh,
			"report_id 0x%x %sabled",
			event->report_id, (event->enabled)?"en":"dis");
}

EVENT_TYPE_DEFINE(hid_notification_event,
		  log_hid_notification_event,
		  NULL,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_NOTIFICATION_EVENT,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
