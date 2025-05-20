/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "simple_events.h"

APP_EVENT_TYPE_DEFINE(simple_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(simple_ping_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(simple_pong_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE());
