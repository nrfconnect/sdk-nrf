/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/sensor.h>
#include <net/lwm2m.h>
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

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_LOG_LEVEL);

#define LIGHT_OBJ_INSTANCE_ID 0
#define COLOUR_OBJ_INSTANCE_ID 1

#if defined(CONFIG_SENSOR_MODULE_ACCEL)
#define ACCEL_STARTUP_DELAY K_SECONDS(CONFIG_SENSOR_MODULE_ACCEL_STARTUP_DELAY)
#define ACCEL_PERIOD CONFIG_SENSOR_MODULE_ACCEL_PERIOD
#define ACCEL_DELTA_X                                                   \
	((double)CONFIG_SENSOR_MODULE_ACCEL_X_DELTA_INT + \
	 (double)CONFIG_SENSOR_MODULE_ACCEL_X_DELTA_DEC / 1000000.0)
#define ACCEL_DELTA_Y                                                   \
	((double)CONFIG_SENSOR_MODULE_ACCEL_Y_DELTA_INT + \
	 (double)CONFIG_SENSOR_MODULE_ACCEL_Y_DELTA_DEC / 1000000.0)
#define ACCEL_DELTA_Z                                                   \
	((double)CONFIG_SENSOR_MODULE_ACCEL_Z_DELTA_INT + \
	 (double)CONFIG_SENSOR_MODULE_ACCEL_Z_DELTA_DEC / 1000000.0)

static struct k_work_delayable accel_work;
#endif /* defined(CONFIG_SENSOR_MODULE_ACCEL) */

#if defined(CONFIG_SENSOR_MODULE_TEMP)
#define TEMP_STARTUP_DELAY K_SECONDS(CONFIG_SENSOR_MODULE_TEMP_STARTUP_DELAY)
#define TEMP_PERIOD CONFIG_SENSOR_MODULE_TEMP_PERIOD
#define TEMP_DELTA                                                   \
	((double)CONFIG_SENSOR_MODULE_TEMP_DELTA_INT + \
	 (double)CONFIG_SENSOR_MODULE_TEMP_DELTA_DEC / 1000000.0)

static struct k_work_delayable temp_work;
#endif /* defined(CONFIG_SENSOR_MODULE_TEMP) */

#if defined(CONFIG_SENSOR_MODULE_PRESS)
#define PRESS_STARTUP_DELAY K_SECONDS(CONFIG_SENSOR_MODULE_PRESS_STARTUP_DELAY)
#define PRESS_PERIOD CONFIG_SENSOR_MODULE_PRESS_PERIOD
#define PRESS_DELTA                                                   \
	((double)CONFIG_SENSOR_MODULE_PRESS_DELTA_INT + \
	 (double)CONFIG_SENSOR_MODULE_PRESS_DELTA_DEC / 1000000.0)

static struct k_work_delayable press_work;
#endif /* defined(CONFIG_SENSOR_MODULE_PRESS) */

#if defined(CONFIG_SENSOR_MODULE_HUMID)
#define HUMID_STARTUP_DELAY K_SECONDS(CONFIG_SENSOR_MODULE_HUMID_STARTUP_DELAY)
#define HUMID_PERIOD CONFIG_SENSOR_MODULE_HUMID_PERIOD
#define HUMID_DELTA                                                   \
	((double)CONFIG_SENSOR_MODULE_HUMID_DELTA_INT + \
	 (double)CONFIG_SENSOR_MODULE_HUMID_DELTA_DEC / 1000000.0)

static struct k_work_delayable humid_work;
#endif /* defined(CONFIG_SENSOR_MODULE_HUMID) */

#if defined(CONFIG_SENSOR_MODULE_GAS_RES)
#define GAS_RES_STARTUP_DELAY K_SECONDS(CONFIG_SENSOR_MODULE_GAS_RES_STARTUP_DELAY)
#define GAS_RES_PERIOD CONFIG_SENSOR_MODULE_GAS_RES_PERIOD
#define GAS_RES_DELTA ((double)CONFIG_SENSOR_MODULE_GAS_RES_DELTA)

