/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "passkey_event.h"


static void log_passkey_input_event(const struct event_header *eh)
{
	const struct passkey_input_event *event = cast_passkey_input_event(eh);

	EVENT_MANAGER_LOG(eh, "passkey: %" PRIu32, event->passkey);
}

static void profile_passkey_input_event(struct log_event_buf *buf,
					const struct event_header *eh)
{
	const struct passkey_input_event *event = cast_passkey_input_event(eh);

	profiler_log_encode_uint32(buf, event->passkey);
}

EVENT_INFO_DEFINE(passkey_input_event,
		  ENCODE(PROFILER_ARG_U32),
		  ENCODE("passkey"),
		  profile_passkey_input_event);

EVENT_TYPE_DEFINE(passkey_input_event,
		  log_passkey_input_event,
		  &passkey_input_event_info,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_PASSKEY_EVENT,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));


static void log_passkey_req_event(const struct event_header *eh)
{
	const struct passkey_req_event *event = cast_passkey_req_event(eh);

	EVENT_MANAGER_LOG(eh, "input %s", (event->active) ?
						  ("started") : ("stopped"));
}

static void profile_passkey_req_event(struct log_event_buf *buf,
				      const struct event_header *eh)
{
	const struct passkey_req_event *event = cast_passkey_req_event(eh);

	profiler_log_encode_uint8(buf, event->active);
}

EVENT_INFO_DEFINE(passkey_req_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("active"),
		  profile_passkey_req_event);

EVENT_TYPE_DEFINE(passkey_req_event,
		  log_passkey_req_event,
		  &passkey_req_event_info,
		  EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_DESKTOP_INIT_LOG_PASSKEY_EVENT,
				(EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
