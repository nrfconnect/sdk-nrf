/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>
#include <math.h>
#include <zephyr/devicetree.h>

#include "accelerometer.h"
#include "accel_event.h"
#include "lwm2m_app_utils.h"

#define MODULE app_lwm2m_accel

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

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

int lwm2m_init_accel(void)
{
	double min_range_val = MIN_RANGE_VALUE;
	double max_range_val = MAX_RANGE_VALUE;

	accelerometer_init();

	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0));
	lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, SENSOR_UNITS_RID),
				 SENSOR_UNIT_NAME, sizeof(SENSOR_UNIT_NAME),
				 sizeof(SENSOR_UNIT_NAME),
				 LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, MIN_RANGE_VALUE_RID),
			       &min_range_val);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, MAX_RANGE_VALUE_RID),
			       &max_range_val);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)) {
		lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0,
						    APPLICATION_TYPE_RID),
					 ACCEL_APP_TYPE, sizeof(ACCEL_APP_TYPE),
					 sizeof(ACCEL_APP_TYPE), LWM2M_RES_DATA_FLAG_RO);
		lwm2m_engine_set_res_buf(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, TIMESTAMP_RID),
					 &timestamp, sizeof(timestamp),
					 sizeof(timestamp), LWM2M_RES_DATA_FLAG_RW);
	}

	return 0;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_accel_event(aeh)) {
		struct accel_event *event = cast_accel_event(aeh);
		double received_value;

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)) {
			set_ipso_obj_timestamp(IPSO_OBJECT_ACCELEROMETER_ID, 0);
		}

		LOG_DBG("Accelerometer sensor event received:"
			"x = %d.%06d, y = %d.%06d, z = %d.%06d",
			event->data.x.val1, event->data.x.val2, event->data.y.val1,
			event->data.y.val2, event->data.z.val1, event->data.z.val2);

		received_value = sensor_value_to_double(&event->data.x);
		lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, X_VALUE_RID),
				       &received_value);

		received_value = sensor_value_to_double(&event->data.y);
		lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Y_VALUE_RID),
				       &received_value);

		received_value = sensor_value_to_double(&event->data.z);
		lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Z_VALUE_RID),
				       &received_value);

		return true;
	}

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, accel_event);
