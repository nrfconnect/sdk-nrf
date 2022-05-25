/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/power_event.h>

APP_EVENT_TYPE_DEFINE(power_down_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_POWER_DOWN_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

APP_EVENT_TYPE_DEFINE(wake_up_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_WAKE_UP_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
