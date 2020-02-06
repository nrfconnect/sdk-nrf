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

#include <zephyr.h>
#include <device.h>
#include <stdbool.h>

#define GPS_NMEA_SENTENCE_MAX_LENGTH	83
#define GPS_PVT_MAX_SV_COUNT		12

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
	u16_t sv;		/**< SV number 1...32 for GPS. */
	u16_t cn0;		/**< 0.1 dB/Hz. */
	s16_t elevation;	/**< SV elevation angle in degrees. */
	s16_t azimuth;		/**< SV azimuth angle in degrees. */
	u8_t signal;		/**< Signal type. 0: invalid, 1: GPS L1C/A. */
	u8_t in_fix:1;		/**< Satellite used in fix calculation. */
	u8_t unhealthy:1;	/**< Satellite is marked as unhealthy. */
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
	float gdop;
	struct gps_datetime datetime;
	struct gps_sv sv[GPS_PVT_MAX_SV_COUNT];
};

enum gps_nav_mode {
	/* Search will be stopped after first fix. */
	GPS_NAV_MODE_SINGLE_FIX,

	/* Search continues until explicitly stopped. */
	GPS_NAV_MODE_CONTINUOUS,

	/* Periodically start search, stops on fix. */
	GPS_NAV_MODE_PERIODIC,
};

enum gps_power_mode {
	/* Best GPS performance. */
	GPS_POWER_MODE_DISABLED,

	/* Lower power, without significant GPS performance degradation. */
	GPS_POWER_MODE_PERFORMANCE,

	/* Lowest power option, with acceptable GPS performance. */
	GPS_POWER_MODE_SAVE,
};

/* GPS search configuration. */
struct gps_config {
	/* GPS navigation mode, @ref enum gps_nav_mode. */
	enum gps_nav_mode nav_mode;

	/* Power mode, @ref enum gps_power_mode. */
	enum gps_power_mode power_mode;

	/* Interval, in seconds, at which to start GPS search. The value is
	 * ignored outside periodic mode. Minimum accepted value is 10 seconds.
	 */
	u32_t interval;

	/* Time to search for fix before giving up. If used in periodic mode,
	 * the timeout repeats every interval. K_FOREVER or 0 indicate that
	 * the GPS  will search until it gets a valid PVT estimate, except in
	 * continuous mode, where it will stay on until explicitly stopped
	 * also in case of valid PVT.
	 */
	s32_t timeout;

	/* Delete stored assistance data before starting GPS search. */
	bool delete_agps_data;
};

/* Flags indicating which AGPS assistance data set is written to the GPS module.
 */
enum gps_agps_type {
	GPS_AGPS_UTC_PARAMETERS			= 1,
	GPS_AGPS_EPHEMERIDES			= 2,
	GPS_AGPS_ALMANAC			= 3,
	GPS_AGPS_KLOBUCHAR_CORRECTION		= 4,
	GPS_AGPS_NEQUICK_CORRECTION		= 5,
	GPS_AGPS_GPS_SYSTEM_CLOCK_AND_TOWS	= 7,
	GPS_AGPS_LOCATION			= 8,
	GPS_AGPS_INTEGRITY			= 9,
};

struct gps_agps_request {
	u32_t sv_mask_ephe;	/* Bit mask indicating the satellite PRNs for
				 * which the assistance GPS ephemeris data is
				 * needed.
				 */
	u32_t sv_mask_alm;	/* Bit mask indicating the satellite PRNs for
				 * which the assistance GPS almanac data is
				 * needed.
				 */
	u8_t utc:1;		/* GPS UTC parameters. */
	u8_t klobuchar:1;	/* Klobuchar parameters. */
	u8_t nequick:1;		/* NeQuick parameters. */
	u8_t system_time_tow:1;	/* GPS system time and SV TOWs. */
	u8_t position:1;	/* Position assistance parameters. */
	u8_t integrity:1;	/* Integrity assistance parameters. */
};

/**
 * @brief GPS event types.
 */
enum gps_event_type {
	GPS_EVT_SEARCH_STARTED,
	GPS_EVT_SEARCH_STOPPED,
	GPS_EVT_SEARCH_TIMEOUT,
	GPS_EVT_PVT,
	GPS_EVT_PVT_FIX,
	GPS_EVT_NMEA,
	GPS_EVT_NMEA_FIX,
	GPS_EVT_OPERATION_BLOCKED,
	GPS_EVT_OPERATION_UNBLOCKED,
	GPS_EVT_AGPS_DATA_NEEDED,
	GPS_EVT_ERROR,
};

