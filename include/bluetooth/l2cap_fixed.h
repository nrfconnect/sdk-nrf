/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_L2CAP_FIXED_CID_H__
#define BT_L2CAP_FIXED_CID_H__

/**
 * @file
 * @defgroup bt_l2cap_fixed Fixed L2CAP channel
 * @{
 * @brief APIs to use a fixed L2CAP channel
 */

#include <stdint.h>
#include <zephyr/bluetooth/l2cap.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Register the callbacks for the fixed L2CAP channel
 *
 * The callbacks need to be registered before a connection is established.
 *
 * @param[in] ops The callbacks to be used.
 *                Must point to memory that remains valid.
 */
void bt_l2cap_fixed_register(const struct bt_l2cap_chan_ops *ops);

/** Send data on the fixed L2CAP channel
 *
 * The maximum data length that can be sent and received
 * is defined by BT_L2CAP_TX_MTU and BT_L2CAP_RX_MTU.
 * These are set by changing @kconfig{CONFIG_BT_L2CAP_TX_MTU}
 * and @kconfig{CONFIG_BT_BUF_ACL_RX_SIZE}.
 * The @kconfig{CONFIG_BT_BUF_ACL_RX_SIZE} needs to account for the 4 octets of
 * the basic L2CAP header.
 *
 * @param[in] conn     The connection context
 * @param[in] data     The data to be sent
 * @param[in] data_len The length of the data to be sent
 *
 * @return 0 in case of success or negative value in case of error.
 * @return -EMSGSIZE if `data_len` is larger than the MTU.
 * @return -EINVAL if `conn` or `data` is NULL.
 * @return -ENOTCONN if underlying conn is disconnected.
 */
int bt_l2cap_fixed_send(struct bt_conn *conn,
			const uint8_t *data,
			size_t data_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_L2CAP_FIXED_CID_H__ */
