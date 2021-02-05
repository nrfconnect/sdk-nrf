/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "config_event.h"

static const char * const status_name[] = {
#define X(name) STRINGIFY(name),
	CONFIG_STATUS_LIST
#undef X
};

static int log_config_event(const struct event_header *eh, char *buf,
			    size_t buf_len)
{
	const struct config_event *event = cast_config_event(eh);

	__ASSERT_NO_MSG(event->status < ARRAY_SIZE(status_name));

	return snprintf(buf, buf_len, "%s %s rcpt: %02x id: %02x",
			status_name[event->status],
			event->is_request ? "req" : "rsp",
			event->recipient,
			event->event_id);
}

EVENT_TYPE_DEFINE(config_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_CONFIG_EVENT),
		  log_config_event,
		  NULL);
