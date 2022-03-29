/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "led_state_event.h"

APPLICATION_EVENT_TYPE_DEFINE(led_state_event,
		  NULL,
		  NULL,
		  APPLICATION_EVENT_FLAGS_CREATE(APPLICATION_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
