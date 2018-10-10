/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "hid_event.h"

static const char * const target_report_name[] = {
#define X(name) STRINGIFY(name),
	TARGET_REPORT_LIST
#undef X
};


EVENT_TYPE_DEFINE(hid_keyboard_event, NULL, NULL);


static void print_mouse_event(const struct event_header *eh)
{
	struct hid_mouse_event *event = cast_hid_mouse_event(eh);

	printk("buttons:0x%x wheel:%d dx:%d dy:%d",
	       event->button_bm,
	       event->wheel,
	       event->dx,
	       event->dy);
}

static void log_args_mouse(struct log_event_buf *buf,
			   const struct event_header *eh)
{
	struct hid_mouse_event *event = cast_hid_mouse_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->button_bm);
	profiler_log_encode_u32(buf, event->wheel);
	profiler_log_encode_u32(buf, event->dx);
	profiler_log_encode_u32(buf, event->dy);
}

EVENT_INFO_DEFINE(hid_mouse_event,
		  ENCODE(PROFILER_ARG_U8, PROFILER_ARG_S32, PROFILER_ARG_S32,
			 PROFILER_ARG_S32),
		  ENCODE("buttons", "wheel", "dx", "dy"), log_args_mouse);
EVENT_TYPE_DEFINE(hid_mouse_event, print_mouse_event, &hid_mouse_event_info);


static void print_hid_report_sent_event(const struct event_header *eh)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(eh);

	printk("%s report sent", target_report_name[event->report_type]);
}

static void log_args_report_sent(struct log_event_buf *buf,
				 const struct event_header *eh)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->report_type);
}

EVENT_INFO_DEFINE(hid_report_sent_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("report_type"), log_args_report_sent);
EVENT_TYPE_DEFINE(hid_report_sent_event, print_hid_report_sent_event,
		  &hid_report_sent_event_info);

static void print_hid_report_subscription_event(const struct event_header *eh)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(eh);

	printk("%s report notification %sabled",
	       target_report_name[event->report_type],
	       (event->enabled)?("en"):("dis"));
}

static void log_args_report_subscription(struct log_event_buf *buf,
					 const struct event_header *eh)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->report_type);
	profiler_log_encode_u32(buf, event->enabled);
}

EVENT_INFO_DEFINE(hid_report_subscription_event,
		  ENCODE(PROFILER_ARG_U8, PROFILER_ARG_U8),
		  ENCODE("report_type", "enabled"), log_args_report_subscription);
EVENT_TYPE_DEFINE(hid_report_subscription_event, print_hid_report_subscription_event,
		  &hid_report_subscription_event_info);
