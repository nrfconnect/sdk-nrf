/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "one_sec_event.h"

static void log_one_sec_event(const struct application_event_header *aeh)
{
	struct one_sec_event *event = cast_one_sec_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "timer: %" PRId8, event->five_sec_timer);
}

static void profile_one_sec_event(struct log_event_buf *buf,
			      const struct application_event_header *aeh)
{
	struct one_sec_event *event = cast_one_sec_event(aeh);

	profiler_log_encode_int8(buf, event->five_sec_timer);
}

EVENT_INFO_DEFINE(one_sec_event,
		  ENCODE(PROFILER_ARG_S8),
		  ENCODE("five_sec_timer"),
		  profile_one_sec_event);

APPLICATION_EVENT_TYPE_DEFINE(one_sec_event,
		  log_one_sec_event,
		  &one_sec_event_info,
		  APPLICATION_EVENT_FLAGS_CREATE(APPLICATION_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
