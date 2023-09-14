/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor_sampler.h"

/**
 * @file
 * @brief Configuration file for sampled sensors.
 *
 * This configuration file is included only once from sensor_sampler module and holds
 * information about the sampled sensors.
 */

/**
 * @brief Structure to ensure single inclusion of this header file.
 *
 * This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} app_sensors_def_include_once;

/** @brief Configuration of sampled channels for the BME688 sensor. */
static const struct sampled_channel bme688_chan[] = {
	{
		.chan = SENSOR_CHAN_AMBIENT_TEMP,	/**< Ambient temperature channel. */
		.data_cnt = 1,				/**< Number of data samples. */
	},
	{
		.chan = SENSOR_CHAN_PRESS,		/**< Pressure channel. */
		.data_cnt = 1,				/**< Number of data samples. */
	},
	{
		.chan = SENSOR_CHAN_HUMIDITY,		/**< Humidity channel. */
		.data_cnt = 1,				/**< Number of data samples. */
	},
	{
		.chan = SENSOR_CHAN_GAS_RES,		/**< Gas resistance channel. */
		.data_cnt = 1,				/**< Number of data samples. */
	},
};

/** @brief Configuration of sampled channels for the BMI270 sensor. */
static const struct sampled_channel bmi270_chan[] = {
	{
		.chan = SENSOR_CHAN_ACCEL_XYZ,		/**< Accelerometer XYZ channel. */
		.data_cnt = 3,				/**< Number of data samples. */
	},
	{
		.chan = SENSOR_CHAN_GYRO_XYZ,		/**< Gyroscope XYZ channel. */
		.data_cnt = 3,				/**< Number of data samples. */
	},
};

/** @brief Configuration of sampled channels for the ADXL362 sensor. */
static const struct sampled_channel adxl362_chan[] = {
	{
		.chan = SENSOR_CHAN_ACCEL_XYZ,		/**< Accelerometer XYZ channel. */
		.data_cnt = 3,				/**< Gyroscope XYZ channel. */
	},
};

/** @brief Configuration array for all sensors used in the application. */
static const struct sensor_config sensor_configs[] = {
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(bme688)),	/**< BME688 sensor device. */
		.event_descr = "env",				/**< Event description. */
		.chans = bme688_chan,				/**< Used channels description. */
		.chan_cnt = ARRAY_SIZE(bme688_chan),		/**< Number of channels. */
		.events_limit = 3,				/**< Maximum unprocessed events. */
	},
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(bmi270)),	/**< BMI270 sensor device. */
		.event_descr = "imu",				/**< Event description. */
		.chans = bmi270_chan,				/**< Used channels description. */
		.chan_cnt = ARRAY_SIZE(bmi270_chan),		/**< Number of channels. */
		.events_limit = 3,				/**< Maximum unprocessed events. */
	},
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(adxl362)),	/**< ADXL362 sensor device. */
		.event_descr = "wu_imu",			/**< Event description. */
		.chans = adxl362_chan,				/**< Used channels description. */
		.chan_cnt = ARRAY_SIZE(adxl362_chan),		/**< Number of channels. */
		.events_limit = 3,				/**< Maximum unprocessed events. */
	},
};
