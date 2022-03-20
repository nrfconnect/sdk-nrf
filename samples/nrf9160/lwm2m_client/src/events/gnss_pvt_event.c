/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "gnss_pvt_event.h"

#include <stdio.h>

static void log_gnss_event(const struct event_header *eh)
{
	struct gnss_pvt_event *event = cast_gnss_pvt_event(eh);

	EVENT_MANAGER_LOG(eh, "gnss_pvt_event lat: %lf long: %lf alt: %lf",
			  event->pvt.latitude, event->pvt.longitude,
			  event->pvt.altitude);
}

EVENT_TYPE_DEFINE(gnss_pvt_event, false, log_gnss_event, NULL);
