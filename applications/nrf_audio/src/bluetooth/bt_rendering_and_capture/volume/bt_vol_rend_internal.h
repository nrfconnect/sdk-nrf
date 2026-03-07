/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_VOL_REND_INTERNAL_H_
#define _BT_VOL_REND_INTERNAL_H_

#include <stdint.h>
/**
 * @brief	Set volume to a specific value.
 *
 * @param[in]	volume	The absolute volume to be set.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_vol_rend_set(uint8_t volume);

/**
 * @brief	Turn the volume up by one step.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_vol_rend_up(void);

/**
 * @brief	Turn the volume down by one step.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_vol_rend_down(void);

/**
 * @brief	Mute the output volume of the device.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_vol_rend_mute(void);

/**
 * @brief	Unmute the output volume of the device.
 *
 * @retval	0	Volume change success.
 * @retval	-ENXIO	The feature is disabled.
 * @retval	other	Errors from underlying drivers.
 */
int bt_vol_rend_unmute(void);

/**
 * @brief	Initialize the Volume renderer.
 *
 * @return	0 for success, error otherwise.
 */
int bt_vol_rend_init(void);

#endif /* _BT_VOL_REND_INTERNAL_H_ */
