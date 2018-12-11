/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "hid_event.h"

static const char * const target_report_name[] = {
#define X(name) STRINGIFY(name),
	TARGET_REPORT_LIST
#undef X
};


EVENT_TYPE_DEFINE(hid_keyboard_event, NULL, NULL);


static int log_hid_mouse_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	struct hid_mouse_event *event = cast_hid_mouse_event(eh);

	return snprintf(buf, buf_len,
			"buttons:0x%x wheel:%d dx:%d dy:%d => %p",
			event->button_bm, event->wheel, event->dx, event->dy,
			event->subscriber);
}

static void profile_hid_mouse_event(struct log_event_buf *buf,
			   const struct event_header *eh)
{
	struct hid_mouse_event *event = cast_hid_mouse_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->button_bm);
	profiler_log_encode_u32(buf, event->wheel);
	profiler_log_encode_u32(buf, event->dx);
	profiler_log_encode_u32(buf, event->dy);
}

EVENT_INFO_DEFINE(hid_mouse_event,
		  ENCODE(PROFILER_ARG_S32, PROFILER_ARG_U8, PROFILER_ARG_S32,
			 PROFILER_ARG_S32, PROFILER_ARG_S32),
		  ENCODE("subscriber", "buttons", "wheel", "dx", "dy"),
		  profile_hid_mouse_event);

EVENT_TYPE_DEFINE(hid_mouse_event, log_hid_mouse_event, &hid_mouse_event_info);

static int log_hid_report_subscriber_event(const struct event_header *eh,
					      char *buf, size_t buf_len)
{
	const struct hid_report_subscriber_event *event =
		cast_hid_report_subscriber_event(eh);

	return snprintf(buf, buf_len,
			"report subscriber %p was %sconnected",
			event->subscriber, (event->connected)?(""):("dis"));
}

static void profile_hid_report_subscriber_event(struct log_event_buf *buf,
						const struct event_header *eh)
{
	const struct hid_report_subscriber_event *event =
		cast_hid_report_subscriber_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->connected);
}

EVENT_INFO_DEFINE(hid_report_subscriber_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8),
		  ENCODE("subscriber", "connected"),
		  profile_hid_report_subscriber_event);

EVENT_TYPE_DEFINE(hid_report_subscriber_event, log_hid_report_subscriber_event,
		  &hid_report_subscriber_event_info);

static int log_hid_report_sent_event(const struct event_header *eh,
					char *buf, size_t buf_len)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(eh);

	if (event->error) {
		return snprintf(buf, buf_len,
				"error while sending %s report by %p",
				target_report_name[event->report_type],
				event->subscriber);
	} else {
		return snprintf(buf, buf_len,
				"%s report sent by %p",
				target_report_name[event->report_type],
				event->subscriber);
	}
}

static void profile_hid_report_sent_event(struct log_event_buf *buf,
						const struct event_header *eh)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->report_type);
	profiler_log_encode_u32(buf, event->error);
}

EVENT_INFO_DEFINE(hid_report_sent_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8, PROFILER_ARG_U8),
		  ENCODE("subscriber", "report_type", "error"),
		  profile_hid_report_sent_event);
EVENT_TYPE_DEFINE(hid_report_sent_event, log_hid_report_sent_event,
		  &hid_report_sent_event_info);

static int log_hid_report_subscription_event(const struct event_header *eh,
						char *buf, size_t buf_len)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(eh);

	return snprintf(buf, buf_len,
			"%s report notification %sabled by %p",
			target_report_name[event->report_type],
			(event->enabled)?("en"):("dis"), event->subscriber);
}

static void profile_hid_report_subscription_event(struct log_event_buf *buf,
						  const struct event_header *eh)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->report_type);
	profiler_log_encode_u32(buf, event->enabled);
}

EVENT_INFO_DEFINE(hid_report_subscription_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8, PROFILER_ARG_U8),
		  ENCODE("subscriber", "report_type", "enabled"),
		  profile_hid_report_subscription_event);

EVENT_TYPE_DEFINE(hid_report_subscription_event,
		  log_hid_report_subscription_event,
		  &hid_report_subscription_event_info);
