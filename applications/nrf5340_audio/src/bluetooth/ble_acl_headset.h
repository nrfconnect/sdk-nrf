/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef _BLE_ACL_HEADSET_H_
#define _BLE_ACL_HEADSET_H_

#include <kernel.h>
#include <bluetooth/conn.h>

/**@brief   Work handler for advertising for peer connection
 *
 * @param   item  The work item that provided the handler
 */
void work_adv_start(struct k_work *item);

/**@brief BLE headset connected handler.
 *
 * @param conn	Connection to peer
 */
void ble_acl_headset_on_connected(struct bt_conn *conn);

#endif /* _BLE_ACL_HEADSET_H_ */
