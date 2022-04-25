/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_ACL_GATEWAY_H_
#define _BLE_ACL_GATEWAY_H_

#include <bluetooth/conn.h>
#include <kernel.h>

/**@brief Get pointer from the peer connection
 *
 * @param[in]	chan_num	The channel of the connection handle to get
 * @param[out]	p_conn		Pointer for peer connection information
 *
 * @return 0 for success, -EINVAL for channel number out of index
 */
int ble_acl_gateway_conn_peer_get(uint8_t chan_num, struct bt_conn **p_conn);

/**@brief Set pointer for the peer connection
 *
 * @param[in]	chan_num	The channel of the connection handle to set
 * @param[out]	p_conn		Pointer for peer connection information
 *
 * @return 0 for success, -EINVAL for channel number out of index
 */
int ble_acl_gateway_conn_peer_set(uint8_t chan_num, struct bt_conn **p_conn);

/**@brief Check if gateway is connected to all headsets over ACL link
 *
 * @return true if all ACL links connected, false otherwise
 */
bool ble_acl_gateway_all_links_connected(void);

/**@brief Work handler for scanning for peer connection
 *
 * @param item The work item that provided the handler
 */
void work_scan_start(struct k_work *item);

/**@brief BLE gateway connected handler
 *
 * @param conn	Connection to peer
 */
void ble_acl_gateway_on_connected(struct bt_conn *conn);

/**@brief Start the MTU exchange procedure.
 *
 * @param conn	Connection to peer
 *
 * @return 0 for success, error otherwise.
 */
int ble_acl_gateway_mtu_exchange(struct bt_conn *conn);

#endif /* _BLE_ACL_GATEWAY_H_ */
