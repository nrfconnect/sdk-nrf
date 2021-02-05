/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
