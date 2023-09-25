/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LOCATION_MODULE_EVENT_H_
#define _LOCATION_MODULE_EVENT_H_

/**
 * @brief Location module event
 * @defgroup location_module_event Location module event
 * @{
 */

#include <nrf_modem_gnss.h>
#include <modem/lte_lc.h>
#include <net/wifi_location_common.h>
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif
#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Location event types submitted by location module. */
enum location_module_event_type {
	/** A valid GNSS location has been obtained and the data is ready to be used.
	 *  The event has associated payload of the type @ref location_module_data in
	 *  the event struct member ``data.location``.
	 *  All struct members within ``data.location`` contain valid data.
	 */
	LOCATION_MODULE_EVT_GNSS_DATA_READY,

	/** Location could not be obtained and will not be ready for this
	 *  sampling interval.
	 *  The event has no associated payload.
	 */
	LOCATION_MODULE_EVT_DATA_NOT_READY,

	/** Neighbor cell measurements and/or Wi-Fi access point information have been gathered
	 *  and the data is ready.
	 *  The event has associated payload of type @ref location_module_cloud_location in
	 *  the ``data.cloud_location`` member.
	 */
	LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY,

	/** The location search timed out without acquiring location.
	 *  The event has associated payload of the type ``struct location_module_data`` in
	 *  the event struct member ``data.location``.
	 *  The ``data.location`` struct member ``search_time`` contains valid data
	 *  and ``satellites_tracked`` contain valid data for GNSS.
	 */
	LOCATION_MODULE_EVT_TIMEOUT,

	/** An active location search has started. */
	LOCATION_MODULE_EVT_ACTIVE,

	/** A location search has stopped, either as a result of timeout, error
	 *  or acquired location.
	 */
	LOCATION_MODULE_EVT_INACTIVE,

	/** The module has been shut down gracefully.
	 *  The event has associated payload of the type ``uint32`` in the struct member
	 *  ``data.id`` that contains the module ID used when acknowledging a shutdown
	 *  request from the util module.
	 */
	LOCATION_MODULE_EVT_SHUTDOWN_READY,

	/** The location library has reported that it needs GPS assistance data.
	 *  The event has associated payload of the type ``struct nrf_modem_gnss_agnss_data_frame``
	 *  in the event struct member ``data.agps_request``, which contains the types
	 *  of A-GPS data that the modem needs.
	 */
	LOCATION_MODULE_EVT_AGPS_NEEDED,

	/** The location library has reported that it needs GPS prediction data.
	 *  The event has associated payload of the type ``struct gps_pgps_request``
	 *  in the event struct member ``data.pgps_request``, which specifies the P-GPS data need.
	 */
	LOCATION_MODULE_EVT_PGPS_NEEDED,

	/** An error has occurred, and data may have been lost.
	 *  The event has associated payload of the type ``int`` in the struct member
	 *  ``data.err``, that contains the original error code that triggered
	 *  the sending of this event.
	 */
	LOCATION_MODULE_EVT_ERROR_CODE,
};

/** @brief Position, velocity and time (PVT) data. */
struct location_module_pvt {
	/** Longitude in degrees. */
	double longitude;

	/** Latitude in degrees. */
	double latitude;

	/** Altitude above WGS-84 ellipsoid in meters. */
	float altitude;

	/** Position accuracy (2D 1-sigma) in meters. */
	float accuracy;

	/** Horizontal speed in m/s. */
	float speed;

	/** Heading of user movement in degrees. */
	float heading;
};

/** @brief Location module data for neighbor cells. */
struct location_module_neighbor_cells {
	/** Information about the current cell. */
	struct lte_lc_cells_info cell_data;
	/** Information about the neighbor cells. */
	struct lte_lc_ncell neighbor_cells[CONFIG_LTE_NEIGHBOR_CELLS_MAX];
	/** Uptime when the event was sent. */
	int64_t timestamp;
};

#if defined(CONFIG_LOCATION_METHOD_WIFI)
/** @brief Location module data for Wi-Fi access points. */
struct location_module_wifi_access_points {
	/** Access points found during scan. */
	struct wifi_scan_result ap_info[CONFIG_LOCATION_METHOD_WIFI_SCANNING_RESULTS_MAX_CNT];
	/** The number of access points found during scan. */
	uint16_t cnt;
	/** Uptime when the event was sent. */
	int64_t timestamp;
};
#endif

/** @brief Location module data for associated payload for event of
 *         LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY type.
 */
struct location_module_cloud_location {
	/** Neighbor cell information is valid. */
	bool neighbor_cells_valid;
	/** Neighbor cells. */
	struct location_module_neighbor_cells neighbor_cells;
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	/** Wi-Fi access point information is valid. */
	bool wifi_access_points_valid;
	/** Wi-Fi access points. */
	struct location_module_wifi_access_points wifi_access_points;
#endif
	/** Uptime when the event was sent. */
	int64_t timestamp;
};

/** @brief Location module data for associated payload for events of LOCATION_MODULE_EVT_TIMEOUT and
 *	   LOCATION_MODULE_EVT_GNSS_DATA_READY types.
 */
struct location_module_data {
	/** Location data in the form of a PVT structure. */
	struct location_module_pvt pvt;

	/** Number of satellites tracked. */
	uint8_t satellites_tracked;

	/** Time when the search was initiated until fix or timeout occurred. */
	uint32_t search_time;

	/** Uptime when location was sampled. */
	int64_t timestamp;
};

/** @brief Location module event. */
struct location_module_event {
	/** Location module application event header. */
	struct app_event_header header;
	/** Location module event type. */
	enum location_module_event_type type;

	/** Data for different events. */
	union {
		/** Data for event LOCATION_MODULE_EVT_GNSS_DATA_READY. */
		struct location_module_data location;
		/** Data for event LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY. */
		struct location_module_cloud_location cloud_location;
		/** Data for event LOCATION_MODULE_EVT_AGPS_NEEDED. */
		struct nrf_modem_gnss_agnss_data_frame agps_request;
#if defined(CONFIG_NRF_CLOUD_PGPS)
		/** Data for event LOCATION_MODULE_EVT_PGPS_NEEDED. */
		struct gps_pgps_request pgps_request;
#endif
		/* Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		/** Cause of the error for event LOCATION_MODULE_EVT_ERROR_CODE. */
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(location_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LOCATION_MODULE_EVENT_H_ */
