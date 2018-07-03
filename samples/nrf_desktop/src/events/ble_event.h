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

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ble_peer_event {
	struct event_header header;

	bt_addr_le_t address;
	bool connected;
};
EVENT_TYPE_DECLARE(ble_peer_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_EVENT_H_ */
