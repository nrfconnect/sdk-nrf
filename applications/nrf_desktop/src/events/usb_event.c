/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <sys/util.h>
#include <stdio.h>

#include "usb_event.h"


static const char * const state_name[] = {
	[USB_STATE_DISCONNECTED] = "DISCONNECTED",
	[USB_STATE_POWERED]      = "POWERED",
	[USB_STATE_ACTIVE]       = "ACTIVE",
	[USB_STATE_SUSPENDED]    = "SUSPENDED"
};

static int log_usb_state_event(const struct event_header *eh, char *buf,
			       size_t buf_len)
{
	const struct usb_state_event *event = cast_usb_state_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == USB_STATE_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < USB_STATE_COUNT);

	return snprintf(buf, buf_len, "state:%s", state_name[event->state]);
}

EVENT_TYPE_DEFINE(usb_state_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_USB_STATE_EVENT),
		  log_usb_state_event,
		  NULL);

static int log_usb_hid_event(const struct event_header *eh, char *buf,
			     size_t buf_len)
{
	const struct usb_hid_event *event = cast_usb_hid_event(eh);

	return snprintf(buf, buf_len, "id:%p %sabled", event->id,
			(event->enabled)?("en"):("dis"));
}

EVENT_TYPE_DEFINE(usb_hid_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_USB_HID_EVENT),
		  log_usb_hid_event,
		  NULL);
