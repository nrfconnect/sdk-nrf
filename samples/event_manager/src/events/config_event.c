/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "config_event.h"


static void profile_config_event(struct log_event_buf *buf,
				 const struct event_header *eh)
{
	struct config_event *event = cast_config_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->init_value1);
}

static int log_config_event(const struct event_header *eh, char *buf,
			    size_t buf_len)
{
	struct config_event *event = cast_config_event(eh);

	return snprintf(buf, buf_len, "init_val_1=%d", event->init_value1);
}

EVENT_INFO_DEFINE(config_event,
		  ENCODE(PROFILER_ARG_S8),
		  ENCODE("init_val_1"),
		  profile_config_event);

EVENT_TYPE_DEFINE(config_event,
		  true,
		  log_config_event,
		  &config_event_info);
