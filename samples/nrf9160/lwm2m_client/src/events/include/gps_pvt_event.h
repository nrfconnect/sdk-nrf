/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef GPS_PVT_EVENT_H__
#define GPS_PVT_EVENT_H__

#include <zephyr.h>
#include <drivers/gps.h>
#include <event_manager.h>
#include <event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gps_pvt_event {
	struct event_header header;

	struct gps_pvt pvt;
};

EVENT_TYPE_DECLARE(gps_pvt_event);

#ifdef __cplusplus
}
#endif

#endif /* GPS_PVT_EVENT_H__ */
