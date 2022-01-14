/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>
#include <lwm2m_resource_ids.h>

#include "env_sensor.h"
#include "sensor_event.h"
#include "lwm2m_app_utils.h"

#define MODULE app_lwm2m_press_sensor

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

#define MIN_RANGE_VALUE 30.0
#define MAX_RANGE_VALUE 110.0

#if defined(CONFIG_ENV_SENSOR_USE_EXTERNAL)
#define PRESS_APP_TYPE "BME680 Pressure Sensor"
#elif defined(CONFIG_ENV_SENSOR_USE_SIM)
#define PRESS_APP_TYPE "Simulated Pressure Sensor"
#endif

#define PRESS_UNIT "kPa"

static double *press_float;
static int64_t sensor_read_timestamp;
static int32_t lwm2m_timestamp;
static uint8_t meas_qual_ind;

static void *pressure_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			      size_t *data_len)
{
	/* Only read sensor if a regular request from server, i.e. not a notify request */
	if (is_regular_read_cb(sensor_read_timestamp)) {
		int ret;
		struct sensor_value press_val;
		double new_press_float;

		ret = env_sensor_read_pressure(&press_val);
		if (ret) {
			LOG_ERR("Read air pressure sensor failed (%d)", ret);
			return NULL;
		}

		LOG_INF("Pressure: %d.%06d kPa", press_val.val1, press_val.val2);

		sensor_read_timestamp = k_uptime_get();

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_PRESSURE_SENSOR_VERSION_1_1)) {
			lwm2m_set_timestamp(IPSO_OBJECT_PRESSURE_ID, obj_inst_id);
		}

		new_press_float = sensor_value_to_double(&press_val);
		lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
				       &new_press_float);
	}

	*data_len = sizeof(*press_float);

	return press_float;
}

int lwm2m_init_press_sensor(void)
{
	double min_range_val = MIN_RANGE_VALUE;
	double max_range_val = MAX_RANGE_VALUE;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;

	sensor_read_timestamp = k_uptime_get();

	env_sensor_init();

	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0));
	lwm2m_engine_register_read_callback(
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID), pressure_read_cb);
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
				  (void **)&press_float, &dummy_data_len, &dummy_data_flags);
	lwm2m_engine_set_res_data(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_UNITS_RID),
				  PRESS_UNIT, sizeof(PRESS_UNIT), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_data(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, APPLICATION_TYPE_RID),
				  PRESS_APP_TYPE, sizeof(PRESS_APP_TYPE), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, MIN_RANGE_VALUE_RID),
			       &min_range_val);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, MAX_RANGE_VALUE_RID),
			       &max_range_val);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_PRESSURE_SENSOR_VERSION_1_1)) {
		meas_qual_ind = 0;

		lwm2m_engine_set_res_data(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
					  &lwm2m_timestamp, sizeof(lwm2m_timestamp),
					  LWM2M_RES_DATA_FLAG_RW);
		lwm2m_engine_set_res_data(
			LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, MEASUREMENT_QUALITY_INDICATOR_RID),
			&meas_qual_ind, sizeof(meas_qual_ind), LWM2M_RES_DATA_FLAG_RW);
	}

	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_sensor_event(eh)) {
		struct sensor_event *event = cast_sensor_event(eh);

		if (event->type == PRESSURE_SENSOR) {
			double received_value;

			sensor_read_timestamp = k_uptime_get();

			LOG_DBG("Pressure sensor event received: val1 = %06d, val2 = %06d",
				event->sensor_value.val1, event->sensor_value.val2);

			if (IS_ENABLED(CONFIG_LWM2M_IPSO_PRESSURE_SENSOR_VERSION_1_1)) {
				lwm2m_set_timestamp(IPSO_OBJECT_PRESSURE_ID, 0);
			}

			received_value = sensor_value_to_double(&event->sensor_value);
			lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0,
							  SENSOR_VALUE_RID),
					       &received_value);

			return true;
		}

		return false;
	}

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, sensor_event);
