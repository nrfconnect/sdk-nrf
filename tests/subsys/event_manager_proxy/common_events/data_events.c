/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "data_events.h"


APP_EVENT_TYPE_DEFINE(data_event,
	NULL,
	NULL,
	APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(data_response_event,
	NULL,
	NULL,
	APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(data_big_event,
	NULL,
	NULL,
	APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(data_big_response_event,
	NULL,
	NULL,
	APP_EVENT_FLAGS_CREATE());
