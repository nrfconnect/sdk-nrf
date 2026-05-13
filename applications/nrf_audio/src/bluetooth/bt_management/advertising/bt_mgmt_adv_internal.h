/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_MGMT_ADV_INTERNAL_H_
#define _BT_MGMT_ADV_INTERNAL_H_

#include <zephyr/bluetooth/bluetooth.h>

/**
 * @brief	Initialize the advertising part of the Bluetooth management module.
 */
void bt_mgmt_adv_init(void);

/**
 * @brief	Handle timed-out directed advertisement.
 *
 *		This function deletes the old ext_adv and creates a new one.
 *		It also sets the dir_adv_timed_out flag and restarts advertisement.
 *
 * @param[in]	ext_adv_index	Index of the ext_adv to restart.
 */
void bt_mgmt_dir_adv_timed_out(uint8_t ext_adv_index);

#endif /* _BT_MGMT_ADV_INTERNAL_H_ */
