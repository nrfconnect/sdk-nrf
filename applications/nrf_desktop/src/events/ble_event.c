/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <sys/util.h>
#include <stdio.h>

#include "ble_event.h"


static const char * const state_name[] = {
#define X(name) STRINGIFY(name),
	PEER_STATE_LIST
#undef X
};

static int log_ble_peer_event(const struct event_header *eh, char *buf,
			      size_t buf_len)
{
	const struct ble_peer_event *event = cast_ble_peer_event(eh);

	BUILD_ASSERT_MSG(ARRAY_SIZE(state_name) == PEER_STATE_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < PEER_STATE_COUNT);

	return snprintf(buf, buf_len, "id=%p %s", event->id,
			state_name[event->state]);
}

static void profile_ble_peer_event(struct log_event_buf *buf,
				   const struct event_header *eh)
{
	const struct ble_peer_event *event = cast_ble_peer_event(eh);

	profiler_log_encode_u32(buf, (u32_t)event->id);
	profiler_log_encode_u32(buf, event->state);
}

EVENT_INFO_DEFINE(ble_peer_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U32),
		  ENCODE("conn_id", "state"),
		  profile_ble_peer_event);

EVENT_TYPE_DEFINE(ble_peer_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BLE_PEER_EVENT),
		  log_ble_peer_event,
		  &ble_peer_event_info);

static int log_ble_peer_search_event(const struct event_header *eh, char *buf,
				     size_t buf_len)
{
	const struct ble_peer_search_event *event = cast_ble_peer_search_event(eh);

	return snprintf(buf, buf_len, "%sactive", (event->active)?(""):("in"));
}

static void profile_ble_peer_search_event(struct log_event_buf *buf,
					  const struct event_header *eh)
{
	const struct ble_peer_search_event *event = cast_ble_peer_search_event(eh);

	profiler_log_encode_u32(buf, (u32_t)event->active);
}

EVENT_INFO_DEFINE(ble_peer_search_event,
		  ENCODE(PROFILER_ARG_U32),
		  ENCODE("active"),
		  profile_ble_peer_search_event);

EVENT_TYPE_DEFINE(ble_peer_search_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BLE_PEER_SEARCH_EVENT),
		  log_ble_peer_search_event,
		  &ble_peer_search_event_info);


static const char * const op_name[] = {
#define X(name) STRINGIFY(name),
	PEER_OPERATION_LIST
#undef X
};

static int log_ble_peer_operation_event(const struct event_header *eh,
					char *buf, size_t buf_len)
{
	const struct ble_peer_operation_event *event =
		cast_ble_peer_operation_event(eh);

	BUILD_ASSERT_MSG(ARRAY_SIZE(op_name) == PEER_OPERATION_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->op < PEER_OPERATION_COUNT);

	return snprintf(buf, buf_len, "%s bt_app_id=%u bt_stack_id=%u",
			op_name[event->op],
			event->bt_app_id,
			event->bt_stack_id);
}

static void profile_ble_peer_operation_event(struct log_event_buf *buf,
					     const struct event_header *eh)
{
	const struct ble_peer_operation_event *event =
		cast_ble_peer_operation_event(eh);

	profiler_log_encode_u32(buf, event->op);
	profiler_log_encode_u32(buf, event->bt_app_id);
	profiler_log_encode_u32(buf, event->bt_stack_id);
}

EVENT_INFO_DEFINE(ble_peer_operation_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U32, PROFILER_ARG_U32),
		  ENCODE("operation", "bt_app_id", "bt_stack_id"),
		  profile_ble_peer_operation_event);

EVENT_TYPE_DEFINE(ble_peer_operation_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BLE_PEER_OPERATION_EVENT),
		  log_ble_peer_operation_event,
		  &ble_peer_operation_event_info);

EVENT_TYPE_DEFINE(ble_discovery_complete_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BLE_DISC_COMPLETE_EVENT),
		  NULL,
		  NULL);
