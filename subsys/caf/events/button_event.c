/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <caf/events/button_event.h>

static void log_button_event(const struct event_header *eh)
{
	const struct button_event *event = cast_button_event(eh);

	EVENT_MANAGER_LOG(eh, "key_id=0x%x %s", event->key_id,
			(event->pressed)?("pressed"):("released"));
}

static void profile_button_event(struct log_event_buf *buf,
				 const struct event_header *eh)
{
	const struct button_event *event = cast_button_event(eh);

	profiler_log_encode_uint16(buf, event->key_id);
	profiler_log_encode_uint8(buf, (event->pressed)?(1):(0));
}

EVENT_INFO_DEFINE(button_event,
		  ENCODE(PROFILER_ARG_U16, PROFILER_ARG_U8),
		  ENCODE("button_id", "status"),
		  profile_button_event);

EVENT_TYPE_DEFINE(button_event,
		  log_button_event,
		  &button_event_info,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_BUTTON_EVENTS,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
