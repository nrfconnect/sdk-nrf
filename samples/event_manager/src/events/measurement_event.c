/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "measurement_event.h"

static int log_measurement_event(const struct event_header *eh, char *buf,
				 size_t buf_len)
{
	struct measurement_event *event = cast_measurement_event(eh);

	return snprintf(buf, buf_len, "val1=%d val2=%d val3=%d", event->value1,
			event->value2, event->value3);
}

static void profile_measurement_event(struct log_event_buf *buf,
				      const struct event_header *eh)
{
	struct measurement_event *event = cast_measurement_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->value1);
	profiler_log_encode_u32(buf, event->value2);
	profiler_log_encode_u32(buf, event->value3);
}

EVENT_INFO_DEFINE(measurement_event,
		  ENCODE(PROFILER_ARG_S8, PROFILER_ARG_S16, PROFILER_ARG_S32),
		  ENCODE("value1", "value2", "value3"),
		  profile_measurement_event);

EVENT_TYPE_DEFINE(measurement_event,
		  true,
		  log_measurement_event,
		  &measurement_event_info);
