/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "control_event.h"


static void profile_control_event(struct log_event_buf *buf,
				  const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(control_event,
		  ENCODE(),
		  ENCODE(),
		  profile_control_event);

EVENT_TYPE_DEFINE(control_event,
		  true,
		  NULL,
		  &control_event_info);
