/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdio.h>
#include "five_sec_event.h"


static void profile_five_sec_event(struct log_event_buf *buf,
				  const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(five_sec_event,
		  ENCODE(),
		  ENCODE(),
		  profile_five_sec_event);

EVENT_TYPE_DEFINE(five_sec_event,
		  true,
		  NULL,
		  &five_sec_event_info);