static struct k_work_delayable gas_res_work;
#endif /* defined(CONFIG_SENSOR_MODULE_GAS_RES) */

#if defined(CONFIG_SENSOR_MODULE_LIGHT)
#define LIGHT_STARTUP_DELAY K_SECONDS(CONFIG_SENSOR_MODULE_LIGHT_STARTUP_DELAY)
#define LIGHT_PERIOD CONFIG_SENSOR_MODULE_LIGHT_PERIOD
#define LIGHT_FETCH_DELAY_MS (LIGHT_PERIOD * MSEC_PER_SEC / 2)
#define LIGHT_DELTA                                          \
	((uint32_t)((CONFIG_SENSOR_MODULE_LIGHT_DELTA_R << 24) | \
		    (CONFIG_SENSOR_MODULE_LIGHT_DELTA_G << 16) |     \
		    (CONFIG_SENSOR_MODULE_LIGHT_DELTA_B << 8) |      \
		    (CONFIG_SENSOR_MODULE_LIGHT_DELTA_IR)))

static struct k_work_delayable light_work;
#endif /* defined(CONFIG_SENSOR_MODULE_LIGHT) */

#if defined(CONFIG_SENSOR_MODULE_COLOUR)
#define COLOUR_STARTUP_DELAY K_SECONDS(CONFIG_SENSOR_MODULE_COLOUR_STARTUP_DELAY)
#define COLOUR_PERIOD CONFIG_SENSOR_MODULE_COLOUR_PERIOD
#define COLOUR_FETCH_DELAY_MS (COLOUR_PERIOD * MSEC_PER_SEC / 2)
#define COLOUR_DELTA                                          \
	((uint32_t)((CONFIG_SENSOR_MODULE_COLOUR_DELTA_R << 24) | \
		    (CONFIG_SENSOR_MODULE_COLOUR_DELTA_G << 16) |     \
		    (CONFIG_SENSOR_MODULE_COLOUR_DELTA_B << 8) |      \
		    (CONFIG_SENSOR_MODULE_COLOUR_DELTA_IR)))

static struct k_work_delayable colour_work;
#endif /* defined(CONFIG_SENSOR_MODULE_COLOUR) */

static bool double_sufficient_change(double new_val, double old_val,
				     double req_change)
{
	double change;

	change = fabs(new_val - old_val);
	if (change > req_change) {
		return true;
	}
	return false;
}

#if defined(CONFIG_SENSOR_MODULE_ACCEL)
static void accel_work_cb(struct k_work *work)
{
	double *old_x_val;
	double *old_y_val;
	double *old_z_val;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;
	struct accelerometer_sensor_data new_data;
	bool sufficient_x, sufficient_y, sufficient_z;

	/* Get latest registered accelerometer values */
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, X_VALUE_RID),
				  (void **)(&old_x_val), &dummy_data_len, &dummy_data_flags);
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Y_VALUE_RID),
				  (void **)(&old_y_val), &dummy_data_len, &dummy_data_flags);
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_ACCELEROMETER_ID, 0, Z_VALUE_RID),
				  (void **)(&old_z_val), &dummy_data_len, &dummy_data_flags);

	accelerometer_read(&new_data);

	sufficient_x = double_sufficient_change(sensor_value_to_double(&new_data.x), *old_x_val,
						ACCEL_DELTA_X);
	sufficient_y = double_sufficient_change(sensor_value_to_double(&new_data.y), *old_y_val,
						ACCEL_DELTA_Y);
	sufficient_z = double_sufficient_change(sensor_value_to_double(&new_data.z), *old_z_val,
						ACCEL_DELTA_Z);

	if (sufficient_x || sufficient_y || sufficient_z) {
		struct accel_event *event = new_accel_event();

		event->data = new_data;

		EVENT_SUBMIT(event);
	}

	k_work_schedule(&accel_work, K_SECONDS(ACCEL_PERIOD));
}
#endif /* defined(CONFIG_SENSOR_MODULE_ACCEL) */

