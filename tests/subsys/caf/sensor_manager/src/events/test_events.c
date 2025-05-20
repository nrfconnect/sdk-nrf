/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "test_events.h"

APP_EVENT_TYPE_DEFINE(test_start_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(test_end_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(test_initialization_done_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE());
