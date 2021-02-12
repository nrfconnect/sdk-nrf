/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

	BUILD_ASSERT(ARRAY_SIZE(state_name) == PEER_STATE_COUNT,
			 "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < PEER_STATE_COUNT);

	return snprintf(buf, buf_len, "id=%p %s", event->id,
			state_name[event->state]);
}

static void profile_ble_peer_event(struct log_event_buf *buf,
				   const struct event_header *eh)
{
	const struct ble_peer_event *event = cast_ble_peer_event(eh);

	profiler_log_encode_u32(buf, (uint32_t)event->id);
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

	profiler_log_encode_u32(buf, (uint32_t)event->active);
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

	BUILD_ASSERT(ARRAY_SIZE(op_name) == PEER_OPERATION_COUNT,
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

static int log_ble_peer_conn_params_event(const struct event_header *eh,
					  char *buf, size_t buf_len)
{
	const struct ble_peer_conn_params_event *event =
		cast_ble_peer_conn_params_event(eh);

	return snprintf(buf, buf_len, "peer=%p min=%"PRIx16 " max=%"PRIx16
			" lat=%"PRIu16 " timeout=%" PRIu16 " (%s)",
			event->id, event->interval_min,	event->interval_max,
			event->latency, event->timeout,
			(event->updated ? "updated" : "required"));
}

EVENT_TYPE_DEFINE(ble_peer_conn_params_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BLE_PEER_CONN_PARAMS_EVENT),
		  log_ble_peer_conn_params_event,
		  NULL);

EVENT_TYPE_DEFINE(ble_discovery_complete_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BLE_DISC_COMPLETE_EVENT),
		  NULL,
		  NULL);

EVENT_TYPE_DEFINE(ble_smp_transfer_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BLE_SMP_TRANSFER_EVENT),
		  NULL,
		  NULL);

#if CONFIG_DESKTOP_BLE_QOS_ENABLE
static int log_ble_qos_event(const struct event_header *eh,
			     char *buf, size_t buf_len)
{
	const struct ble_qos_event *event = cast_ble_qos_event(eh);
	int pos = 0;
	int err;

	err = snprintf(&buf[pos], buf_len - pos, "chmap:");
	for (size_t i = 0; (i < CHMAP_BLE_BITMASK_SIZE) && (err > 0); i++) {
		pos += err;
		err = snprintf(&buf[pos], buf_len - pos, " 0x%02x", event->chmap[i]);
	}
	if (err < 0) {
		pos = err;
	}

	return pos;
}

EVENT_TYPE_DEFINE(ble_qos_event,
		  IS_ENABLED(CONFIG_DESKTOP_INIT_LOG_BLE_QOS_EVENT),
		  log_ble_qos_event,
		  NULL);
#endif
