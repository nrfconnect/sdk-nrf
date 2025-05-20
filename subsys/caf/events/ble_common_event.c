/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <zephyr/sys/util.h>
#include <stdio.h>

#include <caf/events/ble_common_event.h>


static const char * const state_name[] = {
	"DISCONNECTED",
	"DISCONNECTING",
	"CONNECTED",
	"SECURED",
	"CONN_FAILED"
};

static void log_ble_peer_event(const struct app_event_header *aeh)
{
	const struct ble_peer_event *event = cast_ble_peer_event(aeh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == PEER_STATE_COUNT, "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < PEER_STATE_COUNT);

	APP_EVENT_MANAGER_LOG(aeh, "id=%p %s reason=%" PRIu8,
			      event->id, state_name[event->state], event->reason);
}

static void profile_ble_peer_event(struct log_event_buf *buf,
				   const struct app_event_header *aeh)
{
	const struct ble_peer_event *event = cast_ble_peer_event(aeh);

	nrf_profiler_log_encode_uint32(buf, (uint32_t)event->id);
	nrf_profiler_log_encode_uint8(buf, event->state);
	nrf_profiler_log_encode_uint8(buf, event->reason);
}

APP_EVENT_INFO_DEFINE(ble_peer_event,
		  ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U8),
		  ENCODE("conn_id", "state", "reason"),
		  profile_ble_peer_event);

APP_EVENT_TYPE_DEFINE(ble_peer_event,
		  log_ble_peer_event,
		  &ble_peer_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_BLE_PEER_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

static void log_ble_peer_search_event(const struct app_event_header *aeh)
{
	const struct ble_peer_search_event *event = cast_ble_peer_search_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "%sactive", (event->active)?(""):("in"));
}

static void profile_ble_peer_search_event(struct log_event_buf *buf,
					  const struct app_event_header *aeh)
{
	const struct ble_peer_search_event *event = cast_ble_peer_search_event(aeh);

	nrf_profiler_log_encode_uint8(buf, (uint8_t)event->active);
}

APP_EVENT_INFO_DEFINE(ble_peer_search_event,
		  ENCODE(NRF_PROFILER_ARG_U8),
		  ENCODE("active"),
		  profile_ble_peer_search_event);

APP_EVENT_TYPE_DEFINE(ble_peer_search_event,
		  log_ble_peer_search_event,
		  &ble_peer_search_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_BLE_PEER_SEARCH_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));


static void log_ble_adv_data_update_event(const struct app_event_header *aeh)
{
	APP_EVENT_MANAGER_LOG(aeh, "");
}

APP_EVENT_TYPE_DEFINE(ble_adv_data_update_event,
		      log_ble_adv_data_update_event,
		      NULL,
		      APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_BLE_ADV_DATA_UPDATE_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

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

static void log_ble_peer_operation_event(const struct app_event_header *aeh)
{
	const struct ble_peer_operation_event *event =
		cast_ble_peer_operation_event(aeh);

	BUILD_ASSERT(ARRAY_SIZE(op_name) == PEER_OPERATION_COUNT, "Invalid number of elements");

	__ASSERT_NO_MSG(event->op < PEER_OPERATION_COUNT);

	APP_EVENT_MANAGER_LOG(aeh, "%s bt_app_id=%u bt_stack_id=%u",
			op_name[event->op],
			event->bt_app_id,
			event->bt_stack_id);
}

static void profile_ble_peer_operation_event(struct log_event_buf *buf,
					     const struct app_event_header *aeh)
{
	const struct ble_peer_operation_event *event =
		cast_ble_peer_operation_event(aeh);

	nrf_profiler_log_encode_uint8(buf, event->op);
	nrf_profiler_log_encode_uint8(buf, event->bt_app_id);
	nrf_profiler_log_encode_uint8(buf, event->bt_stack_id);
}

APP_EVENT_INFO_DEFINE(ble_peer_operation_event,
		  ENCODE(NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U8, NRF_PROFILER_ARG_U8),
		  ENCODE("operation", "bt_app_id", "bt_stack_id"),
		  profile_ble_peer_operation_event);

APP_EVENT_TYPE_DEFINE(ble_peer_operation_event,
		  log_ble_peer_operation_event,
		  &ble_peer_operation_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_BLE_PEER_OPERATION_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

static void log_ble_peer_conn_params_event(const struct app_event_header *aeh)
{
	const struct ble_peer_conn_params_event *event =
		cast_ble_peer_conn_params_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "peer=%p min=%"PRIx16 " max=%"PRIx16
			" lat=%"PRIu16 " timeout=%" PRIu16 " (%s)",
			event->id, event->interval_min,	event->interval_max,
			event->latency, event->timeout,
			(event->updated ? "updated" : "required"));
}

APP_EVENT_TYPE_DEFINE(ble_peer_conn_params_event,
		  log_ble_peer_conn_params_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_CAF_INIT_LOG_BLE_PEER_CONN_PARAMS_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
