/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <sys/util.h>
#include <stdio.h>

#include <caf/events/ble_common_event.h>


static const char * const state_name[] = {
	"DISCONNECTED",
	"DISCONNECTING",
	"CONNECTED",
	"SECURED",
	"CONN_FAILED"
};

static int log_ble_peer_event(const struct event_header *eh, char *buf,
			      size_t buf_len)
{
	const struct ble_peer_event *event = cast_ble_peer_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == PEER_STATE_COUNT, "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < PEER_STATE_COUNT);

	EVENT_MANAGER_LOG(eh, "id=%p %s", event->id,
			state_name[event->state]);
	return 0;
}

static void profile_ble_peer_event(struct log_event_buf *buf,
				   const struct event_header *eh)
{
	const struct ble_peer_event *event = cast_ble_peer_event(eh);

	profiler_log_encode_uint32(buf, (uint32_t)event->id);
	profiler_log_encode_uint8(buf, event->state);
}

EVENT_INFO_DEFINE(ble_peer_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8),
		  ENCODE("conn_id", "state"),
		  profile_ble_peer_event);

EVENT_TYPE_DEFINE(ble_peer_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_BLE_PEER_EVENTS),
		  log_ble_peer_event,
		  &ble_peer_event_info);

static int log_ble_peer_search_event(const struct event_header *eh, char *buf,
				     size_t buf_len)
{
	const struct ble_peer_search_event *event = cast_ble_peer_search_event(eh);

	EVENT_MANAGER_LOG(eh, "%sactive", (event->active)?(""):("in"));
	return 0;
}

static void profile_ble_peer_search_event(struct log_event_buf *buf,
					  const struct event_header *eh)
{
	const struct ble_peer_search_event *event = cast_ble_peer_search_event(eh);

	profiler_log_encode_uint8(buf, (uint8_t)event->active);
}

EVENT_INFO_DEFINE(ble_peer_search_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("active"),
		  profile_ble_peer_search_event);

EVENT_TYPE_DEFINE(ble_peer_search_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_BLE_PEER_SEARCH_EVENTS),
		  log_ble_peer_search_event,
		  &ble_peer_search_event_info);


static const char * const op_name[] = {
	"SELECT",
	"SELECTED",
	"SCAN_REQUEST",
	"ERASE",
	"ERASE_ADV",
	"ERASE_ADV_CANCEL",
	"ERASED",
	"CANCEL"
};

static int log_ble_peer_operation_event(const struct event_header *eh,
					char *buf, size_t buf_len)
{
	const struct ble_peer_operation_event *event =
		cast_ble_peer_operation_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(op_name) == PEER_OPERATION_COUNT, "Invalid number of elements");

	__ASSERT_NO_MSG(event->op < PEER_OPERATION_COUNT);

	EVENT_MANAGER_LOG(eh, "%s bt_app_id=%u bt_stack_id=%u",
			op_name[event->op],
			event->bt_app_id,
			event->bt_stack_id);
	return 0;
}

static void profile_ble_peer_operation_event(struct log_event_buf *buf,
					     const struct event_header *eh)
{
	const struct ble_peer_operation_event *event =
		cast_ble_peer_operation_event(eh);

	profiler_log_encode_uint8(buf, event->op);
	profiler_log_encode_uint8(buf, event->bt_app_id);
	profiler_log_encode_uint8(buf, event->bt_stack_id);
}

EVENT_INFO_DEFINE(ble_peer_operation_event,
		  ENCODE(PROFILER_ARG_U8, PROFILER_ARG_U8, PROFILER_ARG_U8),
		  ENCODE("operation", "bt_app_id", "bt_stack_id"),
		  profile_ble_peer_operation_event);

EVENT_TYPE_DEFINE(ble_peer_operation_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_BLE_PEER_OPERATION_EVENTS),
		  log_ble_peer_operation_event,
		  &ble_peer_operation_event_info);

static int log_ble_peer_conn_params_event(const struct event_header *eh,
					  char *buf, size_t buf_len)
{
	const struct ble_peer_conn_params_event *event =
		cast_ble_peer_conn_params_event(eh);

	EVENT_MANAGER_LOG(eh, "peer=%p min=%"PRIx16 " max=%"PRIx16
			" lat=%"PRIu16 " timeout=%" PRIu16 " (%s)",
			event->id, event->interval_min,	event->interval_max,
			event->latency, event->timeout,
			(event->updated ? "updated" : "required"));
	return 0;
}

EVENT_TYPE_DEFINE(ble_peer_conn_params_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_BLE_PEER_CONN_PARAMS_EVENTS),
		  log_ble_peer_conn_params_event,
		  NULL);
