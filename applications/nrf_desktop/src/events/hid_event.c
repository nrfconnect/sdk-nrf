/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "hid_event.h"

#define HID_EVENT_LOG_BUF_LEN 128


static int log_hid_report_event(const struct event_header *eh, char *buf,
				size_t buf_len)
{
	const struct hid_report_event *event = cast_hid_report_event(eh);
	int pos;
	char log_buf[HID_EVENT_LOG_BUF_LEN];

	__ASSERT_NO_MSG(event->dyndata.size > 0);

	pos = snprintf(log_buf, sizeof(log_buf), "Report 0x%x src:%p sub:%p:",
		       event->dyndata.data[0],
		       event->source,
		       event->subscriber);
	if ((pos > 0) && (pos < sizeof(log_buf))) {
		for (size_t i = 1; i < event->dyndata.size; i++) {
			int tmp = snprintf(&log_buf[pos], sizeof(log_buf) - pos,
					   " 0x%.2x", event->dyndata.data[i]);
			if (tmp < 0) {
				log_buf[sizeof(log_buf) - 2] = '~';
				pos = tmp;
				break;
			}

			pos += tmp;

			if (pos >= sizeof(log_buf))
				break;
		}
	}
	if (pos < 0) {
		EVENT_MANAGER_LOG(eh, "log message preparation failure");
		return pos;
	}
	EVENT_MANAGER_LOG(eh, "%s", log_strdup(log_buf));
	return 0;
}

static void profile_hid_report_event(struct log_event_buf *buf,
				     const struct event_header *eh)
{
	const struct hid_report_event *event = cast_hid_report_event(eh);

	__ASSERT_NO_MSG(event->dyndata.size > 0);

	profiler_log_encode_uint8(buf, event->dyndata.data[0]);
	profiler_log_encode_uint32(buf, (uint32_t)event->source);
	profiler_log_encode_uint32(buf, (uint32_t)event->subscriber);
}

EVENT_INFO_DEFINE(hid_report_event,
		  ENCODE(PROFILER_ARG_U8, PROFILER_ARG_U32, PROFILER_ARG_U32),
		  ENCODE("report_id", "source", "subscriber"),
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

	EVENT_MANAGER_LOG(eh, "report subscriber %p was %sconnected",
			event->subscriber, (event->connected)?(""):("dis"));
	return 0;
}

static void profile_hid_report_subscriber_event(struct log_event_buf *buf,
						const struct event_header *eh)
{
	const struct hid_report_subscriber_event *event =
		cast_hid_report_subscriber_event(eh);

	profiler_log_encode_uint32(buf, (uint32_t)event->subscriber);
	profiler_log_encode_uint8(buf, event->connected);
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
		EVENT_MANAGER_LOG(eh,
				"error while sending 0x%x report by %p",
				event->report_id,
				event->subscriber);
	} else {
		EVENT_MANAGER_LOG(eh,
				"report 0x%x sent by %p",
				event->report_id,
				event->subscriber);
	}
	return 0;
}

static void profile_hid_report_sent_event(struct log_event_buf *buf,
						const struct event_header *eh)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(eh);

	profiler_log_encode_uint32(buf, (uint32_t)event->subscriber);
	profiler_log_encode_uint8(buf, event->report_id);
	profiler_log_encode_uint8(buf, event->error);
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

	EVENT_MANAGER_LOG(eh,
			"report 0x%x notification %sabled by %p",
			event->report_id,
			(event->enabled)?("en"):("dis"), event->subscriber);
	return 0;
}

static void profile_hid_report_subscription_event(struct log_event_buf *buf,
						  const struct event_header *eh)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(eh);

	profiler_log_encode_uint32(buf, (uint32_t)event->subscriber);
	profiler_log_encode_uint8(buf, event->report_id);
	profiler_log_encode_uint8(buf, event->enabled);
}

EVENT_INFO_DEFINE(hid_report_subscription_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8, PROFILER_ARG_U8),
		  ENCODE("subscriber", "report_id", "enabled"),
		  profile_hid_report_subscription_event);

EVENT_TYPE_DEFINE(hid_report_subscription_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_SUBSCRIPTION_EVENT),
		  log_hid_report_subscription_event,
		  &hid_report_subscription_event_info);
