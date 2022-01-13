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

static int log_click_event(const struct event_header *eh, char *buf,
			   size_t buf_len)
{
	const struct click_event *event = cast_click_event(eh);

	__ASSERT_NO_MSG(event->click < CLICK_COUNT);
	__ASSERT_NO_MSG(click_name[event->click]);
	EVENT_MANAGER_LOG(eh, "key_id: %" PRIu16 " click: %s",
			event->key_id,
			click_name[event->click]);
	return 0;
}

static void profile_click_event(struct log_event_buf *buf,
				const struct event_header *eh)
{
	const struct click_event *event = cast_click_event(eh);

	profiler_log_encode_uint16(buf, event->key_id);
	profiler_log_encode_uint8(buf, event->click);
}

EVENT_INFO_DEFINE(click_event,
		  ENCODE(PROFILER_ARG_U16, PROFILER_ARG_U8),
		  ENCODE("key_id", "click"),
		  profile_click_event);

EVENT_TYPE_DEFINE(click_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_CLICK_EVENTS),
		  log_click_event,
		  &click_event_info);
