/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "sensor_sim_event.h"


static int log_sensor_sim_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	const struct sensor_sim_event *event = cast_sensor_sim_event(eh);

	EVENT_MANAGER_LOG(eh, "%s", event->label);
	return 0;
}

static void profile_sensor_sim_event(struct log_event_buf *buf, const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(sensor_sim_event,
		  ENCODE(),
		  ENCODE(),
		  profile_sensor_sim_event);

EVENT_TYPE_DEFINE(sensor_sim_event,
		  IS_ENABLED(CONFIG_ML_APP_INIT_LOG_SENSOR_SIM_EVENTS),
		  log_sensor_sim_event,
		  &sensor_sim_event_info);
