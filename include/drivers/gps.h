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

#define GPS_NMEA_SENTENCE_MAX_LENGTH	83
#define GPS_MAX_SATELLITES		12

enum gps_channel {
	GPS_CHAN_NMEA,	/** Channel to receive NMEA strings. */
	GPS_CHAN_PVT	/** Channel to receive position, velocity and time. */
};

struct gps_nmea {
	char buf[GPS_NMEA_SENTENCE_MAX_LENGTH];
	u8_t len;
};

struct gps_datetime {
	u16_t year;
	u8_t month;
	u8_t day;
	u8_t hour;
	u8_t minute;
	u8_t seconds;
	u16_t ms;
};

struct gps_sv {
	u16_t sv;
	u16_t cn0;
	s16_t  elevation;
	s16_t  azimuth;
	u8_t  flags;
	u8_t  signal;
};

struct gps_pvt {
	double latitude;
	double longitude;
	float altitude;
	float accuracy;
	float speed;
	float heading;
	float pdop;
	float hdop;
	float vdop;
	float tdop;
	u8_t flags;
	struct gps_datetime datetime;
	struct gps_sv sv[GPS_MAX_SATELLITES];
};

struct gps_data {
	enum gps_channel chan;
	union {
		struct gps_nmea nmea;
		struct gps_pvt pvt;
	};

};

/**
 * @brief GPS trigger types.
 */
enum gps_trigger_type {
	GPS_TRIG_TIMER,
	GPS_TRIG_DATA_READY,
	GPS_TRIG_FIX
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
 * @typedef gps_start_t
 * @brief Callback API for starting GPS operation.
 *
 * See gps_start() for argument description
 */
typedef int (*gps_start_t)(struct device *dev);

/**
 * @typedef gps_stop_t
 * @brief Callback API for stopping GPS operation.
 *
 * See gps_stop() for argument description
 */
typedef int (*gps_stop_t)(struct device *dev);

/**
 * @brief GPS driver API
 *
 * This is the API all GPS drivers must expose.
 */
struct gps_driver_api {
	gps_trigger_set_t trigger_set;
	gps_sample_fetch_t sample_fetch;
	gps_channel_get_t channel_get;
	gps_start_t start;
	gps_stop_t stop;
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

/**
 * @brief Function to start GPS operation.
 *
 * @param dev Pointer to GPS device
 */
static inline int gps_start(struct device *dev)
{
	const struct gps_driver_api *api =
		(const struct gps_driver_api *)dev->driver_api;

	return api->start(dev);
}

/**
 * @brief Function to stop GPS operation.
 *
 * @param dev Pointer to GPS device
 */
static inline int gps_stop(struct device *dev)
{
	const struct gps_driver_api *api =
		(const struct gps_driver_api *)dev->driver_api;

	return api->stop(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_GPS_H_ */
