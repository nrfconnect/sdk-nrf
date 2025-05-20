/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "selector_event.h"


static void log_selector_event(const struct app_event_header *aeh)
{
	const struct selector_event *event = cast_selector_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "id: %" PRIu8 " position: %" PRIu8,
			event->selector_id,
			event->position);
}

static void profile_selector_event(struct log_event_buf *buf,
				   const struct app_event_header *aeh)
{
	const struct selector_event *event = cast_selector_event(aeh);

	nrf_profiler_log_encode_uint8(buf, event->selector_id);
	nrf_profiler_log_encode_uint8(buf, event->position);
}

APP_EVENT_INFO_DEFINE(selector_event,
		  ENCODE(NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U8),
		  ENCODE("selector_id", "position"),
		  profile_selector_event);

APP_EVENT_TYPE_DEFINE(selector_event,
		  log_selector_event,
		  &selector_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_SELECTOR_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
