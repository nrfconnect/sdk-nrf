/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ack_event.h"


static void profile_ack_event(struct log_event_buf *buf,
			      const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(ack_event,
		  ENCODE(),
		  ENCODE(),
		  profile_ack_event);

EVENT_TYPE_DEFINE(ack_event,
		  true,
		  NULL,
		  &ack_event_info);
