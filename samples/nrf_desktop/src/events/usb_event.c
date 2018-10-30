/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <misc/util.h>

#include "usb_event.h"


static const char * const state_name[] = {
	[USB_STATE_DISCONNECTED] = "DISCONNECTED",
	[USB_STATE_POWERED]      = "POWERED",
	[USB_STATE_ACTIVE]       = "ACTIVE",
	[USB_STATE_SUSPENDED]    = "SUSPENDED"
};

static void print_event(const struct event_header *eh)
{
	struct usb_state_event *event = cast_usb_state_event(eh);

	static_assert(ARRAY_SIZE(state_name) == USB_STATE_COUNT,
		      "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < USB_STATE_COUNT);

	printk("%s", state_name[event->state]);
}

EVENT_TYPE_DEFINE(usb_state_event, print_event, NULL);
