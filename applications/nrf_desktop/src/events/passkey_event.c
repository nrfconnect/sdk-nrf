/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "passkey_event.h"


static int log_passkey_input_event(const struct event_header *eh, char *buf,
				   size_t buf_len)
{
	const struct passkey_input_event *event = cast_passkey_input_event(eh);

	return snprintf(buf, buf_len, "passkey: %" PRIu32, event->passkey);
}

static void profile_passkey_input_event(struct log_event_buf *buf,
					const struct event_header *eh)
{
	const struct passkey_input_event *event = cast_passkey_input_event(eh);

	profiler_log_encode_u32(buf, event->passkey);
}

EVENT_INFO_DEFINE(passkey_input_event,
		  ENCODE(PROFILER_ARG_U32),
		  ENCODE("passkey"),
		  profile_passkey_input_event);

EVENT_TYPE_DEFINE(passkey_input_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_PASSKEY_EVENT),
		  log_passkey_input_event,
		  &passkey_input_event_info);


static int log_passkey_req_event(const struct event_header *eh, char *buf,
				 size_t buf_len)
{
	const struct passkey_req_event *event = cast_passkey_req_event(eh);

	return snprintf(buf, buf_len, "input %s", (event->active) ?
						  ("started") : ("stopped"));
}

static void profile_passkey_req_event(struct log_event_buf *buf,
				      const struct event_header *eh)
{
	const struct passkey_req_event *event = cast_passkey_req_event(eh);

	profiler_log_encode_u32(buf, event->active);
}

EVENT_INFO_DEFINE(passkey_req_event,
		  ENCODE(PROFILER_ARG_U32),
		  ENCODE("active"),
		  profile_passkey_req_event);

EVENT_TYPE_DEFINE(passkey_req_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_PASSKEY_EVENT),
		  log_passkey_req_event,
		  &passkey_req_event_info);
