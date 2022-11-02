/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "accel_event.h"

static void log_accel_event(const struct app_event_header *aeh)
{
	struct accel_event *event = cast_accel_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh,
		"Accelerometer event: x = %d.%06d, y = %d.%06d, z = %d.%06d.",
		event->data.x.val1, event->data.x.val2, event->data.y.val1, event->data.y.val2,
		event->data.z.val1, event->data.z.val2);
}

APP_EVENT_TYPE_DEFINE(accel_event, log_accel_event, NULL, APP_EVENT_FLAGS_CREATE());
