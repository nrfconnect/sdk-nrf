/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "power_event.h"

EVENT_TYPE_DEFINE(power_down_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_POWER_DOWN_EVENTS),
		  NULL,
		  NULL);

EVENT_TYPE_DEFINE(wake_up_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_WAKE_UP_EVENTS),
		  NULL,
		  NULL);
