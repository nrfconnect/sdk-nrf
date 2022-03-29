/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "gnss_pvt_event.h"

#include <stdio.h>

static void log_gnss_event(const struct application_event_header *aeh)
{
	struct gnss_pvt_event *event = cast_gnss_pvt_event(aeh);

	APPLICATION_EVENT_MANAGER_LOG(aeh, "gnss_pvt_event lat: %lf long: %lf alt: %lf",
			  event->pvt.latitude, event->pvt.longitude,
			  event->pvt.altitude);
}

APPLICATION_EVENT_TYPE_DEFINE(gnss_pvt_event,
			      log_gnss_event,
			      NULL,
			      APPLICATION_EVENT_FLAGS_CREATE());
