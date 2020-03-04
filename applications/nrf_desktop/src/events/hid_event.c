/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "hid_event.h"


static int log_hid_report_event(const struct event_header *eh, char *buf,
				size_t buf_len)
{
	const struct hid_report_event *event = cast_hid_report_event(eh);
	int pos;

	__ASSERT_NO_MSG(event->dyndata.size > 0);

	pos = snprintf(buf, buf_len, "Report 0x%x send to %p:",
		       event->dyndata.data[0],
		       event->subscriber);
	if ((pos > 0) && (pos < buf_len)) {
		for (size_t i = 1; i < event->dyndata.size; i++) {
			int tmp = snprintf(&buf[pos], buf_len - pos,
					   " 0x%.2x", event->dyndata.data[i]);
			if (tmp < 0) {
				pos = tmp;
				break;
			}

			pos += tmp;

			if (pos >= buf_len)
				break;
		}
	}

	return pos;
}

static void profile_hid_report_event(struct log_event_buf *buf,
				     const struct event_header *eh)
{
	const struct hid_report_event *event = cast_hid_report_event(eh);

	__ASSERT_NO_MSG(event->dyndata.size > 0);

	profiler_log_encode_u32(buf, (u32_t)event->dyndata.data[0]);
	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
}

EVENT_INFO_DEFINE(hid_report_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U32),
		  ENCODE("report_id", "subscriber"),
		  profile_hid_report_event);


EVENT_TYPE_DEFINE(hid_report_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_REPORT_EVENT),
		  log_hid_report_event,
		  &hid_report_event_info);

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
				"error while sending 0x%x report by %p",
				event->report_id,
				event->subscriber);
	} else {
		return snprintf(buf, buf_len,
				"report 0x%x sent by %p",
				event->report_id,
				event->subscriber);
	}
}

static void profile_hid_report_sent_event(struct log_event_buf *buf,
						const struct event_header *eh)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(eh);

	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->report_id);
	profiler_log_encode_u32(buf, event->error);
}

EVENT_INFO_DEFINE(hid_report_sent_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8, PROFILER_ARG_U8),
		  ENCODE("subscriber", "report_id", "error"),
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
			"report 0x%x notification %sabled by %p",
			event->report_id,
			(event->enabled)?("en"):("dis"), event->subscriber);
}

static void profile_hid_report_subscription_event(struct log_event_buf *buf,
						  const struct event_header *eh)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(eh);

	profiler_log_encode_u32(buf, (u32_t)event->subscriber);
	profiler_log_encode_u32(buf, event->report_id);
	profiler_log_encode_u32(buf, event->enabled);
}

EVENT_INFO_DEFINE(hid_report_subscription_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8, PROFILER_ARG_U8),
		  ENCODE("subscriber", "report_id", "enabled"),
		  profile_hid_report_subscription_event);

EVENT_TYPE_DEFINE(hid_report_subscription_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_SUBSCRIPTION_EVENT),
		  log_hid_report_subscription_event,
		  &hid_report_subscription_event_info);
