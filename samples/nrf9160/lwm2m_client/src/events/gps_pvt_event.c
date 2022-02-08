/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "gps_pvt_event.h"

#include <stdio.h>

static void log_gps_event(const struct event_header *eh)
{
	struct gps_pvt_event *event = cast_gps_pvt_event(eh);

	EVENT_MANAGER_LOG(eh, "gps_pvt_event lat: %d long: %d alt: %d",
			(int)event->pvt.latitude, (int)event->pvt.longitude,
			(int)event->pvt.altitude);
}

EVENT_TYPE_DEFINE(gps_pvt_event, false, log_gps_event, NULL);
