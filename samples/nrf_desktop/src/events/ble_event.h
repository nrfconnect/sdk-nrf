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
	X(CONNECTED)		\
	X(SECURED)

/** @brief Peer states. */
enum peer_state {
#define X(name) _CONCAT(PEER_STATE_, name),
	PEER_STATE_LIST
#undef X

	PEER_STATE_COUNT
};

/** @brief BLE peer event. */
struct ble_peer_event {
	struct event_header header;

	enum peer_state state;
	void *id;
};
EVENT_TYPE_DECLARE(ble_peer_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_EVENT_H_ */
