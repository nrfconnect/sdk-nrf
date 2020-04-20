/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _BLE_EVENT_H_
#define _BLE_EVENT_H_

/**
 * @brief BLE Event
 * @defgroup ble_event BLE Event
 * @{
 */

#include <toolchain/common.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Peer connection state list. */
#define PEER_STATE_LIST		\
	X(DISCONNECTED)		\
	X(DISCONNECTING)	\
	X(CONNECTED)		\
	X(SECURED)		\
	X(CONN_FAILED)

/** @brief Peer states. */
enum peer_state {
#define X(name) _CONCAT(PEER_STATE_, name),
	PEER_STATE_LIST
#undef X

	PEER_STATE_COUNT
};

/** @brief Peer operation list. */
#define PEER_OPERATION_LIST	\
	X(SELECT)		\
	X(SELECTED)		\
	X(SCAN_REQUEST)		\
	X(ERASE)		\
	X(ERASE_ADV)		\
	X(ERASE_ADV_CANCEL)	\
	X(ERASED)		\
	X(CANCEL)

/** @brief Peer operations. */
enum peer_operation {
#define X(name) _CONCAT(PEER_OPERATION_, name),
	PEER_OPERATION_LIST
#undef X

	PEER_OPERATION_COUNT
};

/** @brief Peer type list. */
#define PEER_TYPE_LIST			\
	X(MOUSE)			\
	X(KEYBOARD)			\

/** @brief Peer types (position in bitmask). */
enum peer_type {
#define X(name) _CONCAT(PEER_TYPE_, name),
	PEER_TYPE_LIST
#undef X

	PEER_TYPE_COUNT
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
	u8_t bt_app_id;
	u8_t bt_stack_id;
};
EVENT_TYPE_DECLARE(ble_peer_operation_event);

/** @brief BLE peer search event. */
struct ble_peer_search_event {
	struct event_header header;

	bool active;
};
EVENT_TYPE_DECLARE(ble_peer_search_event);

/** @brief BLE discovery complete event. */
struct ble_discovery_complete_event {
	struct event_header header;

	struct bt_gatt_dm *dm;
	u16_t pid;
	bool peer_llpm_support;
	enum peer_type peer_type;
};
EVENT_TYPE_DECLARE(ble_discovery_complete_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_EVENT_H_ */
