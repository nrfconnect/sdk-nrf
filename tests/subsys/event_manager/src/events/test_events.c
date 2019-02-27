/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "test_events.h"


EVENT_TYPE_DEFINE(test_start_event,
		  true,
		  NULL,
		  NULL);

EVENT_TYPE_DEFINE(test_end_event,
		  true,
		  NULL,
		  NULL);
