/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "motion_event.h"

static void log_motion_event(const struct event_header *eh)
{
	const struct motion_event *event = cast_motion_event(eh);

	EVENT_MANAGER_LOG(eh, "dx=%d, dy=%d", event->dx, event->dy);
}

static void profile_motion_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	const struct motion_event *event = cast_motion_event(eh);

	profiler_log_encode_int16(buf, event->dx);
	profiler_log_encode_int16(buf, event->dy);
}

EVENT_INFO_DEFINE(motion_event,
		  ENCODE(PROFILER_ARG_S16, PROFILER_ARG_S16),
		  ENCODE("dx", "dy"),
		  profile_motion_event);

EVENT_TYPE_DEFINE(motion_event,
		  log_motion_event,
		  &motion_event_info,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_MOTION_EVENT,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
