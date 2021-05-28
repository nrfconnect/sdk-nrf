/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/ble_smp_event.h>


EVENT_TYPE_DEFINE(ble_smp_transfer_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_BLE_SMP_TRANSFER_EVENTS),
		  NULL,
		  NULL);
