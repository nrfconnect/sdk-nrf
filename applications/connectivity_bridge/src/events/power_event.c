/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "power_event.h"

APP_EVENT_TYPE_DEFINE(power_down_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_BRIDGE_LOG_POWER_DOWN_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
