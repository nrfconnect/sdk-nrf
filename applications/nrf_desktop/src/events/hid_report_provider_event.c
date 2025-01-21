/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "hid_report_provider_event.h"


static void log_hid_report_provider_event(const struct app_event_header *aeh)
{
	const struct hid_report_provider_event *event = cast_hid_report_provider_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "Report ID:0x%" PRIx8 ", HID provider:%p, HID state:%p",
			      event->report_id, (const void *)event->provider_api,
			      (const void *)event->hid_state_api);
}

static void profile_hid_report_provider_event(struct log_event_buf *buf,
					      const struct app_event_header *aeh)
{
	const struct hid_report_provider_event *event = cast_hid_report_provider_event(aeh);

	nrf_profiler_log_encode_uint8(buf, event->report_id);
	BUILD_ASSERT(sizeof(uint32_t) >= sizeof(event->provider_api));
	nrf_profiler_log_encode_uint32(buf, (uint32_t)event->provider_api);
	BUILD_ASSERT(sizeof(uint32_t) >= sizeof(event->hid_state_api));
	nrf_profiler_log_encode_uint32(buf, (uint32_t)event->hid_state_api);
}

APP_EVENT_INFO_DEFINE(hid_report_provider_event,
		      ENCODE(NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U32),
		      ENCODE("report_id", "provider_api", "hid_state_api"),
		      profile_hid_report_provider_event);

APP_EVENT_TYPE_DEFINE(hid_report_provider_event,
		      log_hid_report_provider_event,
		      &hid_report_provider_event_info,
		      APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_HID_REPORT_PROVIDER_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
