/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "sensor_sim_event.h"


static void log_sensor_sim_event(const struct application_event_header *aeh)
{
	const struct sensor_sim_event *event = cast_sensor_sim_event(aeh);

	APPLICATION_EVENT_MANAGER_LOG(aeh, "%s", event->label);
}

static void profile_sensor_sim_event(struct log_event_buf *buf,
				     const struct application_event_header *aeh)
{
}

EVENT_INFO_DEFINE(sensor_sim_event,
		  ENCODE(),
		  ENCODE(),
		  profile_sensor_sim_event);

APPLICATION_EVENT_TYPE_DEFINE(sensor_sim_event,
		  log_sensor_sim_event,
		  &sensor_sim_event_info,
		  APPLICATION_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_ML_APP_INIT_LOG_SENSOR_SIM_EVENTS,
				(APPLICATION_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
