/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <sys/util.h>
#include <stdio.h>

#include <caf/events/net_state_event.h>


static const char * const state_name[NET_STATE_COUNT] = {
	[NET_STATE_DISABLED] = "NET_STATE_DISABLED",
	[NET_STATE_DISCONNECTED] = "NET_STATE_DISCONNECTED",
	[NET_STATE_CONNECTED] = "NET_STATE_CONNECTED"
};

static int log_net_state_event(const struct event_header *eh, char *buf,
			       size_t buf_len)
{
	const struct net_state_event *event = cast_net_state_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == NET_STATE_COUNT,
		     "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < NET_STATE_COUNT);

	EVENT_MANAGER_LOG(eh, "id=%p %s", event->id,
			state_name[event->state]);
	return 0;
}

static void profile_net_state_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	const struct net_state_event *event = cast_net_state_event(eh);

	profiler_log_encode_uint32(buf, (uint32_t)event->id);
	profiler_log_encode_uint8(buf, event->state);
}

EVENT_INFO_DEFINE(net_state_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8),
		  ENCODE("conn_id", "state"),
		  profile_net_state_event);

EVENT_TYPE_DEFINE(net_state_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_NET_STATE_EVENTS),
		  log_net_state_event,
		  &net_state_event_info);
