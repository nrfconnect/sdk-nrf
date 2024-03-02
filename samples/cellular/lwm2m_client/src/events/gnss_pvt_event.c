/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "gnss_pvt_event.h"

static void log_gnss_event(const struct app_event_header *aeh)
{
	struct gnss_pvt_event *event = cast_gnss_pvt_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "gnss_pvt_event lat: %lf long: %lf alt: %lf",
			  event->pvt.latitude, event->pvt.longitude,
			  (double)event->pvt.altitude);
}

APP_EVENT_TYPE_DEFINE(gnss_pvt_event,
			      log_gnss_event,
			      NULL,
			      APP_EVENT_FLAGS_CREATE());
