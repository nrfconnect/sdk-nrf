/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>

#include "sensor.h"

/*
 * Sensor value is represented as having an integer and a fractional part,
 * and can be obtained using the formula val1 + val2 * 10^(-6).
 */
#define SENSOR_VAL2_DIVISOR 1000000

LOG_MODULE_DECLARE(app, LOG_LEVEL_INF);

/* Structure for storing sensor measurement data */
struct sensor_data_t {
	struct sensor_value temperature;
	struct sensor_value pressure;
	struct sensor_value humidity;
};

static const struct device *sensor;
static struct sensor_data_t sensor_data;

/*
 * Sensor value is represented as having an integer and a fractional part,
 * and can be obtained using the formula val1 + val2 * 10^(-6). Negative
 * values also adhere to the above formula, but may need special attention.
 * Here are some examples of the value representation:
 *
 *      0.5: val1 =  0, val2 =  500000
 *     -0.5: val1 =  0, val2 = -500000
 *     -1.0: val1 = -1, val2 =  0
 *     -1.5: val1 = -1, val2 = -500000
 */
static float convert_sensor_value(struct sensor_value *value)
{
	float result = 0.0f;

	if (value) {
		/* Determine sign */
		result = (value->val1 < 0 || value->val2 < 0) ? -1.0f : 1.0f;

		/* Use absolute values */
		value->val1 = value->val1 < 0 ? -value->val1 : value->val1;
		value->val2 = value->val2 < 0 ? -value->val2 : value->val2;

		/* Calculate value */
		result *= (value->val1 + value->val2 / (float)SENSOR_VAL2_DIVISOR);
	} else {
		LOG_ERR("NULL pointer: value");
	}

	return result;
}

void sensor_init(void)
{
	if (sensor) {
		LOG_WRN("Sensor already initialized");
	} else {
		LOG_INF("Initializing sensor...");

		sensor_data.temperature.val1 = 0;
		sensor_data.temperature.val2 = 0;
		sensor_data.pressure.val1 = 0;
		sensor_data.pressure.val2 = 0;
		sensor_data.humidity.val1 = 0;
		sensor_data.humidity.val2 = 0;

		sensor = DEVICE_DT_GET(DT_INST(0, bosch_bme680));
		if (sensor) {
			LOG_INF("Initializing sensor...OK");
		} else {
			LOG_ERR("Initializing sensor...FAILED");
		}
	}
}

void sensor_update(void)
{
	if (sensor) {
		sensor_sample_fetch(sensor);

		sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &sensor_data.temperature);
		sensor_channel_get(sensor, SENSOR_CHAN_PRESS, &sensor_data.pressure);
		sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &sensor_data.humidity);

		LOG_INF("Sensor     T:%3d.%06d [*C] P:%3d.%06d [kPa] H:%3d.%06d [%%]",
			sensor_data.temperature.val1, sensor_data.temperature.val2,
			sensor_data.pressure.val1, sensor_data.pressure.val2,
			sensor_data.humidity.val1, sensor_data.humidity.val2);
	} else {
		LOG_ERR("Sensor not initialized");
	}
}

float sensor_get_temperature(void)
{
	return convert_sensor_value(&sensor_data.temperature);
}

float sensor_get_pressure(void)
{
	return convert_sensor_value(&sensor_data.pressure);
}

float sensor_get_humidity(void)
{
	return convert_sensor_value(&sensor_data.humidity);
}
