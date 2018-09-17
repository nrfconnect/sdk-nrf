/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _BLE_EVENT_H_
#define _BLE_EVENT_H_

/**
 * @brief BLE Event
 * @defgroup ble_event BLE Event
 * @{
 */

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const void *PEER_DISCONNECTED;
extern const void *PEER_CONNECTED;
extern const void *PEER_SECURED;

struct ble_peer_event {
	struct event_header header;

	struct bt_conn *conn_id;
	const void *state;
};
EVENT_TYPE_DECLARE(ble_peer_event);

struct ble_interval_event {
	struct event_header header;
};
EVENT_TYPE_DECLARE(ble_interval_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_EVENT_H_ */
