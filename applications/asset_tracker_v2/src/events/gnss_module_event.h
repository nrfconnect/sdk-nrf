/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _GNSS_MODULE_EVENT_H_
#define _GNSS_MODULE_EVENT_H_

/**
 * @brief GNSS module event
 * @defgroup gnss_module_event GNSS module event
 * @{
 */

#include <nrf_modem_gnss.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NMEA_MAX_LEN 83

/** @brief GNSS event types submitted by GNSS module. */
enum gnss_module_event_type {
	/** A valid GNSS fix has been obtained and the data is ready to be used.
	 *
	 *  The event has associated payload of the type ``struct gnss_module_data`` in
	 *  the event struct member ``data.gnss``.
	 *  All struct members within `data.gnss`` contain valid data.
	 *
	 *  The format of the location data from the GNSS fix is specified in the
	 *  event struct member ``data.gnss.format`` and decides how you can access
	 *  the anonymous union containing the location data.
	 *
	 *  If the format is ``GNSS_MODULE_DATA_FORMAT_PVT``, the PVT data is available in the
	 *  event struct member ``data.gnss.pvt``.
	 *
	 *  If the format is ``GNSS_MODULE_DATA_FORMAT_NMEA``, the NMEA string is available in the
	 *  event struct member ``data.gnss.nmea``.
	 */
	GNSS_EVT_DATA_READY,

	/** The GNSS search timed out without acquiring a fix.
	 *
	 *  The event has associated payload of the type ``struct gnss_module_data`` in
	 *  the event struct member ``data.gnss``.
	 *  The ``data.gnss`` struct members ``search_time`` and ``data.satellites_tracked``
	 *  contain valid data.
	 */
	GNSS_EVT_TIMEOUT,

	/** An active GNSS search has started. */
	GNSS_EVT_ACTIVE,

	/** A GNSS search has stopped, either as a result of timeout or acquired fix. */
	GNSS_EVT_INACTIVE,

	/** The module has been shut down gracefully.
	 *  The event has associated payload of the type ``uint32`` in the struct member
	 *  ``data.id``, which contains the module ID used when acknowledging a shutdown
	 *  request from the util module.
	 */
	GNSS_EVT_SHUTDOWN_READY,

	/** The modem has reported that it needs GPS assistance data.
	 *  The event has associated payload of the type ``struct nrf_modem_gnss_agps_data_frame``
	 *  in the event struct member ``data.agps_request``, which contains the types
	 *  of A-GPS data that the modem needs.
	 */
	GNSS_EVT_AGPS_NEEDED,

	/** An error has occurred, and data may have been lost.
	 *  The event has associated payload of the type ``int`` in the struct member
	 *  ``data.err``, which contains the original error code from that triggered
	 *  the sending of this event.
	 *
	 */
	GNSS_EVT_ERROR_CODE,
};

/** @brief Position, velocity and time (PVT) data. */
struct gnss_module_pvt {
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

/** @brief GNSS data format for associated payload of GNSS_EVT_DATA_READY event type. */
enum gnss_module_format {
	GNSS_MODULE_DATA_FORMAT_INVALID,
	GNSS_MODULE_DATA_FORMAT_PVT,
	GNSS_MODULE_DATA_FORMAT_NMEA
};

/** @brief GNSS module data for associated payload for events of GNSS_EVT_TIMEOUT and
 *	   GNSS_EVT_DATA_READY types.
 */
struct gnss_module_data {
	/** Location data in the form of a PVT structure or an NMEA GGA sentence. */
	union {
		struct gnss_module_pvt pvt;
		char nmea[NMEA_MAX_LEN];
	};


	uint8_t satellites_tracked;

	/** Time from the search was initiated until fix or timeout occurred. */
	uint32_t search_time;
	enum gnss_module_format format;
	int64_t timestamp;
};

/** @brief GNSSS module event. */
struct gnss_module_event {
	struct app_event_header header;
	enum gnss_module_event_type type;

	union {
		struct gnss_module_data gnss;
		struct nrf_modem_gnss_agps_data_frame agps_request;

		/* Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		int err;
	} data;
};

APP_EVENT_TYPE_DECLARE(gnss_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _GNSS_MODULE_EVENT_H_ */
