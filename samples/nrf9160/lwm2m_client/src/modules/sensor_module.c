/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/net/lwm2m.h>
#include <lwm2m_resource_ids.h>
#include <math.h>
#include <stdlib.h>

#include "lwm2m_app_utils.h"

#include "accelerometer.h"
#include "accel_event.h"

#include "env_sensor.h"
#include "light_sensor.h"
#include "sensor_event.h"

#define MODULE sensor_module

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

#define LIGHT_OBJ_INSTANCE_ID 0
#define COLOUR_OBJ_INSTANCE_ID 1

static struct k_work_delayable accel_work;
static struct k_work_delayable temp_work;
static struct k_work_delayable press_work;
static struct k_work_delayable humid_work;
static struct k_work_delayable gas_res_work;
static struct k_work_delayable light_work;
static struct k_work_delayable colour_work;

#define PERIOD K_MINUTES(2)
#define RETRY K_MSEC(200)
#define ACCEL_DELTA 1.0
#define TEMP_DELTA 1.0
#define PRESS_DELTA 0.5
#define HUMID_DELTA 1.0
#define GAS_RES_DELTA 5000

#define COLOUR_DELTA ((uint32_t)((50 << 24) | (50 << 16) | (50 << 8) | (50)))

static bool double_sufficient_change(double new_val, double old_val, double req_change)
{
	double change;

	change = fabs(new_val - old_val);
	if (change > req_change) {
		return true;
	}
	return false;
}

static void accel_work_cb(struct k_work *work)
{
	double old_x;
	double old_y;
	double old_z;
	struct accelerometer_sensor_data new_data;
	bool x, y, z;

	/* Get latest registered accelerometer values */
	lwm2m_engine_get_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, X_VALUE_RID), &old_x);
	lwm2m_engine_get_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Y_VALUE_RID), &old_y);
	lwm2m_engine_get_float(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Z_VALUE_RID), &old_z);

	accelerometer_read(&new_data);

	x = double_sufficient_change(sensor_value_to_double(&new_data.x), old_x, ACCEL_DELTA);
	y = double_sufficient_change(sensor_value_to_double(&new_data.y), old_y, ACCEL_DELTA);
	z = double_sufficient_change(sensor_value_to_double(&new_data.z), old_z, ACCEL_DELTA);

	if (x || y || z) {
		struct accel_event *event = new_accel_event();

		event->data = new_data;

		APP_EVENT_SUBMIT(event);
	}

	k_work_schedule(&accel_work, PERIOD);
}

static void sensor_worker(int (*read_cb)(struct sensor_value *), const char *path,
			  enum sensor_type type, double delta)
{
	double old;
	struct sensor_value new_data;
	int rc;

	rc = lwm2m_engine_get_float(path, &old);
	if (rc) {
		LOG_ERR("Failed to read previous sensor data %s", path);
		return;
	}

	rc = read_cb(&new_data);
	if (rc) {
		LOG_ERR("Failed to read sensor data");
		return;
	}

	if (double_sufficient_change(sensor_value_to_double(&new_data), old, delta)) {
		struct sensor_event *event = new_sensor_event();

		event->type = type;
		event->sensor_value = new_data;

		APP_EVENT_SUBMIT(event);
	}
}

static void temp_work_cb(struct k_work *work)
{
	sensor_worker(env_sensor_read_temperature,
		      LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
		      TEMPERATURE_SENSOR, TEMP_DELTA);

	k_work_schedule(&temp_work, PERIOD);
}

static void press_work_cb(struct k_work *work)
{
	sensor_worker(env_sensor_read_pressure,
		      LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID), PRESSURE_SENSOR,
		      PRESS_DELTA);

	k_work_schedule(&press_work, PERIOD);
}

static void humid_work_cb(struct k_work *work)
{
	sensor_worker(env_sensor_read_humidity,
		      LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
		      HUMIDITY_SENSOR, HUMID_DELTA);

	k_work_schedule(&humid_work, PERIOD);
}

static void gas_res_work_cb(struct k_work *work)
{
	sensor_worker(env_sensor_read_gas_resistance,
		      LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0, SENSOR_VALUE_RID),
		      GAS_RESISTANCE_SENSOR, GAS_RES_DELTA);

	k_work_schedule(&gas_res_work, PERIOD);
}

