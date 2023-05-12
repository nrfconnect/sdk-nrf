/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_REND_VOL_INTERNAL_H_
#define _BT_REND_VOL_INTERNAL_H_

#include <zephyr/bluetooth/conn.h>

/**
 * @brief	Set volume to a specific value.
 *
 * @param[in]	volume	The absolute volume to be set.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_rend_vol_set(uint8_t volume);

/**
 * @brief	Turn the volume up by one step.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_rend_vol_up(void);

/**
 * @brief	Turn the volume down by one step.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_rend_vol_down(void);

/**
 * @brief	Mute the output volume of the device.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_rend_vol_mute(void);

/**
 * @brief	Unmute the output volume of the device.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_rend_vol_unmute(void);

/**
 * @brief	Discover Volume Control Service and included services.
 *
 * @param[in]	conn	Pointer to the connection on which to discover the services.
 *
 * @note	This function starts a GATT discovery and sets up handles and
 *		subscriptions for the VCS and included services.
 *		Call it once before any other actions related to the VCS.
 *
 * @return	0 for success, error otherwise.
 */
int bt_rend_vol_discover(struct bt_conn *conn);

/**
 * @brief	Initialize the Volume Control Service client.
 *
 * @return	0 for success, error otherwise.
 */
int bt_rend_vol_ctlr_init(void);

/**
 * @brief	Initialize the Volume renderer.
 *
 * @return	0 for success, error otherwise.
 */
int bt_rend_vol_rend_init(void);

#endif /* _BT_REND_VOL_INTERNAL_H_ */
