/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "hid_event.h"

static const char * const in_report_name[] = {
#define X(name) STRINGIFY(name),
	IN_REPORT_LIST
#undef X
};


static int log_hid_keyboard_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct hid_keyboard_event *event = cast_hid_keyboard_event(eh);

	char keys_str[ARRAY_SIZE(event->keys) * 5 + 1];
	int pos = 0;

	BUILD_ASSERT_MSG(sizeof(event->keys[0]) == 1, "");
	for (size_t i = 0; i < ARRAY_SIZE(event->keys); i++) {
		int tmp = snprintf(&keys_str[pos], sizeof(keys_str) - pos,
				   "0x%02x ", event->keys[i]);
		if (tmp < 0) {
			return tmp;
		}
		pos += tmp;
		if (pos > sizeof(keys_str)) {
			break;
		}
	}

	return snprintf(buf, buf_len,
			"mod:0x%x keys:%s => %p",
			event->modifier_bm, keys_str, event->subscriber);
}

EVENT_TYPE_DEFINE(hid_keyboard_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_KEYBOARD_EVENT),
		  log_hid_keyboard_event,
		  NULL);


static int log_hid_mouse_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct hid_mouse_event *event = cast_hid_mouse_event(eh);

	return snprintf(buf, buf_len,
			"buttons:0x%x wheel:%d dx:%d dy:%d => %p",
			event->button_bm, event->wheel, event->dx, event->dy,
			event->subscriber);
}

static void profile_hid_mouse_event(struct log_event_buf *buf,
			   const struct event_header *eh)
{
	const struct hid_mouse_event *event = cast_hid_mouse_event(eh);

	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->button_bm);
	profiler_log_encode_u32(buf, event->wheel);
	profiler_log_encode_u32(buf, event->dx);
	profiler_log_encode_u32(buf, event->dy);
}

EVENT_INFO_DEFINE(hid_mouse_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8, PROFILER_ARG_S32,
			 PROFILER_ARG_S32, PROFILER_ARG_S32),
		  ENCODE("subscriber", "buttons", "wheel", "dx", "dy"),
		  profile_hid_mouse_event);

EVENT_TYPE_DEFINE(hid_mouse_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_MOUSE_EVENT),
		  log_hid_mouse_event,
		  &hid_mouse_event_info);

static int log_hid_ctrl_event(const struct event_header *eh, char *buf,
			      size_t buf_len)
{
	const struct hid_ctrl_event *event = cast_hid_ctrl_event(eh);

	return snprintf(buf, buf_len,
			"usage: 0x%" PRIx16 "(%s) => %p",
			event->usage, in_report_name[event->report_type],
			event->subscriber);
}

static void profile_hid_ctrl_event(struct log_event_buf *buf,
				   const struct event_header *eh)
{
	const struct hid_ctrl_event *event = cast_hid_ctrl_event(eh);

	profiler_log_encode_u32(buf, (u32_t)event->report_type);
	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->usage);
}

EVENT_INFO_DEFINE(hid_ctrl_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U32, PROFILER_ARG_U16),
		  ENCODE("report_type", "subscriber", "usage"),
		  profile_hid_ctrl_event);

EVENT_TYPE_DEFINE(hid_ctrl_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_CTRL_EVENT),
		  log_hid_ctrl_event,
		  &hid_ctrl_event_info);

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

	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->connected);
}

EVENT_INFO_DEFINE(hid_report_subscriber_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8),
		  ENCODE("subscriber", "connected"),
		  profile_hid_report_subscriber_event);

EVENT_TYPE_DEFINE(hid_report_subscriber_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_SUBSCRIBER_EVENT),
		  log_hid_report_subscriber_event,
		  &hid_report_subscriber_event_info);

static int log_hid_report_sent_event(const struct event_header *eh,
					char *buf, size_t buf_len)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(eh);

	if (event->error) {
		return snprintf(buf, buf_len,
				"error while sending %s report by %p",
				in_report_name[event->report_type],
				event->subscriber);
	} else {
		return snprintf(buf, buf_len,
				"%s report sent by %p",
				in_report_name[event->report_type],
				event->subscriber);
	}
}

static void profile_hid_report_sent_event(struct log_event_buf *buf,
						const struct event_header *eh)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(eh);

	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->report_type);
	profiler_log_encode_u32(buf, event->error);
}

EVENT_INFO_DEFINE(hid_report_sent_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8, PROFILER_ARG_U8),
		  ENCODE("subscriber", "report_type", "error"),
		  profile_hid_report_sent_event);

EVENT_TYPE_DEFINE(hid_report_sent_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_REPORT_SENT_EVENT),
		  log_hid_report_sent_event,
		  &hid_report_sent_event_info);

static int log_hid_report_subscription_event(const struct event_header *eh,
						char *buf, size_t buf_len)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(eh);

	return snprintf(buf, buf_len,
			"%s report notification %sabled by %p",
			in_report_name[event->report_type],
			(event->enabled)?("en"):("dis"), event->subscriber);
}

static void profile_hid_report_subscription_event(struct log_event_buf *buf,
						  const struct event_header *eh)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(eh);

	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->report_type);
	profiler_log_encode_u32(buf, event->enabled);
}

EVENT_INFO_DEFINE(hid_report_subscription_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8, PROFILER_ARG_U8),
		  ENCODE("subscriber", "report_type", "enabled"),
		  profile_hid_report_subscription_event);

EVENT_TYPE_DEFINE(hid_report_subscription_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_SUBSCRIPTION_EVENT),
		  log_hid_report_subscription_event,
		  &hid_report_subscription_event_info);
