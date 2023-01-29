/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "name_style_events.h"

APP_EVENT_TYPE_DEFINE(event_name_style,
		      NULL,
		      NULL,
		      APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(event_name_style_break,
		      NULL,
		      NULL,
		      APP_EVENT_FLAGS_CREATE());
