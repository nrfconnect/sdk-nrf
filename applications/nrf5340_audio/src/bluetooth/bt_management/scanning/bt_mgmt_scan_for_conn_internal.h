/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_MGMT_SCAN_FOR_CONN_INTERNAL_H_
#define _BT_MGMT_SCAN_FOR_CONN_INTERNAL_H_

#include <zephyr/bluetooth/bluetooth.h>

/**
 * @brief	Scan for a connection with the given device @p name.
 *
 * @param[in]	scan_param	Scan parameters to use.
 * @param[in]	name		Device name to search for.
 *
 * @return	0 if success, error otherwise.
 */
int bt_mgmt_scan_for_conn_start(struct bt_le_scan_param *scan_param, char const *const name);

#endif /* _BT_MGMT_SCAN_FOR_CONN_INTERNAL_H_ */
