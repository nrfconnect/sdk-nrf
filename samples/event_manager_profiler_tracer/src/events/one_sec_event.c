/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "one_sec_event.h"

static int log_one_sec_event(const struct event_header *eh, char *buf,
				 size_t buf_len)
{
	struct one_sec_event *event = cast_one_sec_event(eh);

	EVENT_MANAGER_LOG(eh, "timer: %" PRId8, event->five_sec_timer);
	return 0;
}

static void profile_one_sec_event(struct log_event_buf *buf,
			      const struct event_header *eh)
{
	struct one_sec_event *event = cast_one_sec_event(eh);

	profiler_log_encode_int8(buf, event->five_sec_timer);
}

EVENT_INFO_DEFINE(one_sec_event,
		  ENCODE(PROFILER_ARG_S8),
		  ENCODE("five_sec_timer"),
		  profile_one_sec_event);

EVENT_TYPE_DEFINE(one_sec_event,
		  true,
		  log_one_sec_event,
		  &one_sec_event_info);
