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
 * @brief	Get the Bluetooth controller version from the network core.
 *
 * @param[out]	ctrl_version	The controller version.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_ctlr_cfg_version_get(uint16_t *ctrl_version);

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
