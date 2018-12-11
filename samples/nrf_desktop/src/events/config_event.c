/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <stdio.h>
#include <misc/printk.h>

#include "config_event.h"

static int log_config_event(const struct event_header *eh, char *buf,
			   size_t buf_len)
{
	struct config_event *event = cast_config_event(eh);

	return snprintf(buf, buf_len, "id=%u", event->id);
}

static void profile_config_event(struct log_event_buf *buf,
				 const struct event_header *eh)
{
	struct config_event *event = cast_config_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->id);
}

EVENT_INFO_DEFINE(config_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("id"),
		  profile_config_event);

EVENT_TYPE_DEFINE(config_event, log_config_event, &config_event_info);
