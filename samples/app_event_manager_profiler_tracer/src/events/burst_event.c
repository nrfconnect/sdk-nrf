/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include "burst_event.h"


static void profile_burst_event(struct log_event_buf *buf,
				  const struct app_event_header *aeh)
{
}

APP_EVENT_INFO_DEFINE(burst_event,
		  ENCODE(),
		  ENCODE(),
		  profile_burst_event);

APP_EVENT_TYPE_DEFINE(burst_event,
		  NULL,
		  &burst_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
