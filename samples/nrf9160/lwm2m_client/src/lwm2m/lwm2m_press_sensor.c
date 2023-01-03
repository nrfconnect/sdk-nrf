/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>

#include "env_sensor.h"
#include "lwm2m_app_utils.h"

#define MODULE app_lwm2m_press_sensor

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

#define MIN_RANGE_VALUE 30.0
#define MAX_RANGE_VALUE 110.0

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sensor_sim), okay)
#define PRESS_APP_TYPE "Simulated Pressure Sensor"
#else
#define PRESS_APP_TYPE "BME680 Pressure Sensor"
#endif

#define PRESS_UNIT "kPa"

static time_t timestamp;

static int update_timestamp_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			       uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	set_ipso_obj_timestamp(IPSO_OBJECT_PRESSURE_ID, obj_inst_id);
	return 0;
}

int lwm2m_init_press_sensor(void)
{
	double min_range_val = MIN_RANGE_VALUE;
	double max_range_val = MAX_RANGE_VALUE;

	env_sensor_init();

	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0));
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_UNITS_RID),
				  PRESS_UNIT, sizeof(PRESS_UNIT), sizeof(PRESS_UNIT),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, APPLICATION_TYPE_RID),
				  PRESS_APP_TYPE, sizeof(PRESS_APP_TYPE), sizeof(PRESS_APP_TYPE),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, MIN_RANGE_VALUE_RID),
			       &min_range_val);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, MAX_RANGE_VALUE_RID),
			       &max_range_val);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_PRESSURE_SENSOR_VERSION_1_1)) {
		lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
					 &timestamp, sizeof(timestamp),
					 sizeof(timestamp), LWM2M_RES_DATA_FLAG_RW);
		lwm2m_engine_register_post_write_callback(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0,
								     SENSOR_VALUE_RID),
							  update_timestamp_cb);
	}

	return 0;
}
