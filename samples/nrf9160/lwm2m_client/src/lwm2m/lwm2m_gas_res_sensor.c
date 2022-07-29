/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>

#include "env_sensor.h"
#include "sensor_event.h"
#include "lwm2m_app_utils.h"

#define MODULE app_lwm2m_gas_res_sensor

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

#define MIN_RANGE_VALUE 0.0
#define MAX_RANGE_VALUE 1000000.0

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sensor_sim), okay)
#define GENERIC_SENSOR_APP_TYPE "Simulated Gas Resistance Sensor"
#else
#define GENERIC_SENSOR_APP_TYPE "BME680 Gas Resistance Sensor"
#endif

#define GENERIC_SENSOR_TYPE "Gas resistance sensor"
#define GAS_RES_UNIT "Ω"

static double *gas_res_float;
static int64_t sensor_read_timestamp;
static int32_t lwm2m_timestamp;
static uint8_t meas_qual_ind;

static void *gas_resistance_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				    size_t *data_len)
{
	/* Only read sensor if a regular request from server, i.e. not a notify request */
	if (is_regular_read_cb(sensor_read_timestamp)) {
		int ret;
		struct sensor_value gas_res_val;
		double new_gas_res_float;

		ret = env_sensor_read_gas_resistance(&gas_res_val);
		if (ret) {
			LOG_ERR("Read gas resistance sensor failed (%d)", ret);
			return NULL;
		}

		LOG_INF("Gas resistance: %d.%06d Ω", gas_res_val.val1, gas_res_val.val2);

		sensor_read_timestamp = k_uptime_get();

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_GENERIC_SENSOR_VERSION_1_1)) {
			lwm2m_set_timestamp(IPSO_OBJECT_GENERIC_SENSOR_ID, 0);
		}

		new_gas_res_float = sensor_value_to_double(&gas_res_val);
		lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0,
						  SENSOR_VALUE_RID),
				       &new_gas_res_float);
	}

	*data_len = sizeof(*gas_res_float);

	return gas_res_float;
}

int lwm2m_init_gas_res_sensor(void)
{
	double min_range_val = MIN_RANGE_VALUE;
	double max_range_val = MAX_RANGE_VALUE;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;

	sensor_read_timestamp = k_uptime_get();

	env_sensor_init();

	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0));
	lwm2m_engine_register_read_callback(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0,
						       SENSOR_VALUE_RID),
					    gas_resistance_read_cb);
	lwm2m_engine_get_res_buf(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0, SENSOR_VALUE_RID),
				 (void **)&gas_res_float, NULL, &dummy_data_len, &dummy_data_flags);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0, SENSOR_UNITS_RID),
				 GAS_RES_UNIT, sizeof(GAS_RES_UNIT), sizeof(GAS_RES_UNIT),
				 LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0, APPLICATION_TYPE_RID),
				 GENERIC_SENSOR_APP_TYPE, sizeof(GENERIC_SENSOR_APP_TYPE),
				 sizeof(GENERIC_SENSOR_APP_TYPE), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0, SENSOR_TYPE_RID),
				 GENERIC_SENSOR_TYPE, sizeof(GENERIC_SENSOR_TYPE),
				 sizeof(GENERIC_SENSOR_TYPE), LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0, MIN_RANGE_VALUE_RID),
			       &min_range_val);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0, MAX_RANGE_VALUE_RID),
			       &max_range_val);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_GENERIC_SENSOR_VERSION_1_1)) {
		meas_qual_ind = 0;

		lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0,
						    TIMESTAMP_RID),
					 &lwm2m_timestamp, sizeof(lwm2m_timestamp),
					 sizeof(lwm2m_timestamp), LWM2M_RES_DATA_FLAG_RW);
		lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0,
						    MEASUREMENT_QUALITY_INDICATOR_RID),
					 &meas_qual_ind, sizeof(meas_qual_ind),
					 sizeof(meas_qual_ind), LWM2M_RES_DATA_FLAG_RW);
	}

	return 0;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_event(aeh)) {
		struct sensor_event *event = cast_sensor_event(aeh);

		if (event->type == GAS_RESISTANCE_SENSOR) {
			double received_value;

			sensor_read_timestamp = k_uptime_get();

			LOG_DBG("Gas resistance sensor event received: val1 = %06d, val2 = %06d",
				event->sensor_value.val1, event->sensor_value.val2);

			if (IS_ENABLED(CONFIG_LWM2M_IPSO_GENERIC_SENSOR_VERSION_1_1)) {
				lwm2m_set_timestamp(IPSO_OBJECT_GENERIC_SENSOR_ID, 0);
			}

			received_value = sensor_value_to_double(&event->sensor_value);
			lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0,
							  SENSOR_VALUE_RID),
					       &received_value);

			return true;
		}

		return false;
	}

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, sensor_event);
