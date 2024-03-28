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
#include <adp536x.h>
#include "lwm2m_app_utils.h"
#include "lwm2m_engine.h"
#include "accelerometer.h"
#include "env_sensor.h"
#include "light_sensor.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_sensors, CONFIG_APP_LOG_LEVEL);

#define LIGHT_OBJ_INSTANCE_ID 0
#define COLOUR_OBJ_INSTANCE_ID 1

static struct k_work_delayable accel_work;
static struct k_work_delayable temp_work;
static struct k_work_delayable press_work;
static struct k_work_delayable humid_work;
static struct k_work_delayable gas_res_work;
static struct k_work_delayable light_work;
static struct k_work_delayable colour_work;
static struct k_work_delayable pmic_work;

#define PERIOD K_MINUTES(2)
#define RETRY K_MSEC(200)

static void accel_work_cb(struct k_work *work)
{
	double x;
	double y;
	double z;
	struct accelerometer_sensor_data data;

	accelerometer_read(&data);
	x = sensor_value_to_double(&data.x);
	y = sensor_value_to_double(&data.y);
	z = sensor_value_to_double(&data.z);

	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID, 0, X_VALUE_RID), x);
	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID, 0, Y_VALUE_RID), y);
	lwm2m_set_f64(&LWM2M_OBJ(IPSO_OBJECT_ACCELEROMETER_ID, 0, Z_VALUE_RID), z);

	k_work_schedule(&accel_work, PERIOD);
}

static void sensor_worker(int (*read_cb)(struct sensor_value *), const struct lwm2m_obj_path *path)
{
	double val;
	struct sensor_value data;
	int rc;

	rc = read_cb(&data);
	if (rc) {
		LOG_ERR("Failed to read sensor data");
		return;
	}
	val = sensor_value_to_double(&data);

	lwm2m_set_f64(path, val);
}

static void temp_work_cb(struct k_work *work)
{
	sensor_worker(env_sensor_read_temperature,
		      &LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID));

	k_work_schedule(&temp_work, PERIOD);
}

static void press_work_cb(struct k_work *work)
{
	sensor_worker(env_sensor_read_pressure,
		      &LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID));

	k_work_schedule(&press_work, PERIOD);
}

static void humid_work_cb(struct k_work *work)
{
	sensor_worker(env_sensor_read_humidity,
		      &LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID));

	k_work_schedule(&humid_work, PERIOD);
}

static void gas_res_work_cb(struct k_work *work)
{
	sensor_worker(env_sensor_read_gas_resistance,
		      &LWM2M_OBJ(IPSO_OBJECT_GENERIC_SENSOR_ID, 0, SENSOR_VALUE_RID));

	k_work_schedule(&gas_res_work, PERIOD);
}

static int light_sensor_worker(int (*read_cb)(uint32_t *), const struct lwm2m_obj_path *path)
{
	uint32_t val;
	int ret;
	char temp[RGBIR_STR_LENGTH];

	/* Read sensor, try again later if busy */
	if (read_cb(&val) == -EBUSY) {
		return -1;
	}

	ret = snprintk(temp, RGBIR_STR_LENGTH, "0x%08X", val);
	if (ret <= 0) {
		return -ENOMEM;
	}
	ret = lwm2m_set_string(path, temp);
	if (ret) {
		return ret;
	}

	return 0;
}

static void light_work_cb(struct k_work *work)
{
	int rc = light_sensor_worker(light_sensor_read,
				     &LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, LIGHT_OBJ_INSTANCE_ID,
						COLOUR_RID));

	if (rc) {
		k_work_schedule(&light_work, RETRY);
		return;
	}

	k_work_schedule(&light_work, PERIOD);
}

static void colour_work_cb(struct k_work *work)
{
	int rc = light_sensor_worker(colour_sensor_read,
				     &LWM2M_OBJ(IPSO_OBJECT_COLOUR_ID, COLOUR_OBJ_INSTANCE_ID,
						COLOUR_RID));

	if (rc) {
		k_work_schedule(&light_work, RETRY);
		return;
	}

	k_work_schedule(&colour_work, PERIOD);
}

static void pmic_work_cb(struct k_work *work)
{
	int rc;
	uint8_t percentage;
	uint16_t millivolts;
	uint8_t status;

	rc = adp536x_fg_soc(&percentage);
	if (rc) {
		k_work_schedule(&pmic_work, RETRY);
		return;
	}

	rc = adp536x_fg_volts(&millivolts);
	if (rc) {
		k_work_schedule(&pmic_work, RETRY);
		return;
	}

	rc = adp536x_charger_status_1_read(&status);
	if (rc) {
		k_work_schedule(&pmic_work, RETRY);
		return;
	}

	lwm2m_set_s32(&LWM2M_OBJ(3, 0, 7, 0), (int) millivolts);
	lwm2m_set_u8(&LWM2M_OBJ(3, 0, 9), percentage);

	k_work_schedule(&pmic_work, PERIOD);
}

static int sensor_module_init(void)
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

	if (IS_ENABLED(CONFIG_BOARD_THINGY91_NRF9160_NS)) {
		k_work_init_delayable(&pmic_work, pmic_work_cb);
		k_work_schedule(&pmic_work, K_NO_WAIT);
	}

	return 0;
}

LWM2M_APP_INIT(sensor_module_init);
