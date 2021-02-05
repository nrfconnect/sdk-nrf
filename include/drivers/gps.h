/**
 * @file gps.h
 *
 * @brief Public APIs for the GPS driver.
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
#define GPS_SOCKET_NOT_PROVIDED		0

struct gps_nmea {
	char buf[GPS_NMEA_SENTENCE_MAX_LENGTH];
	uint8_t len;
};

struct gps_datetime {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t seconds;
	uint16_t ms;
};

struct gps_sv {
	uint16_t sv;		/**< SV number 1...32 for GPS. */
	uint16_t cn0;		/**< 0.1 dB/Hz. */
	int16_t elevation;	/**< SV elevation angle in degrees. */
	int16_t azimuth;		/**< SV azimuth angle in degrees. */
	uint8_t signal;		/**< Signal type. 0: invalid, 1: GPS L1C/A. */
	uint8_t in_fix:1;		/**< Satellite used in fix calculation. */
	uint8_t unhealthy:1;	/**< Satellite is marked as unhealthy. */
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

enum gps_use_case {
	/* Target best single cold start performance. */
	GPS_USE_CASE_SINGLE_COLD_START,

	/* Target best multiple hot starts performance. */
	GPS_USE_CASE_MULTIPLE_HOT_START,
};

enum gps_accuracy {
	/** Use normal accuracy thresholds for producing GPS fix. */
	GPS_ACCURACY_NORMAL,

	/** Allow low accuracy fixes using 3 satellites.
	 *  Note that one of the two conditions must be met:
	 *	- Altitude, accurate within 10s of meters provided using
	 *	  gps_agps_write(). Valid for 24 hours.
	 *	- One fix using 5 or more satellites in the previous 24 hours,
	 *	  without the device rebooting in the meantime.
	 *
	 *  See the GNSS documentation for more details.
	 */
	GPS_ACCURACY_LOW,
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

	/* GPS use case, @ref enum gps_use_case. */
	enum gps_use_case use_case;

	/* Accuracy threshold for producing fix, @ref enum gps_accuracy. */
	enum gps_accuracy accuracy;

	/* Interval, in seconds, at which to start GPS search. The value is
	 * ignored outside periodic mode. Minimum accepted value is 10 seconds.
	 */
	uint32_t interval;

	/* Time to search for fix before giving up. If used in periodic mode,
	 * the timeout repeats every interval. K_FOREVER or 0 indicate that
	 * the GPS  will search until it gets a valid PVT estimate, except in
	 * continuous mode, where it will stay on until explicitly stopped
	 * also in case of valid PVT.
	 */
	int32_t timeout;

	/* Delete stored assistance data before starting GPS search. */
	bool delete_agps_data;

	/* Give GPS priority in competition with other radio resource users.
	 * This may affect the operation of other protocols, such as LTE in the
	 * case of nRF9160.
	 */
	bool priority;
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
	uint32_t sv_mask_ephe;	/* Bit mask indicating the satellite PRNs for
				 * which the assistance GPS ephemeris data is
				 * needed.
				 */
	uint32_t sv_mask_alm;	/* Bit mask indicating the satellite PRNs for
				 * which the assistance GPS almanac data is
				 * needed.
				 */
	uint8_t utc:1;		/* GPS UTC parameters. */
	uint8_t klobuchar:1;	/* Klobuchar parameters. */
	uint8_t nequick:1;		/* NeQuick parameters. */
	uint8_t system_time_tow:1;	/* GPS system time and SV TOWs. */
	uint8_t position:1;	/* Position assistance parameters. */
	uint8_t integrity:1;	/* Integrity assistance parameters. */
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
 * @typedef gps_event_handler_t
 * @brief Callback API on GPS event
 *
 * @param dev Pointer to GPS device
 * @param evt Pointer to event data
 */
typedef void (*gps_event_handler_t)(const struct device *dev,
				    struct gps_event *evt);

/**
 * @typedef gps_start_t
 * @brief Callback API for starting GPS operation.
 *
 * See gps_start() for argument description
 */
typedef int (*gps_start_t)(const struct device *dev, struct gps_config *cfg);

/**
 * @typedef gps_stop_t
 * @brief Callback API for stopping GPS operation.
 *
 * See gps_stop() for argument description
 */
typedef int (*gps_stop_t)(const struct device *dev);

/**
 * @typedef gps_agps_write_t
 * @brief Callback API for writing to A-GPS data to GPS module.
 *
 * See gps_write() for argument description
 */
typedef int (*gps_agps_write_t)(const struct device *dev,
				enum gps_agps_type type,
				void *data, size_t data_len);

/**
 * @typedef gps_init_t
 * @brief Callback API for initializing GPS device.
 *
 * See gps_init() for argument description
 */
typedef int (*gps_init_t)(const struct device *dev,
			  gps_event_handler_t handler);

/**
 * @typedef gps_deinit_t
 * @brief Callback API for deinitializing GPS device.
 *
 * See gps_deinit() for argument description
 */
typedef int (*gps_deinit_t)(const struct device *dev);

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
 * If gps is already running a call to this function will
 * restart the gps.
 *
 * @param dev Pointer to GPS device
 * @param cfg Pointer to GPS configuration.
 */
static inline int gps_start(const struct device *dev, struct gps_config *cfg)
{
	struct gps_driver_api *api;

	if ((dev == NULL) || (cfg == NULL)) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->api;

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
static inline int gps_stop(const struct device *dev)
{
	struct gps_driver_api *api;

	if (dev == NULL) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->api;

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
 * @param data_len Length of @p data
 *
 * @return Zero on success or (negative) error code otherwise.
 */
static inline int gps_agps_write(const struct device *dev,
				 enum gps_agps_type type,
				 void *data, size_t data_len)
{
	struct gps_driver_api *api;

	if ((data == NULL) || (dev == NULL)) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->api;

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
static inline int gps_init(const struct device *dev,
			   gps_event_handler_t handler)
{
	struct gps_driver_api *api;

	if ((dev == NULL) || (handler == NULL)) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->api;

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
static inline int gps_deinit(const struct device *dev)
{
	struct gps_driver_api *api;

	if (dev == NULL) {
		return -EINVAL;
	}

	api = (struct gps_driver_api *)dev->api;

	if (api->deinit == NULL) {
		return -ENOTSUP;
	}

	return api->deinit(dev);
}

/**
 * @brief Function to request A-GPS data.
 *
 * @param request Assistance data to request from A-GPS service.
 * @param socket GPS socket to which assistance data will be written
 *               when it's received from the selected A-GPS service.
 *               If zero the GPS driver will be used to write the data instead
 *               of directly to the provided socket.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int gps_agps_request(struct gps_agps_request request, int socket);

/**
 * @brief Processes A-GPS data.
 *
 * @param buf Pointer to A-GPS data.
 * @param len Buffer size of data to be processed.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int gps_process_agps_data(const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_GPS_H_ */
