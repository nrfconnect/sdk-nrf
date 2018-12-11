/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <misc/util.h>
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
	struct usb_state_event *event = cast_usb_state_event(eh);

	static_assert(ARRAY_SIZE(state_name) == USB_STATE_COUNT,
		      "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < USB_STATE_COUNT);

	return snprintf(buf, buf_len, "id:%p state:%s", event->id,
			state_name[event->state]);
}

EVENT_TYPE_DEFINE(usb_state_event, log_usb_state_event, NULL);
