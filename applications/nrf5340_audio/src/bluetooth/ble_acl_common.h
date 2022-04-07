/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_ACL_COMMON_H_
#define _BLE_ACL_COMMON_H_

#include "ble_trans.h"
#include <bluetooth/conn.h>

#define DEVICE_NAME_PEER_L CONFIG_BLE_DEVICE_NAME_BASE "_H_L"
#define DEVICE_NAME_PEER_L_LEN (sizeof(DEVICE_NAME_PEER_L) - 1)

#define DEVICE_NAME_PEER_R CONFIG_BLE_DEVICE_NAME_BASE "_H_R"
#define DEVICE_NAME_PEER_R_LEN (sizeof(DEVICE_NAME_PEER_R) - 1)

/**@brief Set pointer to peer connection in ble_acl_common.c
 *
 * @param conn	Connection to peer
 */
void ble_acl_common_conn_peer_set(struct bt_conn *conn);

/**@brief Get pointer from the peer connection
 *
 * @param p_conn Pointer for peer connection information
 */
void ble_acl_common_conn_peer_get(struct bt_conn **p_conn);

/**@brief Start ACL GAP event, scan on gateway and advertising on headset
 */
void ble_acl_common_start(void);

/**@brief Register callback functions for ACL, and also service init
 *
 * @return 0 if successful, error otherwise
 */
int ble_acl_common_init(void);

#endif /* _BLE_ACL_COMMON_H_ */
