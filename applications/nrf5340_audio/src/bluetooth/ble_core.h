/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_CORE_H_
#define _BLE_CORE_H_

#include <zephyr.h>

/**@brief	BLE core ready callback type.
 */
typedef void (*ble_core_ready_t)(void);

/**@brief	Get controller version
 *
 * @param[out]	version Pointer to version
 *
 * @return      0 for success, error otherwise
 */
int net_core_ctrl_version_get(uint16_t *version);

/**@brief	Disable LE power control feature
 *
 * @return	0 for success, error otherwise
 */
int ble_core_le_pwr_ctrl_disable(void);

/**@brief	Initialize BLE subsystem
 *
 * @return	0 for success, error otherwise
 */
int ble_core_init(ble_core_ready_t ready_callback);

#endif /* _BLE_CORE_H_ */
