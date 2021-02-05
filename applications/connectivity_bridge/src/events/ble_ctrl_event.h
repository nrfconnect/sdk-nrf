/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_CTRL_EVENT_H_
#define _BLE_CTRL_EVENT_H_

/**
 * @brief BLE Control Event
 * @defgroup ble_ctrl_event BLE Control Event
 * @{
 */

#include <string.h>
#include <toolchain/common.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ble_ctrl_cmd {
	BLE_CTRL_ENABLE,
	BLE_CTRL_DISABLE,
	BLE_CTRL_NAME_UPDATE,
};

/** BLE control event. */
struct ble_ctrl_event {
	struct event_header header;

	enum ble_ctrl_cmd cmd;
	union {
		const char *name_update;
	} param;
};

EVENT_TYPE_DECLARE(ble_ctrl_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_CTRL_EVENT_H_ */
