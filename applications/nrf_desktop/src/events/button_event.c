/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "button_event.h"

static int log_button_event(const struct event_header *eh, char *buf,
			    size_t buf_len)
{
	const struct button_event *event = cast_button_event(eh);

	return snprintf(buf, buf_len, "key_id=0x%x %s", event->key_id,
			(event->pressed)?("pressed"):("released"));
}

static void profile_button_event(struct log_event_buf *buf,
				 const struct event_header *eh)
{
	const struct button_event *event = cast_button_event(eh);

	profiler_log_encode_u32(buf, event->key_id);
	profiler_log_encode_u32(buf, (event->pressed)?(1):(0));
}

EVENT_INFO_DEFINE(button_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U32),
		  ENCODE("button_id", "status"),
		  profile_button_event);

EVENT_TYPE_DEFINE(button_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BUTTON_EVENT),
		  log_button_event,
		  &button_event_info);
