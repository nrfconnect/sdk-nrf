/**
 * @file gps.h
 *
 * @brief Public APIs for the GPS driver.
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef ZEPHYR_INCLUDE_GPS_H_
#define ZEPHYR_INCLUDE_GPS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>
#include <errno.h>
#include <zephyr/types.h>

#define GPS_NMEA_SENTENCE_MAX_LENGTH 81

struct gps_data {
	char str[GPS_NMEA_SENTENCE_MAX_LENGTH];
	u8_t len;
};

enum gps_channel { GPS_CHAN_NMEA };

/**
 * @brief GPS trigger types.
 */
enum gps_trigger_type {
	GPS_TRIG_TIMER,
	GPS_TRIG_DATA_READY,
};

/**
 * @brief GPS trigger spec.
 */
struct gps_trigger {
	/** Trigger type. */
	enum gps_trigger_type type;
	/** Channel the trigger is set on. */
	enum gps_channel chan;
};

/**
 * @typedef gps_trigger_handler_t
 * @brief Callback API upon firing of a trigger
 *
 * @param "struct device *dev" Pointer to the GPS device
 * @param "struct gps_trigger *trigger" The trigger
 */
typedef void (*gps_trigger_handler_t)(struct device *dev,
				      struct gps_trigger *trigger);

/**
 * @typedef gps_trigger_set_t
 * @brief Callback API for setting a gps's trigger and handler
 *
 * See gps_trigger_set() for argument description
 */
typedef int (*gps_trigger_set_t)(struct device *dev,
				 const struct gps_trigger *trig,
				 gps_trigger_handler_t handler);

/**
 * @typedef gps_sample_fetch_t
 * @brief Callback API for fetching data from a GPS device.
 *
 * See gps_sample_fetch() for argument description
 */
typedef int (*gps_sample_fetch_t)(struct device *dev);

/**
 * @typedef gps_channel_get_t
 * @brief Callback API for getting a reading from a GPS device
 *
 * See gps_channel_get() for argument description
 */
typedef int (*gps_channel_get_t)(struct device *dev, enum gps_channel chan,
				 struct gps_data *val);

/**
 * @brief GPS driver API
 *
 * This is the API all GPS drivers must expose.
 */
struct gps_driver_api {
	gps_trigger_set_t trigger_set;
	gps_sample_fetch_t sample_fetch;
	gps_channel_get_t channel_get;
};

/**
 * @brief Function to trigger sampling of data from GPS.
 *
 * @param dev Pointer to GPS device
 */
static inline int gps_sample_fetch(struct device *dev)
{
	const struct gps_driver_api *api =
		(const struct gps_driver_api *)dev->driver_api;

	return api->sample_fetch(dev);
}

/**
 * @brief Function to get data from a single GPS data channel.
 *
 * @param dev Pointer to GPS device
 * @param chan Channel to get data from
 * @param data Pointer to structure where to store the sampled data.
 */
static inline int gps_channel_get(struct device *dev, enum gps_channel chan,
				  struct gps_data *data)
{
	const struct gps_driver_api *api =
		(const struct gps_driver_api *)dev->driver_api;

	return api->channel_get(dev, chan, data);
}

/**
 * @brief Function to set GPS simulator trigger.
 *
 * @param dev Pointer to GPS device
 * @param chan Channel for trigger
 * @param data Pointer to structure where to store the sampled data.
 */
static inline int gps_trigger_set(struct device *dev,
				  const struct gps_trigger *trigger,
				  gps_trigger_handler_t handler)
{
	const struct gps_driver_api *api =
		(const struct gps_driver_api *)dev->driver_api;

	return api->trigger_set(dev, trigger, handler);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_GPS_H_ */
