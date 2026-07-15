/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "hids_event.h"

static void log_hid_notification_event(const struct app_event_header *aeh)
{
	const struct hid_notification_event *event =
		cast_hid_notification_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh,
			"report_id 0x%x %sabled",
			event->report_id, (event->enabled)?"en":"dis");
}

APP_EVENT_TYPE_DEFINE(hid_notification_event,
		  log_hid_notification_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_NOTIFICATION_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

#if CONFIG_BT_HIDS_SCI
static void log_hid_sci_mode_request_event(const struct app_event_header *aeh)
{
	const struct hid_sci_mode_request_event *event =
		cast_hid_sci_mode_request_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "SCI mode request 0x%02" PRIx8 " from %p",
			      (uint8_t)event->mode, event->subscriber);
}

APP_EVENT_TYPE_DEFINE(hid_sci_mode_request_event,
		  log_hid_sci_mode_request_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_SCI_MODE_REQUEST_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
#endif /* CONFIG_BT_HIDS_SCI */

static void log_hid_host_suspend_event(const struct app_event_header *aeh)
{
	const struct hid_host_suspend_event *event = cast_hid_host_suspend_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "host %s from %p",
			      event->suspended ? "suspended" : "resumed", event->subscriber);
}

APP_EVENT_TYPE_DEFINE(hid_host_suspend_event,
		  log_hid_host_suspend_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_HOST_SUSPEND_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
