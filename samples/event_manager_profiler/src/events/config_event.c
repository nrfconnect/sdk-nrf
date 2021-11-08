/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "config_event.h"

static void profile_config_event(struct log_event_buf *buf,
				  const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(config_event,
		  ENCODE(),
		  ENCODE(),
		  profile_config_event);

EVENT_TYPE_DEFINE(config_event,
		  true,
		  NULL,
		  &config_event_info);
