/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "config_event.h"

static int log_config_event(const struct event_header *eh, char *buf,
			   size_t buf_len)
{
	const struct config_event *event = cast_config_event(eh);

	return snprintf(buf, buf_len, "id=%u", event->id);
}

static void profile_config_event(struct log_event_buf *buf,
				 const struct event_header *eh)
{
	const struct config_event *event = cast_config_event(eh);

	profiler_log_encode_u32(buf, event->id);
}

static int log_config_fetch_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	struct config_fetch_event *event = cast_config_fetch_event(eh);

	return snprintf(buf, buf_len, "id=%u", event->id);
}

static void profile_config_fetch_event(struct log_event_buf *buf,
				       const struct event_header *eh)
{
	struct config_fetch_event *event = cast_config_fetch_event(eh);

	profiler_log_encode_u32(buf, event->id);
}

static int log_config_fetch_request_event(const struct event_header *eh,
					  char *buf,
					  size_t buf_len)
{
	struct config_fetch_request_event *event =
		cast_config_fetch_request_event(eh);

	return snprintf(buf, buf_len, "id=%u", event->id);
}

static void profile_config_fetch_request_event(struct log_event_buf *buf,
					       const struct event_header *eh)
{
	struct config_fetch_request_event *event =
		cast_config_fetch_request_event(eh);

	profiler_log_encode_u32(buf, event->id);
}

EVENT_INFO_DEFINE(config_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("id"),
		  profile_config_event);

EVENT_TYPE_DEFINE(config_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_CONFIG_EVENT),
		  log_config_event,
		  &config_event_info);

EVENT_INFO_DEFINE(config_fetch_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("id"),
		  profile_config_fetch_event);

EVENT_TYPE_DEFINE(config_fetch_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_CONFIG_EVENT),
		  log_config_fetch_event,
		  &config_fetch_event_info);

EVENT_INFO_DEFINE(config_fetch_request_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("id"),
		  profile_config_fetch_request_event);

EVENT_TYPE_DEFINE(config_fetch_request_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_CONFIG_EVENT),
		  log_config_fetch_request_event,
		  &config_fetch_request_event_info);

EVENT_TYPE_DEFINE(config_forward_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_CONFIG_EVENT),
		  NULL,
		  NULL);

EVENT_TYPE_DEFINE(config_forward_get_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_CONFIG_EVENT),
		  NULL,
		  NULL);

EVENT_TYPE_DEFINE(config_forwarded_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_CONFIG_EVENT),
		  NULL,
		  NULL);
