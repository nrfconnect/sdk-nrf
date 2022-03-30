/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "test_events.h"

APPLICATION_EVENT_TYPE_DEFINE(test_start_event,
		  NULL,
		  NULL,
		  APPLICATION_EVENT_FLAGS_CREATE());

APPLICATION_EVENT_TYPE_DEFINE(test_end_event,
		  NULL,
		  NULL,
		  APPLICATION_EVENT_FLAGS_CREATE());
