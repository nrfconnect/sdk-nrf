/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_DONGLE_PEER_EVENT_H_
#define _BLE_DONGLE_PEER_EVENT_H_

/**
 * @brief BLE Dongle Peer event
 * @defgroup ble_dongle_peer_event BLE Dongle Peer event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief BLE dongle Peer event. */
struct ble_dongle_peer_event {
	struct app_event_header header;

	uint8_t bt_app_id;
};
APP_EVENT_TYPE_DECLARE(ble_dongle_peer_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_DONGLE_PEER_EVENT_H_ */
