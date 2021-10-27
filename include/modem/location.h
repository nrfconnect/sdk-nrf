/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file location.h
 * @brief Public APIs for the Location library.
 * @defgroup location Location library
 * @{
 */

#ifndef LOCATION_H_
#define LOCATION_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
#include <nrf_modem_gnss.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LOC_MAX_METHODS 3

/** Positioning methods. */
enum loc_method {
	/** LTE cellular positioning. */
	LOC_METHOD_CELLULAR = 1,
	/** Global Navigation Satellite System (GNSS). */
	LOC_METHOD_GNSS,
	/** WiFi positioning. */
	LOC_METHOD_WIFI,
};

/** Event IDs. */
enum loc_event_id {
	/** Location update. */
	LOC_EVT_LOCATION,
	/** Getting location timed out. */
	LOC_EVT_TIMEOUT,
	/** An error occurred when trying to get the location. */
	LOC_EVT_ERROR,
	/** GNSS is requesting A-GPS or P-GPS assistance data. */
	LOC_EVT_GNSS_ASSISTANCE_REQUEST
};

/** Positioning accuracy enumerator. */
enum loc_accuracy {
	/** Allow lower accuracy to conserve power. */
	LOC_ACCURACY_LOW,
	/** Normal accuracy. */
	LOC_ACCURACY_NORMAL,
	/** Allow higher power consumption to improve accuracy. */
	LOC_ACCURACY_HIGH
};

/** Positioning service. */
enum loc_service {
	/**
	 * @brief Use any location service that has been configured to be available.
	 *
	 * @details This is especially useful when only one service is configured but can be used
	 * even if many are available.
	 */
	LOC_SERVICE_ANY,
	/** nRF Cloud location service. */
	LOC_SERVICE_NRF_CLOUD,
	/**Here location service. */
	LOC_SERVICE_HERE,
	/**Skyhook location service. */
	LOC_SERVICE_SKYHOOK
};

/** Date and time (UTC). */
struct loc_datetime {
	/** True if date and time are valid, false if not. */
	bool valid;
	/** 4-digit representation (Gregorian calendar). */
	uint16_t year;
	/** 1...12 */
	uint8_t month;
	/** 1...31 */
	uint8_t day;
	/** 0...23 */
	uint8_t hour;
	/** 0...59 */
	uint8_t minute;
	/** 0...59 */
	uint8_t second;
	/** 0...999 */
	uint16_t ms;
};

/** Location data. */
struct loc_location {
	/** Geodetic latitude (deg) in WGS-84. */
	double latitude;
	/** Geodetic longitude (deg) in WGS-84. */
	double longitude;
	/** Location accuracy in meters (1-sigma). */
	float accuracy;
	/** Date and time (UTC). */
	struct loc_datetime datetime;
};

/** Location event data. */
struct loc_event_data {
	/** Event ID. */
	enum loc_event_id id;

	/** Used positioning method. */
	enum loc_method method;

	union {
		/** Current location, used with event LOC_EVT_LOCATION. */
		struct loc_location location;
#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
		/** A-GPS notification data frame used by GNSS to let the application know it
		 *  needs new assistance data.
		 */
		struct nrf_modem_gnss_agps_data_frame request;
#endif
	};
};

/** GNSS configuration. */
struct loc_gnss_config {
	/**
	 * @brief Timeout (in seconds), i.e. how long GNSS is allowed to run when trying to
	 * acquire a fix.
	 *
	 * @details Note that this is not real time as experienced by the user.
	 * Since GNSS cannot run while LTE is operating, the running time is
	 * counted from the instant when GNSS is allowed to start. If LTE power saving mode (PSM)
	 * is requested, GNSS waits until the modem enters PSM before starting up, thus maximizing
	 * uninterrupted operating window and minimizing power consumption.
	 */
	uint16_t timeout;

	/** Desired accuracy level. */
	enum loc_accuracy accuracy;

	/**
	 * @brief If accuracy is set to LOC_ACCURACY_HIGH, instead of using the first fix, GNSS
	 * is allowed to run for a longer time and produce the configured number fixes before the
	 * library outputs the current location.
	 *
	 * @details This typically improves the location accuracy. If accuracy is set to
	 * LOC_ACCURACY_NORMAL or LOC_ACCURACY_LOW this parameter has no effect.
	 */
	uint8_t num_consecutive_fixes;
};

/** LTE cellular positioning configuration. */
struct loc_cellular_config {
	/** Timeout (in seconds) on how long cellular positioning procedure can take. */
	uint16_t timeout;

	/** Used cellular positioning service. */
	enum loc_service service;
};

/** WiFi positioning configuration. */
struct loc_wifi_config {
	/** Timeout (in seconds) on how long WiFi positioning procedure can take. */
	uint16_t timeout;

	/** Used WiFi positioning service. */
	enum loc_service service;
};

/** Positioning method configuration. */
struct loc_method_config {
	/** Positioning method. */
	enum loc_method method;
	union {
		/** Configuration for LOC_METHOD_CELLULAR. */
		struct loc_cellular_config cellular;
		/** Configuration for LOC_METHOD_GNSS. */
		struct loc_gnss_config gnss;
		/** Configuration for LOC_METHOD_WIFI. */
		struct loc_wifi_config wifi;
	};
};

/** Location request configuration. */
struct loc_config {
	/** Number of location methods in 'methods'. */
	uint8_t methods_count;
	/**
	 * @brief Selected positioning methods and associated configurations in priority order.
	 *
	 * @details Index 0 has the highest priority. Number of methods is indicated in
	 * 'methods_count'.
	 */
	struct loc_method_config *methods;
	/**
	 * @brief Position update interval in seconds.
	 *
	 * @details Set to 0 for a single position update. For periodic position updates
	 * the valid range is 10...65535 seconds.
	 */
	uint16_t interval;
};

/**
 * @brief Event handler prototype.
 *
 * @param[in] event_data Event data.
 */
typedef void (*location_event_handler_t)(const struct loc_event_data *event_data);

/** @brief Initializes the library.
 *
 * @details Initializes the library and sets the event handler function.
 *
 * @param[in] event_handler Event handler function.
 *
 * @return 0 on success, or negative error code on failure.
 */
int location_init(location_event_handler_t event_handler);

/**
 * @brief Requests the current position or starts periodic position updates.
 *
 * @details Requests the current position using the given configuration. Depending on the
 *          configuration, a single position or periodic position updates are given. The results are
 *          delivered to the event handler function.
 *
 *          Periodic position updates can be stopped by calling location_cancel().
 *
 * @param[in] config Used configuration or NULL to get a single position update with
 *                   the default configuration.
 *
 * @return 0 on success, or negative error code on failure.
 */
int location_request(const struct loc_config *config);

/**
 * @brief Cancels periodic position updates.
 *
 * @return 0 on success, or negative error code on failure.
 */
int location_request_cancel(void);

/**
 * @brief Sets default values to given configuration.
 *
 * @details Given methods are set as list of methods for the given configuration.
 * Caller is responsible for allocating and deallocating memory for both configuration and
 * method list. 'methods' memory area must be kept allocated as long as the configuration is used.
 *
 * @param[inout] config Configuration which is supplied with default values.
 * @param[in] methods_count Number of methods to be stored in 'methods'.
 * @param[in] methods List of methods. This will be initialized to zero.
 */
void loc_config_defaults_set(struct loc_config *config, uint8_t methods_count,
			     struct loc_method_config *methods);

/**
 * @brief Sets default values for given method configuration based on given type.
 *
 * @details Intended use is that this is part of loc_config that has just been initialized, i.e.,
 * this function is called right after loc_config_defaults_set().
 *
 * @param[inout] method Method configuration which is supplied with default values.
 * @param[in] method_type Location method type.
 */
void loc_config_method_defaults_set(struct loc_method_config *method, enum loc_method method_type);

#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
/**
 * @brief Feed in A-GPS data to be processed by library.
 *
 * @details If A-GPS assistance data are received from a source other that nRF cloud, they can be
 * handed over to Location library using this function. Note that the data must be nevertheless
 * formatted similarly to nRF cloud A-GPS data, cf. nrf_cloud_agps_schema_v1.h.
 *
 * @param[in] buf Pointer to data received.
 * @param[in] buf_len Buffer size of data to be processed.
 *
 * @return 0 if successful, otherwise a (negative) error code.
 */
int location_agps_data_process(const char *buf, size_t buf_len);
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* LOCATION_H_ */