#if defined(CONFIG_SENSOR_MODULE_TEMP)
static void temp_work_cb(struct k_work *work)
{
	double *old_temp_val;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;
	struct sensor_value new_temp_val;

	/* Get latest registered temperature value */
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
				  (void **)(&old_temp_val), &dummy_data_len, &dummy_data_flags);

	env_sensor_read_temperature(&new_temp_val);

	if (double_sufficient_change(sensor_value_to_double(&new_temp_val), *old_temp_val,
				     TEMP_DELTA)) {
		struct sensor_event *event = new_sensor_event();

		event->type = TEMPERATURE_SENSOR;
		event->sensor_value = new_temp_val;

		EVENT_SUBMIT(event);
	}

	k_work_schedule(&temp_work, K_SECONDS(TEMP_PERIOD));
}
#endif /* defined(CONFIG_SENSOR_MODULE_TEMP) */

#if defined(CONFIG_SENSOR_MODULE_PRESS)
static void press_work_cb(struct k_work *work)
{
	double *old_press_val;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;
	struct sensor_value new_press_val;

	/* Get latest registered pressure value */
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
				  (void **)(&old_press_val), &dummy_data_len, &dummy_data_flags);

	env_sensor_read_pressure(&new_press_val);

	if (double_sufficient_change(sensor_value_to_double(&new_press_val), *old_press_val,
				     PRESS_DELTA)) {
		struct sensor_event *event = new_sensor_event();

		event->type = PRESSURE_SENSOR;
		event->sensor_value = new_press_val;

		EVENT_SUBMIT(event);
	}

	k_work_schedule(&press_work, K_SECONDS(PRESS_PERIOD));
}
#endif /* defined(CONFIG_SENSOR_MODULE_PRESS) */

#if defined(CONFIG_SENSOR_MODULE_HUMID)
static void humid_work_cb(struct k_work *work)
{
	double *old_humid_val;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;
	struct sensor_value new_humid_val;

	/* Get latest registered humidity value */
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
				  (void **)(&old_humid_val), &dummy_data_len, &dummy_data_flags);

	env_sensor_read_humidity(&new_humid_val);

	if (double_sufficient_change(sensor_value_to_double(&new_humid_val), *old_humid_val,
				     HUMID_DELTA)) {
		struct sensor_event *event = new_sensor_event();

		event->type = HUMIDITY_SENSOR;
		event->sensor_value = new_humid_val;

		EVENT_SUBMIT(event);
	}

	k_work_schedule(&humid_work, K_SECONDS(HUMID_PERIOD));
}
#endif /* defined(CONFIG_SENSOR_MODULE_HUMID) */

#if defined(CONFIG_SENSOR_MODULE_GAS_RES)
static void gas_res_work_cb(struct k_work *work)
{
	double *old_gas_res_val;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;
	struct sensor_value new_gas_res_val;

	/* Get latest registered gas resistance value */
	lwm2m_engine_get_res_data(LWM2M_PATH(IPSO_OBJECT_GENERIC_SENSOR_ID, 0, SENSOR_VALUE_RID),
				  (void **)(&old_gas_res_val), &dummy_data_len, &dummy_data_flags);

	env_sensor_read_gas_resistance(&new_gas_res_val);

	if (double_sufficient_change(sensor_value_to_double(&new_gas_res_val), *old_gas_res_val,
				     GAS_RES_DELTA)) {
		struct sensor_event *event = new_sensor_event();

		event->type = GAS_RESISTANCE_SENSOR;
		event->sensor_value = new_gas_res_val;

		EVENT_SUBMIT(event);
	}

	k_work_schedule(&gas_res_work, K_SECONDS(GAS_RES_PERIOD));
}
#endif /* defined(CONFIG_SENSOR_MODULE_GAS_RES) */

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

