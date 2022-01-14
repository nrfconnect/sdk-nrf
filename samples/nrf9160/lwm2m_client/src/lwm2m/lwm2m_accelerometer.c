/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <net/lwm2m.h>
#include <lwm2m_resource_ids.h>
#include <math.h>

#include "accelerometer.h"
#include "accel_event.h"
#include "lwm2m_app_utils.h"

#define MODULE app_lwm2m_accel

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

#if defined(CONFIG_ACCEL_USE_EXTERNAL)
#define ACCEL_APP_TYPE "ADXL362 Accelerometer"
#elif defined(CONFIG_ACCEL_USE_SIM)
#define ACCEL_APP_TYPE "Simulated Accelerometer"
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

static double *x_val;
static double *y_val;
static double *z_val;
static int64_t accel_read_timestamp[3];
int32_t lwm2m_timestamp;
static uint8_t meas_qual_ind;

static int get_res_timestamp_index(uint16_t res_id)
{
	switch (res_id) {
	case X_VALUE_RID:
		return 0;
	case Y_VALUE_RID:
		return 1;
	case Z_VALUE_RID:
		return 2;
	default:
		LOG_ERR("Resource ID not supported (%d)", -ENOTSUP);
		return -ENOTSUP;
	}
}

static void *accel_x_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			     size_t *data_len)
{
	uint8_t res_timestamp_index = get_res_timestamp_index(res_id);

	if (is_regular_read_cb(accel_read_timestamp[res_timestamp_index])) {
		int ret;
		struct accelerometer_sensor_data accel_data;
		double new_x_val;

		ret = accelerometer_read(&accel_data);
		if (ret) {
			LOG_ERR("Read accelerometer failed (%d)", ret);
			return NULL;
		}

		LOG_INF("Acceleration x-direction: %d.%06d", accel_data.x.val1, accel_data.x.val2);

		accel_read_timestamp[0] = k_uptime_get();

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)) {
			lwm2m_set_timestamp(IPSO_OBJECT_ACCELEROMETER_ID, obj_inst_id);
		}

		new_x_val = sensor_value_to_double(&accel_data.x);
		lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, X_VALUE_RID),
				       &new_x_val);
	}

	*data_len = sizeof(*x_val);

	return x_val;
}

static void *accel_y_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			     size_t *data_len)
{
	uint8_t res_timestamp_index = get_res_timestamp_index(res_id);

	if (is_regular_read_cb(accel_read_timestamp[res_timestamp_index])) {
		int ret;
		struct accelerometer_sensor_data accel_data;
		double new_y_val;

		ret = accelerometer_read(&accel_data);
		if (ret) {
			LOG_ERR("Read accelerometer failed (%d)", ret);
			return NULL;
		}

		LOG_INF("Acceleration y-direction: %d.%06d", accel_data.y.val1, accel_data.y.val2);

		accel_read_timestamp[1] = k_uptime_get();

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)) {
			lwm2m_set_timestamp(IPSO_OBJECT_ACCELEROMETER_ID, obj_inst_id);
		}

		new_y_val = sensor_value_to_double(&accel_data.y);
		lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Y_VALUE_RID),
				       &new_y_val);
	}

	*data_len = sizeof(*y_val);

	return y_val;
}

static void *accel_z_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			     size_t *data_len)
{
	uint8_t res_timestamp_index = get_res_timestamp_index(res_id);

	if (is_regular_read_cb(accel_read_timestamp[res_timestamp_index])) {
		int ret;
		struct accelerometer_sensor_data accel_data;
		double new_z_val;

		ret = accelerometer_read(&accel_data);
		if (ret) {
			LOG_ERR("Read accelerometer failed (%d)", ret);
			return NULL;
		}

		LOG_INF("Acceleration z-direction: %d.%06d", accel_data.z.val1, accel_data.z.val2);

		accel_read_timestamp[2] = k_uptime_get();

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)) {
			lwm2m_set_timestamp(IPSO_OBJECT_ACCELEROMETER_ID, obj_inst_id);
		}

		new_z_val = sensor_value_to_double(&accel_data.z);
		lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Z_VALUE_RID),
				       &new_z_val);
	}

	*data_len = sizeof(*z_val);

	return z_val;
}

int lwm2m_init_accel(void)
{
	double min_range_val = MIN_RANGE_VALUE;
	double max_range_val = MAX_RANGE_VALUE;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;

	accel_read_timestamp[0] = k_uptime_get();
	accel_read_timestamp[1] = k_uptime_get();
	accel_read_timestamp[2] = k_uptime_get();

	accelerometer_init();

	lwm2m_engine_create_obj_inst(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0));
	lwm2m_engine_set_res_data(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, SENSOR_UNITS_RID),
				  SENSOR_UNIT_NAME, sizeof(SENSOR_UNIT_NAME),
				  LWM2M_RES_DATA_FLAG_RO);
	lwm2m_engine_register_read_callback(
		LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, X_VALUE_RID), accel_x_read_cb);
	lwm2m_engine_register_read_callback(
		LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Y_VALUE_RID), accel_y_read_cb);
	lwm2m_engine_register_read_callback(
		LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Z_VALUE_RID), accel_z_read_cb);
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, X_VALUE_RID),
				  (void **)&x_val, &dummy_data_len, &dummy_data_flags);
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Y_VALUE_RID),
				  (void **)&y_val, &dummy_data_len, &dummy_data_flags);
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Z_VALUE_RID),
				  (void **)&z_val, &dummy_data_len, &dummy_data_flags);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, MIN_RANGE_VALUE_RID),
			       &min_range_val);
	lwm2m_engine_set_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, MAX_RANGE_VALUE_RID),
			       &max_range_val);

	if (IS_ENABLED(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)) {
		meas_qual_ind = 0;

		lwm2m_engine_set_res_data(
			LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, APPLICATION_TYPE_RID),
			ACCEL_APP_TYPE, sizeof(ACCEL_APP_TYPE), LWM2M_RES_DATA_FLAG_RO);
		lwm2m_engine_set_res_data(
			LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, TIMESTAMP_RID),
			&lwm2m_timestamp, sizeof(lwm2m_timestamp), LWM2M_RES_DATA_FLAG_RW);
		lwm2m_engine_set_res_data(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0,
						     MEASUREMENT_QUALITY_INDICATOR_RID),
					  &meas_qual_ind, sizeof(meas_qual_ind),
					  LWM2M_RES_DATA_FLAG_RW);
	}

	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_accel_event(eh)) {
		struct accel_event *event = cast_accel_event(eh);
		double received_value;

		accel_read_timestamp[0] = k_uptime_get();
		accel_read_timestamp[1] = k_uptime_get();
		accel_read_timestamp[2] = k_uptime_get();

		if (IS_ENABLED(CONFIG_LWM2M_IPSO_ACCELEROMETER_VERSION_1_1)) {
			lwm2m_set_timestamp(IPSO_OBJECT_ACCELEROMETER_ID, 0);
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

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, accel_event);
