/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "control_event.h"


static void profile_control_event(struct log_event_buf *buf,
				  const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(control_event,
		  ENCODE(),
		  ENCODE(),
		  profile_control_event);

APP_EVENT_TYPE_DEFINE(control_event,
		  NULL,
		  &control_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
