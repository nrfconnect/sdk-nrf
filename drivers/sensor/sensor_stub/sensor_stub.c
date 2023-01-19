/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_sensor_stub

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <drivers/sensor_stub.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_stub, CONFIG_SENSOR_LOG_LEVEL);


struct sensor_stub_data {
	void *udata;
};

struct sensor_stub_config {
	int (*init)(const struct device *dev);
};


void *sensor_stub_udata_get(const struct device *dev)
{
	struct sensor_stub_data *data = dev->data;

	return data->udata;
}

void sensor_stub_udata_set(const struct device *dev, void *udata)
{
	struct sensor_stub_data *data = dev->data;

	data->udata = udata;
}

static int sensor_stub_init(const struct device *dev)
{
	int ret;
	const struct sensor_stub_config *config = dev->config;

	ret = config->init(dev);
	if (ret) {
		LOG_ERR("%s, Initialization error: %d", dev->name, ret);
		return ret;
	}

	LOG_DBG("%s: Initialization complete", dev->name);
	return 0;
}

#define SENSOR_STUB_DEFINE(_idx)                                                                   \
	extern int _CONCAT(DT_STRING_TOKEN(DT_DRV_INST(_idx), generator), _fetch)(                 \
		const struct device *dev, enum sensor_channel ch);                                 \
	extern int _CONCAT(DT_STRING_TOKEN(DT_DRV_INST(_idx), generator), _get)(                   \
		const struct device *dev, enum sensor_channel ch, struct sensor_value *val);       \
	extern int _CONCAT(DT_STRING_TOKEN(DT_DRV_INST(_idx), generator), _init)(                  \
		const struct device *dev);                                                         \
												   \
	static const struct sensor_driver_api sensor_stub_api_funcs_##_idx = {                     \
		.sample_fetch = _CONCAT(DT_STRING_TOKEN(DT_DRV_INST(_idx), generator), _fetch),    \
		.channel_get  = _CONCAT(DT_STRING_TOKEN(DT_DRV_INST(_idx), generator), _get)       \
	};                                                                                         \
												   \
	static struct sensor_stub_data data_##_idx;                                                \
	const struct sensor_stub_config config_##_idx = {                                          \
		.init = _CONCAT(DT_STRING_TOKEN(DT_DRV_INST(_idx), generator), _init)              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(_idx, sensor_stub_init, NULL, &data_##_idx, &config_##_idx,          \
				      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	                   \
				      &sensor_stub_api_funcs_##_idx);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_STUB_DEFINE)
