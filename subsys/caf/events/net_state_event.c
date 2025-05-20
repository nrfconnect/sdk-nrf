/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

#include <caf/events/net_state_event.h>


static const char * const state_name[NET_STATE_COUNT] = {
	[NET_STATE_DISABLED] = "NET_STATE_DISABLED",
	[NET_STATE_DISCONNECTED] = "NET_STATE_DISCONNECTED",
	[NET_STATE_CONNECTED] = "NET_STATE_CONNECTED"
};

static void log_net_state_event(const struct app_event_header *aeh)
{
	const struct net_state_event *event = cast_net_state_event(aeh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == NET_STATE_COUNT,
		     "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < NET_STATE_COUNT);

	APP_EVENT_MANAGER_LOG(aeh, "id=%p %s", event->id,
			state_name[event->state]);
}

static void profile_net_state_event(struct log_event_buf *buf,
				    const struct app_event_header *aeh)
{
	const struct net_state_event *event = cast_net_state_event(aeh);

	nrf_profiler_log_encode_uint32(buf, (uint32_t)event->id);
	nrf_profiler_log_encode_uint8(buf, event->state);
}

APP_EVENT_INFO_DEFINE(net_state_event,
		  ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U8),
		  ENCODE("conn_id", "state"),
		  profile_net_state_event);

APP_EVENT_TYPE_DEFINE(net_state_event,
		  log_net_state_event,
		  &net_state_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_NET_STATE_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
