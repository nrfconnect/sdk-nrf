/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_MGMT_CTRL_CFG_INTERNAL_H_
#define _BT_MGMT_CTRL_CFG_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief	Get the Bluetooth controller manufacturer.
 *
 * @param[in]   print_version   Print the controller version.
 * @param[out]	manufacturer	The controller manufacturer.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_ctlr_cfg_manufacturer_get(bool print_version, uint16_t *manufacturer);

/**
 * @brief	Configure the Bluetooth controller.
 *
 * @param[in]	watchdog_enable	If true, the function will, at given intervals, poll the controller
 *				to ensure it is still alive.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_ctlr_cfg_init(bool watchdog_enable);

#endif /* _BT_MGMT_CTRL_CFG_INTERNAL_H_ */
