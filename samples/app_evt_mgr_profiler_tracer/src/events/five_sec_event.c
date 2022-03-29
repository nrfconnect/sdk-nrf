/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include "five_sec_event.h"


static void profile_five_sec_event(struct log_event_buf *buf,
				  const struct application_event_header *aeh)
{
}

EVENT_INFO_DEFINE(five_sec_event,
		  ENCODE(),
		  ENCODE(),
		  profile_five_sec_event);

APPLICATION_EVENT_TYPE_DEFINE(five_sec_event,
		  NULL,
		  &five_sec_event_info,
		  APPLICATION_EVENT_FLAGS_CREATE(APPLICATION_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