#if defined(CONFIG_SENSOR_MODULE_LIGHT)
static void light_work_cb(struct k_work *work)
{
	char *old_light_val_str;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;
	uint32_t old_light_val;
	uint32_t new_light_val;

	/* Get latest registered light value */
	lwm2m_engine_get_res_data(
		LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID, COLOUR_RID),
		(void **)(&old_light_val_str), &dummy_data_len, &dummy_data_flags);
	old_light_val = strtol(old_light_val_str, NULL, 0);

	/* Read sensor, try again later if busy */
	if (light_sensor_read(&new_light_val) == -EBUSY) {
		k_work_schedule(&light_work, K_MSEC(LIGHT_FETCH_DELAY_MS));
		return;
	}

	if (rgbir_sufficient_change(new_light_val, old_light_val, LIGHT_DELTA)) {
		struct sensor_event *event = new_sensor_event();

		event->type = LIGHT_SENSOR;
		event->unsigned_value = new_light_val;

		EVENT_SUBMIT(event);
	}

	k_work_schedule(&light_work, K_SECONDS(LIGHT_PERIOD));
}
#endif /* defined(CONFIG_SENSOR_MODULE_LIGHT) */

#if defined(CONFIG_SENSOR_MODULE_COLOUR)
static void colour_work_cb(struct k_work *work)
{
	char *old_colour_val_str;
	uint16_t dummy_data_len;
	uint8_t dummy_data_flags;
	uint32_t old_colour_val;
	uint32_t new_colour_val;

	/* Get latest registered colour value */
	lwm2m_engine_get_res_data(
		LWM2M_PATH(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID, COLOUR_RID),
		(void **)(&old_colour_val_str), &dummy_data_len, &dummy_data_flags);
	old_colour_val = strtol(old_colour_val_str, NULL, 0);

	/* Read sensor, try again later if busy */
	if (colour_sensor_read(&new_colour_val) == -EBUSY) {
		k_work_schedule(&colour_work, K_MSEC(COLOUR_FETCH_DELAY_MS));
		return;
	}

	if (rgbir_sufficient_change(new_colour_val, old_colour_val, COLOUR_DELTA)) {
		struct sensor_event *event = new_sensor_event();

		event->type = COLOUR_SENSOR;
		event->unsigned_value = new_colour_val;

		EVENT_SUBMIT(event);
	}

	k_work_schedule(&colour_work, K_SECONDS(COLOUR_PERIOD));
}
#endif /* defined(CONFIG_SENSOR_MODULE_COLOUR) */

int sensor_module_init(void)
{
#if defined(CONFIG_SENSOR_MODULE_ACCEL)
	k_work_init_delayable(&accel_work, accel_work_cb);
	k_work_schedule(&accel_work, ACCEL_STARTUP_DELAY);
#endif

#if defined(CONFIG_SENSOR_MODULE_TEMP)
	k_work_init_delayable(&temp_work, temp_work_cb);
	k_work_schedule(&temp_work, TEMP_STARTUP_DELAY);
#endif

#if defined(CONFIG_SENSOR_MODULE_PRESS)
	k_work_init_delayable(&press_work, press_work_cb);
	k_work_schedule(&press_work, PRESS_STARTUP_DELAY);
#endif

#if defined(CONFIG_SENSOR_MODULE_HUMID)
	k_work_init_delayable(&humid_work, humid_work_cb);
	k_work_schedule(&humid_work, HUMID_STARTUP_DELAY);
#endif

#if defined(CONFIG_SENSOR_MODULE_GAS_RES)
	k_work_init_delayable(&gas_res_work, gas_res_work_cb);
	k_work_schedule(&gas_res_work, GAS_RES_STARTUP_DELAY);
#endif

#if defined(CONFIG_SENSOR_MODULE_LIGHT)
	k_work_init_delayable(&light_work, light_work_cb);
	k_work_schedule(&light_work, LIGHT_STARTUP_DELAY);
#endif

#if defined(CONFIG_SENSOR_MODULE_COLOUR)
	k_work_init_delayable(&colour_work, colour_work_cb);
	k_work_schedule(&colour_work, COLOUR_STARTUP_DELAY);
#endif

	return 0;
}
