/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "hid_event.h"

static void print_event(const struct event_header *eh)
{
}

static void log_args_mouse_xy(struct log_event_buf *buf,
			     const struct event_header *eh)
{
	struct hid_mouse_xy_event *event = cast_hid_mouse_xy_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->dx);
	profiler_log_encode_u32(buf, event->dy);
}

static void log_args_mouse_wp(struct log_event_buf *buf,
			     const struct event_header *eh)
{
	struct hid_mouse_wp_event *event = cast_hid_mouse_wp_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->wheel);
	profiler_log_encode_u32(buf, event->pan);
}

EVENT_TYPE_DEFINE(hid_keyboard_event, print_event, NULL);

EVENT_INFO_DEFINE(hid_mouse_xy_event,
		  ENCODE(PROFILER_ARG_S32, PROFILER_ARG_S32),
		  ENCODE("dx", "dy"), log_args_mouse_xy);
EVENT_TYPE_DEFINE(hid_mouse_xy_event, print_event, &hid_mouse_xy_event_info);

EVENT_INFO_DEFINE(hid_mouse_wp_event,
		  ENCODE(PROFILER_ARG_S32, PROFILER_ARG_S32),
		  ENCODE("wheel", "pan"), log_args_mouse_wp);
EVENT_TYPE_DEFINE(hid_mouse_wp_event, print_event, &hid_mouse_wp_event_info);

EVENT_TYPE_DEFINE(hid_mouse_button_event, print_event, NULL);
