/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "hids_event.h"

static int log_hid_notification_event(const struct event_header *eh, char *buf,
				      size_t buf_len)
{
	const struct hid_notification_event *event =
		cast_hid_notification_event(eh);

	return snprintf(buf, buf_len,
			"report:0x%x mode:0x%x event:0x%x",
			event->report_type, event->report_mode, event->event);
}

EVENT_TYPE_DEFINE(hid_notification_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_NOTIFICATION_EVENT),
		  log_hid_notification_event,
		  NULL);
