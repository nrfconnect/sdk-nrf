/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/random/random.h>

#include "temperature.h"

LOG_MODULE_REGISTER(temperature, CONFIG_WIFI_NRF_CLOUD_LOG_LEVEL);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(temp))

static const struct device *temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp));

int get_temperature(double *temp)
{
	struct sensor_value val;
	int err;

	if (!device_is_ready(temp_dev)) {
		LOG_ERR("Temperature sensor %s not ready", temp_dev->name);
		return -ENODEV;
	}

	err = sensor_sample_fetch_chan(temp_dev, SENSOR_CHAN_DIE_TEMP);
	if (err) {
		LOG_ERR("Failed to fetch temperature sample, error: %d", err);
		return err;
	}

	err = sensor_channel_get(temp_dev, SENSOR_CHAN_DIE_TEMP, &val);
	if (err) {
		LOG_ERR("Failed to get temperature value, error: %d", err);
		return err;
	}

	*temp = sensor_value_to_double(&val);
	return 0;
}

#else /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(temp)) */

int get_temperature(double *temp)
{
	/* No on-die temperature sensor available, report a simulated value */
	LOG_WRN_ONCE("No die temperature sensor available, reporting simulated values");
	*temp = 40.0 + (sys_rand32_get() % 100) / 40.0;
	return 0;
}

#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(temp)) */
