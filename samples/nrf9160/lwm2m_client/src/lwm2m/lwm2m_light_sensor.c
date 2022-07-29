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

#define SENSOR_FETCH_DELAY_MS 200

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

static char light_value_str[RGBIR_STR_LENGTH] = "-";
static char colour_value_str[RGBIR_STR_LENGTH] = "-";
static int64_t sensor_read_timestamp[2];
static int32_t lwm2m_timestamp[2];
static uint8_t meas_qual_ind[2];

static void *light_sensor_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				  size_t *data_len)
{
	/* Only read sensor if a regular request from server, i.e. not a notify request */
	if (is_regular_read_cb(sensor_read_timestamp[obj_inst_id])) {
		int ret;
		uint32_t light_value;

		ret = light_sensor_read(&light_value);
		/* Fetch failed. Wait before trying fetch again. */
		if (ret == -EBUSY) {
			k_sleep(K_MSEC(SENSOR_FETCH_DELAY_MS));
			ret = light_sensor_read(&light_value);
			if (ret) {
				LOG_ERR("Read light sensor failed (%d)", ret);
				return NULL;
			}
		} else if (ret) {
			LOG_ERR("Read light sensor failed (%d)", ret);
			return NULL;
		}

		sensor_read_timestamp[obj_inst_id] = k_uptime_get();

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1)) {
			lwm2m_set_timestamp(IPSO_OBJECT_COLOUR_ID, obj_inst_id);
		}

		LOG_INF("Light value: 0x%08X", light_value);

		snprintf(light_value_str, RGBIR_STR_LENGTH, "0x%08X", light_value);
	}

	*data_len = sizeof(light_value_str);

	return &light_value_str;
}

static void *colour_sensor_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				   size_t *data_len)
{
	/* Only read sensor if a regular request from server, i.e. not a notify request */
	if (is_regular_read_cb(sensor_read_timestamp[obj_inst_id])) {
		int ret;
		uint32_t colour_value;

		ret = colour_sensor_read(&colour_value);
		/* Fetch failed. Wait before trying fetch again. */
		if (ret == -EBUSY) {
			k_sleep(K_MSEC(SENSOR_FETCH_DELAY_MS));
			ret = colour_sensor_read(&colour_value);
			if (ret) {
				LOG_ERR("Read colour sensor failed (%d)", ret);
				return NULL;
			}
		} else if (ret) {
			LOG_ERR("Read colour sensor failed (%d)", ret);
			return NULL;
		}

		sensor_read_timestamp[obj_inst_id] = k_uptime_get();

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1)) {
			lwm2m_set_timestamp(IPSO_OBJECT_COLOUR_ID, obj_inst_id);
		}

		LOG_INF("Colour value: 0x%08X", colour_value);

		snprintf(colour_value_str, RGBIR_STR_LENGTH, "0x%08X", colour_value);
	}

	*data_len = sizeof(colour_value_str);

	return &colour_value_str;
}

int lwm2m_init_light_sensor(void)
{
	sensor_read_timestamp[LIGHT_OBJ_INSTANCE_ID] = k_uptime_get();
	sensor_read_timestamp[COLOUR_OBJ_INSTANCE_ID] = k_uptime_get();

	light_sensor_init();

	/* Ambient light sensor */
	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID));
	lwm2m_engine_register_read_callback(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
						       COLOUR_RID),
					    light_sensor_read_cb);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
					     COLOUR_RID),
				  &light_value_str, RGBIR_STR_LENGTH, RGBIR_STR_LENGTH,
				  LWM2M_RES_DATA_FLAG_RW);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
					    APPLICATION_TYPE_RID),
				 LIGHT_APP_TYPE, sizeof(LIGHT_APP_TYPE), sizeof(LIGHT_APP_TYPE),
				 LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
					     SENSOR_UNITS_RID),
				  LIGHT_UNIT, sizeof(LIGHT_UNIT), sizeof(LIGHT_UNIT),
				  LWM2M_RES_DATA_FLAG_RO);

	/* Surface colour sensor */
	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID));
	lwm2m_engine_register_read_callback(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID,
						       COLOUR_OBJ_INSTANCE_ID, COLOUR_RID),
					    colour_sensor_read_cb);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
					     COLOUR_RID),
				  &colour_value_str, RGBIR_STR_LENGTH, RGBIR_STR_LENGTH,
				  LWM2M_RES_DATA_FLAG_RW);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
					     APPLICATION_TYPE_RID),
				  COLOUR_APP_TYPE, sizeof(COLOUR_APP_TYPE), sizeof(COLOUR_APP_TYPE),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
					     SENSOR_UNITS_RID),
				  LIGHT_UNIT, sizeof(LIGHT_UNIT), sizeof(LIGHT_UNIT),
				  LWM2M_RES_DATA_FLAG_RO);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1)) {
		/* Ambient light sensor */
		meas_qual_ind[LIGHT_OBJ_INSTANCE_ID] = 0;
		lwm2m_engine_set_res_buf(
			LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID, TIMESTAMP_RID),
			&lwm2m_timestamp[LIGHT_OBJ_INSTANCE_ID],
			sizeof(lwm2m_timestamp[LIGHT_OBJ_INSTANCE_ID]),
			sizeof(lwm2m_timestamp[LIGHT_OBJ_INSTANCE_ID]), LWM2M_RES_DATA_FLAG_RW);
		lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
						     MEASUREMENT_QUALITY_INDICATOR_RID),
					  &meas_qual_ind[LIGHT_OBJ_INSTANCE_ID],
					  sizeof(meas_qual_ind[LIGHT_OBJ_INSTANCE_ID]),
					  sizeof(meas_qual_ind[LIGHT_OBJ_INSTANCE_ID]),
					  LWM2M_RES_DATA_FLAG_RW);

		/* Surface colour sensor */
		meas_qual_ind[COLOUR_OBJ_INSTANCE_ID] = 0;
		lwm2m_engine_set_res_buf(
			LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID, TIMESTAMP_RID),
			&lwm2m_timestamp[COLOUR_OBJ_INSTANCE_ID],
			sizeof(lwm2m_timestamp[COLOUR_OBJ_INSTANCE_ID]),
			sizeof(lwm2m_timestamp[COLOUR_OBJ_INSTANCE_ID]),
			LWM2M_RES_DATA_FLAG_RW);
		lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
						     MEASUREMENT_QUALITY_INDICATOR_RID),
					  &meas_qual_ind[COLOUR_OBJ_INSTANCE_ID],
					  sizeof(meas_qual_ind[COLOUR_OBJ_INSTANCE_ID]),
					  sizeof(meas_qual_ind[COLOUR_OBJ_INSTANCE_ID]),
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

			sensor_read_timestamp[LIGHT_OBJ_INSTANCE_ID] = k_uptime_get();

			if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1)) {
				lwm2m_set_timestamp(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID);
			}

			lwm2m_engine_set_string(LWM2M_PATH(IPSO_OBJECT_COLOUR_ID,
							   LIGHT_OBJ_INSTANCE_ID, COLOUR_RID),
						temp_value_str);
			break;

		case COLOUR_SENSOR:
			snprintf(temp_value_str, RGBIR_STR_LENGTH, "0x%08X", event->unsigned_value);
			LOG_DBG("Colour sensor event received! Val: 0x%08X", event->unsigned_value);

			sensor_read_timestamp[COLOUR_OBJ_INSTANCE_ID] = k_uptime_get();

			if (IS_ENABLED(CONFIG_LWM2M_IPSO_APP_COLOUR_SENSOR_VERSION_1_1)) {
				lwm2m_set_timestamp(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID);
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
