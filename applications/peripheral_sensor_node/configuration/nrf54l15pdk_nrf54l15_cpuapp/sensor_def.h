/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor_sampler.h"

/* This configuration file is included only once from sensor_sampler module and holds
 * information about the sampled sensors.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} app_sensors_def_include_once;


static const struct sampled_channel bme688_chan[] = {
	{
		.chan = SENSOR_CHAN_AMBIENT_TEMP,
		.data_cnt = 1,
	},
	{
		.chan = SENSOR_CHAN_PRESS,
		.data_cnt = 1,
	},
	{
		.chan = SENSOR_CHAN_HUMIDITY,
		.data_cnt = 1,
	},
	{
		.chan = SENSOR_CHAN_GAS_RES,
		.data_cnt = 1,
	},
};

static const struct sampled_channel bmi270_chan[] = {
	{
		.chan = SENSOR_CHAN_ACCEL_XYZ,
		.data_cnt = 3,
	},
	{
		.chan = SENSOR_CHAN_GYRO_XYZ,
		.data_cnt = 3,
	},
};

static const struct sampled_channel adxl362_chan[] = {
	{
		.chan = SENSOR_CHAN_ACCEL_XYZ,
		.data_cnt = 3,
	},
};

static const struct sensor_config sensor_configs[] = {
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(bme688)),
		.event_descr = "env",
		.chans = bme688_chan,
		.chan_cnt = ARRAY_SIZE(bme688_chan),
		.events_limit = 3,
	},
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(bmi270)),
		.event_descr = "imu",
		.chans = bmi270_chan,
		.chan_cnt = ARRAY_SIZE(bmi270_chan),
		.events_limit = 3,
	},
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(adxl362)),
		.event_descr = "wu_imu",
		.chans = adxl362_chan,
		.chan_cnt = ARRAY_SIZE(adxl362_chan),
		.events_limit = 3,
	},
};
