/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "click_event.h"

static const char * const click_name[] = {
#define X(name) STRINGIFY(name),
	CLICK_LIST
#undef X
};

static int log_click_event(const struct event_header *eh, char *buf,
			   size_t buf_len)
{
	const struct click_event *event = cast_click_event(eh);

	__ASSERT_NO_MSG(event->click < CLICK_COUNT);
	return snprintf(buf, buf_len, "key_id: %" PRIu16 " click: %s",
			event->key_id,
			click_name[event->click]);
}

static void profile_click_event(struct log_event_buf *buf,
				const struct event_header *eh)
{
	const struct click_event *event = cast_click_event(eh);

	profiler_log_encode_u32(buf, event->key_id);
	profiler_log_encode_u32(buf, event->click);
}

EVENT_INFO_DEFINE(click_event,
		  ENCODE(PROFILER_ARG_U16, PROFILER_ARG_U8),
		  ENCODE("key_id", "click"),
		  profile_click_event);

EVENT_TYPE_DEFINE(click_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_CLICK_EVENT),
		  log_click_event,
		  &click_event_info);
