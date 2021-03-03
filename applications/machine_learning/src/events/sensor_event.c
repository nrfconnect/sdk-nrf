/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "sensor_event.h"


static int log_sensor_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	const struct sensor_event *event = cast_sensor_event(eh);

	return snprintf(buf, buf_len, "%s", event->descr);
}

static void profile_sensor_event(struct log_event_buf *buf, const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(sensor_event,
		  ENCODE(),
		  ENCODE(),
		  profile_sensor_event);

EVENT_TYPE_DEFINE(sensor_event,
		  IS_ENABLED(CONFIG_ML_APP_INIT_LOG_SENSOR_EVENT),
		  log_sensor_event,
		  &sensor_event_info);
