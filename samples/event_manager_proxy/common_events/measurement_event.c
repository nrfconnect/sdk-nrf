/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "measurement_event.h"


static void log_measurement_event(const struct app_event_header *aeh)
{
	struct measurement_event *event = cast_measurement_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "val1=%d val2=%d val3=%d", event->value1,
			event->value2, event->value3);
}

static void profile_measurement_event(struct log_event_buf *buf,
				      const struct app_event_header *aeh)
{
	struct measurement_event *event = cast_measurement_event(aeh);

	ARG_UNUSED(event);
	nrf_profiler_log_encode_int8(buf, event->value1);
	nrf_profiler_log_encode_int16(buf, event->value2);
	nrf_profiler_log_encode_int32(buf, event->value3);
}

APP_EVENT_INFO_DEFINE(measurement_event,
		  ENCODE(NRF_PROFILER_ARG_S8, NRF_PROFILER_ARG_S16, NRF_PROFILER_ARG_S32),
		  ENCODE("value1", "value2", "value3"),
		  profile_measurement_event);

APP_EVENT_TYPE_DEFINE(measurement_event,
		  log_measurement_event,
		  &measurement_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
