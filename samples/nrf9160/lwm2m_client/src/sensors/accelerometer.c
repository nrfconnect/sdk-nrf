/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/sensor.h>

#include "accelerometer.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(accelerometer, CONFIG_APP_LOG_LEVEL);

#if defined(CONFIG_ACCEL_USE_EXTERNAL)
#define ACCEL_NODE			DT_PATH(soc, peripheral_40000000, spi_b000, adxl362_0)
#define ACCEL_DEV_LABEL		DT_LABEL(ACCEL_NODE)
#elif defined(CONFIG_ACCEL_USE_SIM)
#define ACCEL_DEV_LABEL		"SENSOR_SIM"
#endif

#if defined(CONFIG_ACCEL_CALIBRATE_ON_STARTUP)
#define CALIBRATION_ITERATIONS CONFIG_ACCEL_CALIBRATION_ITERATIONS
#endif

static const struct device *accel_dev;

static double accel_offset[3];

int accelerometer_read(struct accelerometer_sensor_data *data)
{
	int ret;
	double x_temp, y_temp, z_temp;

	ret = sensor_sample_fetch(accel_dev);
	if (ret) {
		LOG_ERR("Fetch sample failed (%d)", ret);
		return ret;
	}

	ret = sensor_channel_get(accel_dev, SENSOR_CHAN_ACCEL_X, &(data->x));
	if (ret) {
		LOG_ERR("Get x channel failed (%d)", ret);
		return ret;
	}
	ret = sensor_channel_get(accel_dev, SENSOR_CHAN_ACCEL_Y, &(data->y));
	if (ret) {
		LOG_ERR("Get y channel failed (%d)", ret);
		return ret;
	}
	ret = sensor_channel_get(accel_dev, SENSOR_CHAN_ACCEL_Z, &(data->z));
	if (ret) {
		LOG_ERR("Get z channel failed (%d)", ret);
		return ret;
	}

	/* Adjust for sensor bias */
	x_temp = sensor_value_to_double(&(data->x)) - accel_offset[0];
	y_temp = sensor_value_to_double(&(data->y)) - accel_offset[1];
	z_temp = sensor_value_to_double(&(data->z)) - accel_offset[2];
	sensor_value_from_double(&(data->x), x_temp);
	sensor_value_from_double(&(data->y), y_temp);
	sensor_value_from_double(&(data->z), z_temp);

	return 0;
}

#if defined(CONFIG_ACCEL_CALIBRATE_ON_STARTUP)
static int accelerometer_calibrate(void)
{
	int ret;
	struct accelerometer_sensor_data accel_data;
	double aggr_data[3] = { 0 };

	/* TODO: Check if this is needed for accurate calibration */
	k_sleep(K_SECONDS(2));

	for (int i = 0; i < CALIBRATION_ITERATIONS; i++) {
		ret = accelerometer_read(&accel_data);
		if (ret) {
			LOG_ERR("Read accelerometer failed (%d)", ret);
			return ret;
		}

		aggr_data[0] += sensor_value_to_double(&(accel_data.x));
		aggr_data[1] += sensor_value_to_double(&(accel_data.y));
		aggr_data[2] +=
			sensor_value_to_double(&(accel_data.z)) + ((double)SENSOR_G) / 1000000.0;
	}

	accel_offset[0] = aggr_data[0] / (double)CALIBRATION_ITERATIONS;
	accel_offset[1] = aggr_data[1] / (double)CALIBRATION_ITERATIONS;
	accel_offset[2] = aggr_data[2] / (double)CALIBRATION_ITERATIONS;

	LOG_INF("Finished accelerometer calibration");

	return 0;
}
#endif /* if defined(CONFIG_ACCEL_CALIBRATE_ON_STARTUP) */

int accelerometer_init(void)
{
	accel_dev = device_get_binding(ACCEL_DEV_LABEL);
	if (!accel_dev) {
		LOG_ERR("Could not get accelerometer device (%d)", -ENODEV);
		return -ENODEV;
	}

#if defined(CONFIG_ACCEL_CALIBRATE_ON_STARTUP)
	int ret;

	ret = accelerometer_calibrate();
	if (ret) {
		LOG_ERR("Calibrate accelerometer failed (%d)", ret);
		return ret;
	}
#endif

	return 0;
}
