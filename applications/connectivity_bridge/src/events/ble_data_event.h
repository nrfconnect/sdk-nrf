/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _BLE_DATA_EVENT_H_
#define _BLE_DATA_EVENT_H_

/**
 * @brief BLE Data Event
 * @defgroup ble_data_event BLE Data Event
 * @{
 */

#include <string.h>
#include <toolchain/common.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Peer connection event. */
struct ble_data_event {
	struct event_header header;

	u8_t *buf;
	size_t len;
};

EVENT_TYPE_DECLARE(ble_data_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_DATA_EVENT_H_ */
