/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ack_event.h"


static void profile_ack_event(struct log_event_buf *buf,
			      const struct application_event_header *aeh)
{
}

EVENT_INFO_DEFINE(ack_event,
		  ENCODE(),
		  ENCODE(),
		  profile_ack_event);

APPLICATION_EVENT_TYPE_DEFINE(ack_event,
		  NULL,
		  &ack_event_info,
		  APPLICATION_EVENT_FLAGS_CREATE(APPLICATION_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
