/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_EVENT_H_
#define _BLE_EVENT_H_

/**
 * @brief BLE Event
 * @defgroup ble_event BLE Event
 * @{
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include "hwid.h"

#if CONFIG_DESKTOP_BLE_QOS_ENABLE
#include "chmap_filter.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Peer types (position in bitmask). */
enum peer_type {
	PEER_TYPE_MOUSE,
	PEER_TYPE_KEYBOARD,

	PEER_TYPE_COUNT
};

/** @brief BLE discovery complete event. */
struct ble_discovery_complete_event {
	struct app_event_header header;

	struct bt_gatt_dm *dm;
	uint16_t pid;
	uint8_t hwid[HWID_LEN];
	bool peer_llpm_support;
	enum peer_type peer_type;
};
APP_EVENT_TYPE_DECLARE(ble_discovery_complete_event);

#if CONFIG_DESKTOP_BLE_QOS_ENABLE
/** @brief BLE QoS event. */
struct ble_qos_event {
	struct app_event_header header;

	uint8_t chmap[CHMAP_BLE_BITMASK_SIZE];
};
APP_EVENT_TYPE_DECLARE(ble_qos_event);
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_EVENT_H_ */
