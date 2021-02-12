/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "data_event.h"


static void profile_data_event(struct log_event_buf *buf,
			       const struct event_header *eh)
{
	struct data_event *event = cast_data_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->val1);
	profiler_log_encode_u32(buf, event->val2);
	profiler_log_encode_u32(buf, event->val3);
	profiler_log_encode_u32(buf, event->val1u);
	profiler_log_encode_u32(buf, event->val2u);
	profiler_log_encode_u32(buf, event->val3u);
}

EVENT_INFO_DEFINE(data_event,
		  ENCODE(PROFILER_ARG_S8, PROFILER_ARG_S16, PROFILER_ARG_S32,
			 PROFILER_ARG_U8, PROFILER_ARG_U16, PROFILER_ARG_U32),
		  ENCODE("val1", "val2", "val3",
			 "val1u", "val2u", "val3u"),
		  profile_data_event);

EVENT_TYPE_DEFINE(data_event,
		  true,
		  NULL,
		  &data_event_info);