/**
 * @brief GPS errors.
 */
enum gps_error {
	GPS_ERROR_GPS_DISABLED,
};

struct gps_event {
	enum gps_event_type type;
	union {
		struct gps_pvt pvt;
		struct gps_nmea nmea;
		struct gps_agps_request agps_request;
		enum gps_error error;
	};
};

/**
 * @typedef gps_evt_handler_t
 * @brief Callback API on GPS event
 *
 * @param dev Pointer to GPS device
 * @param evt Pointer to event data
 */
typedef void (*gps_event_handler_t)(struct device *dev,
				    struct gps_event *evt);

/**
 * @typedef gps_start_t
 * @brief Callback API for starting GPS operation.
 *
 * See gps_start() for argument description
 */
typedef int (*gps_start_t)(struct device *dev, struct gps_config *cfg);

/**
 * @typedef gps_stop_t
 * @brief Callback API for stopping GPS operation.
 *
 * See gps_stop() for argument description
 */
typedef int (*gps_stop_t)(struct device *dev);

/**
 * @typedef gps_write_t
 * @brief Callback API for writing to A-GPS data to GPS module.
 *
 * See gps_write() for argument description
 */
typedef int (*gps_agps_write_t)(struct device *dev, enum gps_agps_type type,
				void *data, size_t data_len);

/**
 * @typedef gps_init_t
 * @brief Callback API for initializing GPS device.
 *
 * See gps_init() for argument description
 */
typedef int (*gps_init_t)(struct device *dev, gps_event_handler_t handler);

/**
 * @typedef gps_deinit_t
 * @brief Callback API for deinitializing GPS device.
 *
 * See gps_deinit() for argument description
 */
typedef int (*gps_deinit_t)(struct device *dev);

/**
 * @brief GPS driver API
 *
 * This is the API all GPS drivers must expose.
 */
struct gps_driver_api {
	gps_start_t start;
	gps_stop_t stop;
	gps_agps_write_t agps_write;
	gps_init_t init;
	gps_deinit_t deinit;
};

/**
 * @brief Function to start GPS operation.
 *
 * @param dev Pointer to GPS device
 * @param cfg Pointer to GPS configuration.
 */
static inline int gps_start(struct device *dev, struct gps_config *cfg)
{
	struct gps_driver_api *api;

	if ((dev == NULL) || (cfg == NULL)) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->driver_api;

	if (api->start == NULL) {
		return -ENOTSUP;
	}

	return api->start(dev, cfg);
}

/**
 * @brief Function to stop GPS operation.
 *
 * @param dev Pointer to GPS device
 */
static inline int gps_stop(struct device *dev)
{
	struct gps_driver_api *api;

	if (dev == NULL) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->driver_api;

	if (api->stop == NULL) {
		return -ENOTSUP;
	}

	return api->stop(dev);
}

/**
 * @brief Function to write A-GPS data to GPS module.
 *
 * @param dev Pointer to GPS device
 * @param type A-GPS data type
 * @param data Pointer to A-GPS data
 *
 * @return Zero on success or (negative) error code otherwise.
 */
static inline int gps_agps_write(struct device *dev, enum gps_agps_type type,
				 void *data, size_t data_len)
{
	struct gps_driver_api *api;

	if ((data == NULL) || (dev == NULL)) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->driver_api;

	if (api->agps_write == NULL) {
		return -ENOTSUP;
	}

	return api->agps_write(dev, type, data, data_len);
}

/**
 * @brief Initializes GPS device.
 *
 * @param dev Pointer to GPS device.
 * @param handler Pointer to GPS event handler.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
static inline int gps_init(struct device *dev, gps_event_handler_t handler)
{
	struct gps_driver_api *api;

	if ((dev == NULL) || (handler == NULL)) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->driver_api;

	if (api->init == NULL) {
		return -ENOTSUP;
	}

	return api->init(dev, handler);
}

/**
 * @brief Deinitializes GPS device.
 *
 * @param dev Pointer to GPS device.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
static inline int gps_deinit(struct device *dev)
{
	struct gps_driver_api *api;

	if (dev == NULL) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->driver_api;

	if (api->deinit == NULL) {
		return -ENOTSUP;
	}

	return api->deinit(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_GPS_H_ */
