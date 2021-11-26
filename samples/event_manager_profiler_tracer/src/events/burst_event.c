/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include "burst_event.h"


static void profile_burst_event(struct log_event_buf *buf,
				  const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(burst_event,
		  ENCODE(),
		  ENCODE(),
		  profile_burst_event);

EVENT_TYPE_DEFINE(burst_event,
		  true,
		  NULL,
		  &burst_event_info);
