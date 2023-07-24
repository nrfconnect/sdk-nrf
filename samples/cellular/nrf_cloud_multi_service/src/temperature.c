/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if CONFIG_TEMP_DATA_USE_SENSOR

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#else /* CONFIG_TEMP_DATA_USE_SENSOR */

#include <zephyr/random/rand32.h>

#endif /* CONFIG_TEMP_DATA_USE_SENSOR */

#include "temperature.h"

LOG_MODULE_REGISTER(temperature, CONFIG_MULTI_SERVICE_LOG_LEVEL);

#if CONFIG_TEMP_DATA_USE_SENSOR

static const struct device *temp_sensor = DEVICE_DT_GET(DT_ALIAS(temp_sensor));

int get_temperature(double *temp)
{
	int err;
	struct sensor_value data = {0};

	/* Fetch all data the sensor supports. */
	err = sensor_sample_fetch_chan(temp_sensor, SENSOR_CHAN_ALL);
	if (err) {
		LOG_ERR("Failed to sample temperature sensor, error %d", err);
		return -ENODATA;
	}

	/* Pick out the ambient temperature data. */
	err = sensor_channel_get(temp_sensor, SENSOR_CHAN_AMBIENT_TEMP, &data);
	if (err) {
		LOG_ERR("Failed to read temperature, error %d", err);
		return -ENODATA;
	}

	*temp = sensor_value_to_double(&data);

	return 0;
}

#else /* CONFIG_TEMP_DATA_USE_SENSOR */

int get_temperature(double *temp)
{
	*temp = 22.0 + (sys_rand32_get() % 100) / 40.0;
	return 0;
}

#endif /* CONFIG_TEMP_DATA_USE_SENSOR */
