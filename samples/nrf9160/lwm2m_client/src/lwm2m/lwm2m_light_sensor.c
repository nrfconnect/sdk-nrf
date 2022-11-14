/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>
#include <stdio.h>

#include "light_sensor.h"
#include "sensor_event.h"
#include "lwm2m_app_utils.h"

#define MODULE app_lwm2m_light_sensor

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

#define LIGHT_OBJ_INSTANCE_ID 0
#define COLOUR_OBJ_INSTANCE_ID 1

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sensor_sim), okay)
#define LIGHT_APP_TYPE "Simulated Light Sensor"
#define COLOUR_APP_TYPE "Simulated Colour Sensor"
#else
#define LIGHT_APP_TYPE "BH1749 Light Sensor"
#define COLOUR_APP_TYPE "BH1749 Colour Sensor"
#endif

#define LIGHT_SENSOR_APP_NAME "Light sensor"
#define COLOUR_SENSOR_APP_NAME "Colour sensor"
#define LIGHT_UNIT "RGB-IR"

static time_t timestamp[2];

int lwm2m_init_light_sensor(void)
{
	light_sensor_init();

	/* Ambient light sensor */
	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID));
	lwm2m_engine_set_string(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
					    APPLICATION_TYPE_RID),
				 LIGHT_APP_TYPE);
	lwm2m_engine_set_string(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
					     SENSOR_UNITS_RID),
				  LIGHT_UNIT);

	/* Surface colour sensor */
	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID));
	lwm2m_engine_set_string(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
					     APPLICATION_TYPE_RID),
				  COLOUR_APP_TYPE);
	lwm2m_engine_set_string(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
					     SENSOR_UNITS_RID),
				  LIGHT_UNIT);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1)) {
		/* Ambient light sensor */
		lwm2m_engine_set_res_buf(
			LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID, TIMESTAMP_RID),
			&timestamp[LIGHT_OBJ_INSTANCE_ID],
			sizeof(timestamp[LIGHT_OBJ_INSTANCE_ID]),
			sizeof(timestamp[LIGHT_OBJ_INSTANCE_ID]), LWM2M_RES_DATA_FLAG_RW);

		/* Surface colour sensor */
		lwm2m_engine_set_res_buf(
			LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID, TIMESTAMP_RID),
			&timestamp[COLOUR_OBJ_INSTANCE_ID],
			sizeof(timestamp[COLOUR_OBJ_INSTANCE_ID]),
			sizeof(timestamp[COLOUR_OBJ_INSTANCE_ID]),
			LWM2M_RES_DATA_FLAG_RW);
	}

	return 0;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_event(aeh)) {
		struct sensor_event *event = cast_sensor_event(aeh);
		char temp_value_str[RGBIR_STR_LENGTH];

		switch (event->type) {
		case LIGHT_SENSOR:
			snprintf(temp_value_str, RGBIR_STR_LENGTH, "0x%08X", event->unsigned_value);
			LOG_DBG("Light sensor event received! Val: 0x%08X", event->unsigned_value);

			if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1)) {
				set_ipso_obj_timestamp(IPSO_OBJECT_COLOUR_ID,
						       LIGHT_OBJ_INSTANCE_ID);
			}

			lwm2m_engine_set_string(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID,
							   LIGHT_OBJ_INSTANCE_ID, COLOUR_RID),
						temp_value_str);
			break;

		case COLOUR_SENSOR:
			snprintf(temp_value_str, RGBIR_STR_LENGTH, "0x%08X", event->unsigned_value);
			LOG_DBG("Colour sensor event received! Val: 0x%08X", event->unsigned_value);

			if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1)) {
				set_ipso_obj_timestamp(IPSO_OBJECT_COLOUR_ID,
						       COLOUR_OBJ_INSTANCE_ID);
			}

			lwm2m_engine_set_string(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID,
							   COLOUR_OBJ_INSTANCE_ID, COLOUR_RID),
						temp_value_str);
			break;

		default:
			return false;
		}

		return true;
	}

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, sensor_event);
