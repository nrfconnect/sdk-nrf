/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_sensors, CONFIG_APP_LOG_LEVEL);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sensor_sim), okay)
#define ENV_SENSOR_NODE DT_NODELABEL(sensor_sim)
#define GAS_RES_SIM_BASE CONFIG_ENV_SENSOR_GAS_RES_SIM_BASE
#define GAS_RES_SIM_MAX_DIFF CONFIG_ENV_SENSOR_GAS_RES_SIM_MAX_DIFF
#else
#define ENV_SENSOR_NODE DT_NODELABEL(bme680)
#endif

static const struct device *env_sensor_dev = DEVICE_DT_GET(ENV_SENSOR_NODE);

static int read_sensor(struct sensor_value *value, enum sensor_channel channel)
{
	int ret;

	ret = sensor_sample_fetch(env_sensor_dev);
	if (ret) {
		LOG_ERR("Fetch sample failed (%d)", ret);
		return ret;
	}

	ret = sensor_channel_get(env_sensor_dev, channel, value);
	if (ret) {
		LOG_ERR("Get channel failed (%d)", ret);
		return ret;
	}

	return 0;
}

int env_sensor_read_temperature(struct sensor_value *temp_val)
{
	int ret;

	ret = read_sensor(temp_val, SENSOR_CHAN_AMBIENT_TEMP);
	if (ret) {
		LOG_ERR("Read temperatur sensor failed (%d)", ret);
		return ret;
	}

	return 0;
}

int env_sensor_read_pressure(struct sensor_value *press_value)
{
	int ret;

	ret = read_sensor(press_value, SENSOR_CHAN_PRESS);
	if (ret) {
		LOG_ERR("Read pressure sensor failed (%d)", ret);
		return ret;
	}

	return 0;
}

int env_sensor_read_humidity(struct sensor_value *humid_val)
{
	int ret;

	ret = read_sensor(humid_val, SENSOR_CHAN_HUMIDITY);
	if (ret) {
		LOG_ERR("Read humidity sensor failed (%d)", ret);
		return ret;
	}

	return 0;
}

int env_sensor_read_gas_resistance(struct sensor_value *gas_res_val)
{
#if !DT_NODE_HAS_STATUS(DT_NODELABEL(sensor_sim), okay)
	int ret;

	ret = read_sensor(gas_res_val, SENSOR_CHAN_GAS_RES);
	if (ret) {
		LOG_ERR("Read gas resistance sensor failed (%d)", ret);
		return ret;
	}
#else
	int32_t sim_val =
		MAX(0, GAS_RES_SIM_BASE + (rand() % GAS_RES_SIM_MAX_DIFF) * (1 - 2 * (rand() % 2)));

	gas_res_val->val1 = sim_val;
	gas_res_val->val2 = 0;
#endif

	return 0;
}

int env_sensor_init(void)
{
	if (!device_is_ready(env_sensor_dev)) {
		LOG_ERR("Environment sensor device not ready");
		return -ENODEV;
	}

	return 0;
}
