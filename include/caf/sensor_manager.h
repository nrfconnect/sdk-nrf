/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SENSOR_MANAGER_H_
#define _SENSOR_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

enum act_type {
	ACT_TYPE_PERC,
	ACT_TYPE_ABS,
};

struct sm_trigger_activation {
	enum act_type type;
	float thresh;
	unsigned int timeout_ms;
};

struct sm_trigger {
	struct sensor_trigger cfg;
	struct sm_trigger_activation activation;
};

/**
 * @brief Description of single channel
 *
 * The description of the channel to do the measurement on.
 */
struct sm_sampled_channel {
	/** @brief Channel identifier */
	enum sensor_channel chan;
	/** @brief Number of data samples in selected channel */
	uint8_t data_cnt;
};

/**
 * @brief Sensor configuration
 *
 * The sensor configuration is provided by the application in file specified by
 * the :kconfig:`CONFIG_CAF_SENSOR_MANAGER_DEF_PATH` option.
 */
struct sm_sensor_config {
	/**
	 * @brief Device name
	 *
	 * The device name that would be used to access the interface
	 * using device_get_binding().
	 */
	const char *dev_name;
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
	 * Channels in the sensor that should be handled by sensor manager module
	 */
	const struct sm_sampled_channel *chans;
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
	 * be passed to event manager.
	 */
	uint8_t active_events_limit;
	/**
	 * @brief Sampling period
	 */
	unsigned int sampling_period_ms;
	/**
	 * @brief Sensor trigger configuration
	 *
	 * Trigger is used for sensor activity checking and to wake up
	 * from suspend.
	 */
	struct sm_trigger *trigger;
	/**
	 * @brief Flag to indicate whether sensor should be suspended or not.
	 */
	bool suspend;
};

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_MANAGER_H_ */
