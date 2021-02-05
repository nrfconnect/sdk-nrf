/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <stdio.h>

#include "cpu_load_event.h"


static int log_cpu_load_event(const struct event_header *eh, char *buf,
			      size_t buf_len)
{
	const struct cpu_load_event *event = cast_cpu_load_event(eh);

	return snprintf(buf, buf_len, "CPU load: %03u,%03u%%",
			event->load / 1000, event->load % 1000);
}

static void profile_cpu_load_event(struct log_event_buf *buf,
				   const struct event_header *eh)
{
	const struct cpu_load_event *event = cast_cpu_load_event(eh);

	profiler_log_encode_u32(buf, event->load);
}

EVENT_INFO_DEFINE(cpu_load_event,
		  ENCODE(PROFILER_ARG_U32),
		  ENCODE("load"),
		  profile_cpu_load_event);

EVENT_TYPE_DEFINE(cpu_load_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_CPU_LOAD_EVENT),
		  log_cpu_load_event,
		  &cpu_load_event_info);
