/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrf_modem_gnss.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/lwm2m_path.h>

#include "gnss_pvt_event.h"
#include "lwm2m_app_utils.h"

#define MODULE lwm2m_app_loc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_gnss_pvt_event(aeh)) {
		struct gnss_pvt_event *event = cast_gnss_pvt_event(aeh);

		double latitude = event->pvt.latitude;
		double longitude = event->pvt.longitude;
		double altitude = event->pvt.altitude;
		double speed = event->pvt.speed;
		double radius = event->pvt.accuracy;

		lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LATITUDE_RID),
				       &latitude);
		lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LONGITUDE_RID),
				       &longitude);
		lwm2m_engine_set_float(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, ALTITUDE_RID),
				       &altitude);
		lwm2m_engine_set_float(
			LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_RADIUS_RID), &radius);
		lwm2m_engine_set_float(
			LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LOCATION_SPEED_RID), &speed);

		return true;
	}

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, gnss_pvt_event);
