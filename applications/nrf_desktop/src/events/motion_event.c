/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "motion_event.h"

static int log_motion_event(const struct event_header *eh, char *buf,
				size_t buf_len)
{
	const struct motion_event *event = cast_motion_event(eh);

	return snprintf(buf, buf_len, "dx=%d, dy=%d", event->dx, event->dy);
}

static void profile_motion_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	const struct motion_event *event = cast_motion_event(eh);

	profiler_log_encode_u32(buf, event->dx);
	profiler_log_encode_u32(buf, event->dy);
}


EVENT_INFO_DEFINE(motion_event,
		  ENCODE(PROFILER_ARG_S32, PROFILER_ARG_S32),
		  ENCODE("dx", "dy"),
		  profile_motion_event);

EVENT_TYPE_DEFINE(motion_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_MOTION_EVENT),
		  log_motion_event,
		  &motion_event_info);
