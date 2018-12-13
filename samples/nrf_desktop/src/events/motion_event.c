/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "motion_event.h"

static void print_event(const struct event_header *eh)
{
	struct motion_event *event = cast_motion_event(eh);

	printk("dx=%d, dy=%d", event->dx, event->dy);
}

static void log_event(struct log_event_buf *buf,
			     const struct event_header *eh)
{
	struct motion_event *event = cast_motion_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->dx);
	profiler_log_encode_u32(buf, event->dy);
}


EVENT_INFO_DEFINE(motion_event, ENCODE(PROFILER_ARG_S32, PROFILER_ARG_S32),
			ENCODE("dx", "dy"), log_event);
EVENT_TYPE_DEFINE(motion_event, print_event, &motion_event_info);
