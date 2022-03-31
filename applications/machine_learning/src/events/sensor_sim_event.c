/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "sensor_sim_event.h"


static void log_sensor_sim_event(const struct app_event_header *aeh)
{
	const struct sensor_sim_event *event = cast_sensor_sim_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "%s", event->label);
}

static void profile_sensor_sim_event(struct log_event_buf *buf,
				     const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(sensor_sim_event,
		  ENCODE(),
		  ENCODE(),
		  profile_sensor_sim_event);

APP_EVENT_TYPE_DEFINE(sensor_sim_event,
		  log_sensor_sim_event,
		  &sensor_sim_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_ML_APP_INIT_LOG_SENSOR_SIM_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
