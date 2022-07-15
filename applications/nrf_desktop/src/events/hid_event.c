/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "hid_event.h"

#define HID_EVENT_LOG_BUF_LEN 128


static void log_hid_report_event(const struct app_event_header *aeh)
{
	const struct hid_report_event *event = cast_hid_report_event(aeh);
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
		APP_EVENT_MANAGER_LOG(aeh, "log message preparation failure");
		return;
	}
	APP_EVENT_MANAGER_LOG(aeh, "%s", log_buf);
}

static void profile_hid_report_event(struct log_event_buf *buf,
				     const struct app_event_header *aeh)
{
	const struct hid_report_event *event = cast_hid_report_event(aeh);

	__ASSERT_NO_MSG(event->dyndata.size > 0);

	nrf_profiler_log_encode_uint8(buf, event->dyndata.data[0]);
	nrf_profiler_log_encode_uint32(buf, (uint32_t)event->source);
	nrf_profiler_log_encode_uint32(buf, (uint32_t)event->subscriber);
}

APP_EVENT_INFO_DEFINE(hid_report_event,
		  ENCODE(NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U32),
		  ENCODE("report_id", "source", "subscriber"),
		  profile_hid_report_event);

APP_EVENT_TYPE_DEFINE(hid_report_event,
		  log_hid_report_event,
		  &hid_report_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_REPORT_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

static void log_hid_report_subscriber_event(const struct app_event_header *aeh)
{
	const struct hid_report_subscriber_event *event =
		cast_hid_report_subscriber_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "report subscriber %p was %sconnected",
			event->subscriber, (event->connected)?(""):("dis"));
}

static void profile_hid_report_subscriber_event(struct log_event_buf *buf,
						const struct app_event_header *aeh)
{
	const struct hid_report_subscriber_event *event =
		cast_hid_report_subscriber_event(aeh);

	nrf_profiler_log_encode_uint32(buf, (uint32_t)event->subscriber);
	nrf_profiler_log_encode_uint8(buf, event->connected);
}

APP_EVENT_INFO_DEFINE(hid_report_subscriber_event,
		  ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U8),
		  ENCODE("subscriber", "connected"),
		  profile_hid_report_subscriber_event);

APP_EVENT_TYPE_DEFINE(hid_report_subscriber_event,
		  log_hid_report_subscriber_event,
		  &hid_report_subscriber_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_SUBSCRIBER_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

static void log_hid_report_sent_event(const struct app_event_header *aeh)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(aeh);

	if (event->error) {
		APP_EVENT_MANAGER_LOG(aeh,
				"error while sending 0x%x report by %p",
				event->report_id,
				event->subscriber);
	} else {
		APP_EVENT_MANAGER_LOG(aeh,
				"report 0x%x sent by %p",
				event->report_id,
				event->subscriber);
	}
}

static void profile_hid_report_sent_event(struct log_event_buf *buf,
						const struct app_event_header *aeh)
{
	const struct hid_report_sent_event *event =
		cast_hid_report_sent_event(aeh);

	nrf_profiler_log_encode_uint32(buf, (uint32_t)event->subscriber);
	nrf_profiler_log_encode_uint8(buf, event->report_id);
	nrf_profiler_log_encode_uint8(buf, event->error);
}

APP_EVENT_INFO_DEFINE(hid_report_sent_event,
		  ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U8),
		  ENCODE("subscriber", "report_id", "error"),
		  profile_hid_report_sent_event);

APP_EVENT_TYPE_DEFINE(hid_report_sent_event,
		  log_hid_report_sent_event,
		  &hid_report_sent_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_REPORT_SENT_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

static void log_hid_report_subscription_event(const struct app_event_header *aeh)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh,
			"report 0x%x notification %sabled by %p",
			event->report_id,
			(event->enabled)?("en"):("dis"), event->subscriber);
}

static void profile_hid_report_subscription_event(struct log_event_buf *buf,
						  const struct app_event_header *aeh)
{
	const struct hid_report_subscription_event *event =
		cast_hid_report_subscription_event(aeh);

	nrf_profiler_log_encode_uint32(buf, (uint32_t)event->subscriber);
	nrf_profiler_log_encode_uint8(buf, event->report_id);
	nrf_profiler_log_encode_uint8(buf, event->enabled);
}

APP_EVENT_INFO_DEFINE(hid_report_subscription_event,
		  ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U8),
		  ENCODE("subscriber", "report_id", "enabled"),
		  profile_hid_report_subscription_event);

APP_EVENT_TYPE_DEFINE(hid_report_subscription_event,
		  log_hid_report_subscription_event,
		  &hid_report_subscription_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_SUBSCRIPTION_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
