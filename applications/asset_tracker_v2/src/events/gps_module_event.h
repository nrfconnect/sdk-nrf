/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _GPS_MODULE_EVENT_H_
#define _GPS_MODULE_EVENT_H_

/**
 * @brief GPS module event
 * @defgroup gps_module_event GPS module event
 * @{
 */

#include <drivers/gps.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NMEA_MAX_LEN 83

/** @brief GPS event types submitted by GPS module. */
enum gps_module_event_type {
	GPS_EVT_DATA_READY,
	GPS_EVT_TIMEOUT,
	GPS_EVT_ACTIVE,
	GPS_EVT_INACTIVE,
	GPS_EVT_SHUTDOWN_READY,
	GPS_EVT_AGPS_NEEDED,
	GPS_EVT_ERROR_CODE,
};

struct gps_module_pvt {
	double longitude;
	double latitude;
	float altitude;
	float accuracy;
	float speed;
	float heading;
};

enum gps_module_format {
	GPS_MODULE_DATA_FORMAT_INVALID,
	GPS_MODULE_DATA_FORMAT_PVT,
	GPS_MODULE_DATA_FORMAT_NMEA
};

struct gps_module_data {
	union {
		struct gps_module_pvt pvt;
		char nmea[NMEA_MAX_LEN];
	};

	enum gps_module_format format;
	int64_t timestamp;
};

/** @brief GPS event. */
struct gps_module_event {
	struct event_header header;
	enum gps_module_event_type type;

	union {
		struct gps_module_data gps;
		struct gps_agps_request agps_request;
		/* Module ID, used when acknowledging shutdown requests. */
		uint32_t id;
		int err;
	} data;
};

EVENT_TYPE_DECLARE(gps_module_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _GPS_MODULE_EVENT_H_ */
