/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_SAMPLER_H_
#define _SENSOR_SAMPLER_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Description of single channel
 *
 * The description of the channel to do the measurement on.
 */
struct sampled_channel {
	/** @brief Channel identifier */
	enum sensor_channel chan;
	/** @brief Number of data samples in selected channel */
	uint8_t data_cnt;
};

/**
 * @brief Sensor configuration
 *
 * The sensor configuration is provided by the application in file specified by
 * the :kconfig:option:`CONFIG_CAF_SENSOR_SAMPLER_DEF_PATH` option.
 */
struct sensor_config {
	/**
	 * @brief Device
	 */
	const struct device *dev;
	/**
	 * @brief Event descriptor
	 *
	 * Descriptor that would be used to identify source of sensor_events
	 * by the application modules.
	 */
	const char *event_descr;
	/**
	 * @brief Used channels description
	 *
	 * Channels in the sensor that should be handled by sensor sampler module
	 */
	const struct sampled_channel *chans;
	/**
	 * @brief Number of channels
	 *
	 * Number of channels handled for the sensor
	 */
	uint8_t chan_cnt;
	/**
	 * @brief Allowed number of unprocessed events
	 *
	 * This is a protection against OOM error when event processing is blocked.
	 * No more events related to this sensor than the number defined would
	 * be passed to Application Event Manager.
	 */
	uint8_t events_limit;
};


#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_SAMPLER_H_ */
