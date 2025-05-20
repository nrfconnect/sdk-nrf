/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <caf/events/click_event.h>


static const char * const click_name[] = {
	[CLICK_NONE] = "NONE",
	[CLICK_SHORT] = "SHORT",
	[CLICK_LONG] = "LONG",
	[CLICK_DOUBLE] = "DOUBLE",
};

static void log_click_event(const struct app_event_header *aeh)
{
	const struct click_event *event = cast_click_event(aeh);

	__ASSERT_NO_MSG(event->click < CLICK_COUNT);
	__ASSERT_NO_MSG(click_name[event->click]);
	APP_EVENT_MANAGER_LOG(aeh, "key_id: %" PRIu16 " click: %s",
			event->key_id,
			click_name[event->click]);
}

static void profile_click_event(struct log_event_buf *buf,
				const struct app_event_header *aeh)
{
	const struct click_event *event = cast_click_event(aeh);

	nrf_profiler_log_encode_uint16(buf, event->key_id);
	nrf_profiler_log_encode_uint8(buf, event->click);
}

APP_EVENT_INFO_DEFINE(click_event,
		  ENCODE(NRF_PROFILER_ARG_U16, NRF_PROFILER_ARG_U8),
		  ENCODE("key_id", "click"),
		  profile_click_event);

APP_EVENT_TYPE_DEFINE(click_event,
		  log_click_event,
		  &click_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_CLICK_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
