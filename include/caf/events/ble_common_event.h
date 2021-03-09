/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_COMMON_EVENT_H_
#define _BLE_COMMON_EVENT_H_

/**
 * @brief BLE Common Event
 * @defgroup ble_common_event BLE Common Event
 * @{
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Peer states. */
enum peer_state {
	PEER_STATE_DISCONNECTED,
	PEER_STATE_DISCONNECTING,
	PEER_STATE_CONNECTED,
	PEER_STATE_SECURED,
	PEER_STATE_CONN_FAILED,

	PEER_STATE_COUNT
};

/** @brief Peer operations. */
enum peer_operation {
	PEER_OPERATION_SELECT,
	PEER_OPERATION_SELECTED,
	PEER_OPERATION_SCAN_REQUEST,
	PEER_OPERATION_ERASE,
	PEER_OPERATION_ERASE_ADV,
	PEER_OPERATION_ERASE_ADV_CANCEL,
	PEER_OPERATION_ERASED,
	PEER_OPERATION_CANCEL,

	PEER_OPERATION_COUNT
};

/** @brief BLE peer event. */
struct ble_peer_event {
	struct event_header header;

	enum peer_state state;
	void *id;
};
EVENT_TYPE_DECLARE(ble_peer_event);

/** @brief BLE peer operation event. */
struct ble_peer_operation_event {
	struct event_header header;

	enum peer_operation op;
	uint8_t bt_app_id;
	uint8_t bt_stack_id;
};
EVENT_TYPE_DECLARE(ble_peer_operation_event);

/** @brief BLE connection parameters event. */
struct ble_peer_conn_params_event {
	struct event_header header;

	void *id;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	bool updated;
};
EVENT_TYPE_DECLARE(ble_peer_conn_params_event);

/** @brief BLE peer search event. */
struct ble_peer_search_event {
	struct event_header header;

	bool active;
};
EVENT_TYPE_DECLARE(ble_peer_search_event);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_COMMON_EVENT_H_ */
