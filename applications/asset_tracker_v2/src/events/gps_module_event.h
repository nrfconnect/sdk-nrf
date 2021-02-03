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

struct gps_module_data {
	int64_t timestamp;
	double longitude;
	double latitude;
	float altitude;
	float accuracy;
	float speed;
	float heading;
};

/** @brief GPS event. */
struct gps_module_event {
	struct event_header header;
	enum gps_module_event_type type;

	union {
		struct gps_module_data gps;
		struct gps_agps_request agps_request;
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
