/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ml_app_mode_event.h"

static const char * const ml_app_mode_name[] = {
	[ML_APP_MODE_MODEL_RUNNING] = "MODEL_RUNNING",
	[ML_APP_MODE_DATA_FORWARDING] = "DATA_FORWARDING",
};

static int log_ml_app_mode_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	const struct ml_app_mode_event *event = cast_ml_app_mode_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(ml_app_mode_name) == ML_APP_MODE_COUNT);
	__ASSERT_NO_MSG(event->mode < ML_APP_MODE_COUNT);
	__ASSERT_NO_MSG(ml_app_mode_name[event->mode] != NULL);

	return snprintf(buf, buf_len, "state: %s", ml_app_mode_name[event->mode]);
}

EVENT_TYPE_DEFINE(ml_app_mode_event,
		  IS_ENABLED(CONFIG_ML_APP_INIT_LOG_ML_APP_MODE_EVENTS),
		  log_ml_app_mode_event,
		  NULL);
