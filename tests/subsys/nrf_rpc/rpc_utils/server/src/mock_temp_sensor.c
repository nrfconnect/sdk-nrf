/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

/* Mock temperature sensor for testing die_temp RPC */

struct mock_temp_sensor_data {
	struct sensor_value value;
	int fetch_result;
	int channel_get_result;
};

static struct mock_temp_sensor_data mock_data = {
	.value = {0, 0},
	.fetch_result = 0,
	.channel_get_result = 0,
};

void mock_temp_sensor_set_value(struct sensor_value *val)
{
	mock_data.value = *val;
}

void mock_temp_sensor_set_fetch_result(int result)
{
	mock_data.fetch_result = result;
}

void mock_temp_sensor_set_channel_get_result(int result)
{
	mock_data.channel_get_result = result;
}

static int mock_temp_sensor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mock_temp_sensor_data *data = dev->data;

	ARG_UNUSED(chan);

	return data->fetch_result;
}

static int mock_temp_sensor_channel_get(const struct device *dev, enum sensor_channel chan,
					struct sensor_value *val)
{
	struct mock_temp_sensor_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	if (data->channel_get_result != 0) {
		return data->channel_get_result;
	}

	*val = data->value;
	return 0;
}

static int mock_temp_sensor_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static const struct sensor_driver_api mock_temp_sensor_api = {
	.sample_fetch = mock_temp_sensor_sample_fetch,
	.channel_get = mock_temp_sensor_channel_get,
};

DEVICE_DT_DEFINE(DT_NODELABEL(temp), mock_temp_sensor_init, NULL, &mock_data, NULL, POST_KERNEL,
		 CONFIG_SENSOR_INIT_PRIORITY, &mock_temp_sensor_api);
