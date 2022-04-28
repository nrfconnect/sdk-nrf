/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <stdio.h>

#include "cpu_load_event.h"


static void log_cpu_load_event(const struct app_event_header *aeh)
{
	const struct cpu_load_event *event = cast_cpu_load_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "CPU load: %03u,%03u%%",
			event->load / 1000, event->load % 1000);
}

static void profile_cpu_load_event(struct log_event_buf *buf,
				   const struct app_event_header *aeh)
{
	const struct cpu_load_event *event = cast_cpu_load_event(aeh);

	nrf_profiler_log_encode_uint32(buf, event->load);
}

APP_EVENT_INFO_DEFINE(cpu_load_event,
		  ENCODE(NRF_PROFILER_ARG_U32),
		  ENCODE("load"),
		  profile_cpu_load_event);

APP_EVENT_TYPE_DEFINE(cpu_load_event,
		  log_cpu_load_event,
		  &cpu_load_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_CPU_LOAD_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
