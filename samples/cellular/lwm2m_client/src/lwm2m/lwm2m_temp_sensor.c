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

#define MIN_RANGE_VALUE -40.0
#define MAX_RANGE_VALUE 85.0

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sensor_sim), okay)
#define TEMP_APP_TYPE "Simulated Temperature Sensor"
#else
#define TEMP_APP_TYPE "BME680 Temperature Sensor"
#endif

#define TEMP_UNIT "°C"

static time_t timestamp;

static int update_timestamp_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			       uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	set_ipso_obj_timestamp(IPSO_OBJECT_TEMP_SENSOR_ID, obj_inst_id);
	return 0;
}

int lwm2m_init_temp_sensor(void)
{
	double min_range_val = MIN_RANGE_VALUE;
	double max_range_val = MAX_RANGE_VALUE;

	env_sensor_init();

	lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_UNITS_RID),
			  TEMP_UNIT, sizeof(TEMP_UNIT), sizeof(TEMP_UNIT),
			  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MIN_RANGE_VALUE_RID),
		      min_range_val);
	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MAX_RANGE_VALUE_RID),
		      max_range_val);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_TEMP_SENSOR_VERSION_1_1)) {
		lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0,
					     APPLICATION_TYPE_RID),
				  TEMP_APP_TYPE, sizeof(TEMP_APP_TYPE),
				  sizeof(TEMP_APP_TYPE), LWM2M_RES_DATA_FLAG_RO);
		lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
				  &timestamp, sizeof(timestamp),
				  sizeof(timestamp),
				  LWM2M_RES_DATA_FLAG_RW);
		lwm2m_register_post_write_callback(&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0,
							      SENSOR_VALUE_RID),
						   update_timestamp_cb);
	}

	return 0;
}