static bool rgbir_sufficient_change(uint32_t new_light_val, uint32_t old_light_val,
				    uint32_t req_change)
{
	uint8_t *new_val_ptr = (uint8_t *)(&new_light_val);
	uint8_t *old_val_ptr = (uint8_t *)(&old_light_val);
	int16_t new_byte, old_byte;
	uint32_t change = 0;

	/* Get change per colour channel; 1 byte per colour channel */
	for (int i = 0; i < 4; i++) {
		new_byte = *(new_val_ptr + i);
		old_byte = *(old_val_ptr + i);

		change |= (uint32_t)(fabs(new_byte - old_byte)) << 8 * i;
	}

	/* Check if any of the colour channels has changed sufficiently */
	for (int i = 0; i < 4; i++) {
		if ((uint8_t)(change >> 8 * i) > (uint8_t)(req_change >> 8 * i)) {
			return true;
		}
	}

	return false;
}

static int light_sensor_worker(int (*read_cb)(uint32_t *), const char *path, enum sensor_type type,
			       uint32_t delta)
{
	char *old_str;
	uint32_t old;
	uint32_t new;
	int ret;

	/* Get latest registered light value */
	ret = lwm2m_engine_get_res_buf(path, (void **)&old_str, NULL, NULL, NULL);
	if (ret < 0) {
		return ret;
	}

	old = strtol(old_str, NULL, 0);

	/* Read sensor, try again later if busy */
	if (read_cb(&new) == -EBUSY) {
		return -1;
	}

	if (rgbir_sufficient_change(new, old, delta)) {
		struct sensor_event *event = new_sensor_event();

		event->type = type;
		event->unsigned_value = new;

		APP_EVENT_SUBMIT(event);
	}
	return 0;
}

static void light_work_cb(struct k_work *work)
{
	int rc = light_sensor_worker(light_sensor_read,
				     LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
						COLOUR_RID),
				     LIGHT_SENSOR, COLOUR_DELTA);

	if (rc) {
		k_work_schedule(&light_work, RETRY);
		return;
	}

	k_work_schedule(&light_work, PERIOD);
}

static void colour_work_cb(struct k_work *work)
{
	int rc = light_sensor_worker(colour_sensor_read,
				     LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
						COLOUR_RID),
				     COLOUR_SENSOR, COLOUR_DELTA);

	if (rc) {
		k_work_schedule(&light_work, RETRY);
		return;
	}

	k_work_schedule(&colour_work, PERIOD);
}

int sensor_module_init(void)
{
	if (IS_ENABLED(CONFIG_SENSOR_MODULE_ACCEL)) {
		k_work_init_delayable(&accel_work, accel_work_cb);
		k_work_schedule(&accel_work, K_NO_WAIT);
	}

	if (IS_ENABLED(CONFIG_SENSOR_MODULE_TEMP)) {
		k_work_init_delayable(&temp_work, temp_work_cb);
		k_work_schedule(&temp_work, K_NO_WAIT);
	}

	if (IS_ENABLED(CONFIG_SENSOR_MODULE_PRESS)) {
		k_work_init_delayable(&press_work, press_work_cb);
		k_work_schedule(&press_work, K_NO_WAIT);
	}

	if (IS_ENABLED(CONFIG_SENSOR_MODULE_HUMID)) {
		k_work_init_delayable(&humid_work, humid_work_cb);
		k_work_schedule(&humid_work, K_NO_WAIT);
	}

	if (IS_ENABLED(CONFIG_SENSOR_MODULE_GAS_RES)) {
		k_work_init_delayable(&gas_res_work, gas_res_work_cb);
		k_work_schedule(&gas_res_work, K_NO_WAIT);
	}

	if (IS_ENABLED(CONFIG_SENSOR_MODULE_LIGHT)) {
		k_work_init_delayable(&light_work, light_work_cb);
		k_work_schedule(&light_work, K_NO_WAIT);
	}

	if (IS_ENABLED(CONFIG_SENSOR_MODULE_COLOUR)) {
		k_work_init_delayable(&colour_work, colour_work_cb);
		k_work_schedule(&colour_work, K_NO_WAIT);
	}

	return 0;
}
