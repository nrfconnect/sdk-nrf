/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>
#include <zephyr/devicetree.h>
#include "accelerometer.h"
#include "lwm2m_app_utils.h"
#include "lwm2m_engine.h"

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sensor_sim), okay)
#define ACCEL_APP_TYPE "Simulated Accelerometer"
#else
#define ACCEL_APP_TYPE "ADXL362 Accelerometer"
#endif

#define SENSOR_UNIT_NAME "m/s^2"

#if defined(CONFIG_ADXL362_ACCEL_RANGE_8G)
#define ACCEL_RANGE_G 8
#elif defined(CONFIG_ADXL362_ACCEL_RANGE_4G)
#define ACCEL_RANGE_G 4
#else
#define ACCEL_RANGE_G 2
#endif

#define MIN_RANGE_VALUE (-ACCEL_RANGE_G * SENSOR_G / 1000000.0)
#define MAX_RANGE_VALUE (ACCEL_RANGE_G * SENSOR_G / 1000000.0)

static time_t timestamp;

static int update_timestamp_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			       uint8_t *data, uint16_t data_len, bool last_block,
			       size_t total_size, size_t offset)
{
	set_ipso_obj_timestamp(IPSO_OBJECT_ACCELEROMETER_ID, obj_inst_id);
	return 0;
}

static int lwm2m_init_accel(void)
{
	double min_range_val = MIN_RANGE_VALUE;
	double max_range_val = MAX_RANGE_VALUE;

	accelerometer_init();

	lwm2m_create_object_inst(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID, 0));
	lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID, 0, SENSOR_UNITS_RID),
			  SENSOR_UNIT_NAME, sizeof(SENSOR_UNIT_NAME),
			  sizeof(SENSOR_UNIT_NAME),
			  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID, 0, MIN_RANGE_VALUE_RID),
		      min_range_val);
	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID, 0, MAX_RANGE_VALUE_RID),
		      max_range_val);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)) {
		lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID, 0,
					     APPLICATION_TYPE_RID),
				  ACCEL_APP_TYPE, sizeof(ACCEL_APP_TYPE),
					 sizeof(ACCEL_APP_TYPE), LWM2M_RES_DATA_FLAG_RO);
		lwm2m_set_res_buf(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID, 0, TIMESTAMP_RID),
				  &timestamp, sizeof(timestamp),
					 sizeof(timestamp), LWM2M_RES_DATA_FLAG_RW);
		lwm2m_register_post_write_callback(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID,
							      0, X_VALUE_RID),
						   update_timestamp_cb);
		lwm2m_register_post_write_callback(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID,
							      0, Y_VALUE_RID),
						   update_timestamp_cb);
		lwm2m_register_post_write_callback(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID,
							      0, Z_VALUE_RID),
						   update_timestamp_cb);
	}

	return 0;
}

LWM2M_APP_INIT(lwm2m_init_accel);
