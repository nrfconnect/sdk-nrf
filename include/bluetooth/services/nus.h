/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_NUS_H_
#define BT_NUS_H_

/**
 * @file
 * @defgroup bt_nus Nordic UART (NUS) GATT Service
 * @{
 * @brief Nordic UART (NUS) GATT Service API.
 */

#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UUID of the NUS Service. **/
#define BT_UUID_NUS_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)

/** @brief UUID of the TX Characteristic. **/
#define BT_UUID_NUS_TX_VAL \
	BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)

/** @brief UUID of the RX Characteristic. **/
#define BT_UUID_NUS_RX_VAL \
	BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)

#define BT_UUID_NUS_SERVICE   BT_UUID_DECLARE_128(BT_UUID_NUS_VAL)
#define BT_UUID_NUS_RX        BT_UUID_DECLARE_128(BT_UUID_NUS_RX_VAL)
#define BT_UUID_NUS_TX        BT_UUID_DECLARE_128(BT_UUID_NUS_TX_VAL)

/** @brief Pointers to the callback functions for service events. */
struct bt_nus_cb {
	/** @brief Data received callback.
	 *
	 * The data has been received as a write request on the NUS RX
	 * Characteristic.
	 *
	 * @param[in] conn  Pointer to connection object that has received data.
	 * @param[in] data  Received data.
	 * @param[in] len   Length of received data.
	 */
	void (*received)(struct bt_conn *conn,
			 const uint8_t *const data, uint16_t len);

	/** @brief Data sent callback.
	 *
	 * The data has been sent as a notification and written on the NUS TX
	 * Characteristic.
	 *
	 * @param[in] conn Pointer to connection object, or NULL if sent to all
	 *                 connected peers.
	 */
	void (*sent)(struct bt_conn *conn);
};

/**@brief Initialize the service.
 *
 * @details This function registers a GATT service with two characteristics,
 *          TX and RX. A remote device that is connected to this service
 *          can send data to the RX Characteristic. When the remote enables
 *          notifications, it is notified when data is sent to the TX
 *          Characteristic.
 *
 * @param[in] callbacks  Struct with function pointers to callbacks for service
 *                       events. If no callbacks are needed, this parameter can
 *                       be NULL.
 *
 * @retval 0 If initialization is successful.
 *           Otherwise, a negative value is returned.
 */
int bt_nus_init(struct bt_nus_cb *callbacks);

/**@brief Send data.
 *
 * @details This function sends data to a connected peer, or all connected
 *          peers.
 *
 * @param[in] conn Pointer to connection object, or NULL to send to all
 *                 connected peers.
 * @param[in] data Pointer to a data buffer.
 * @param[in] len  Length of the data in the buffer.
 *
 * @retval 0 If the data is sent.
 *           Otherwise, a negative value is returned.
 */
int bt_nus_send(struct bt_conn *conn, const uint8_t *data, uint16_t len);

/**@brief Get maximum data length that can be used for @ref bt_nus_send.
 *
 * @param[in] conn Pointer to connection Object.
 *
 * @return Maximum data length.
 */
static inline uint32_t bt_nus_get_mtu(struct bt_conn *conn)
{
	/* According to 3.4.7.1 Handle Value Notification off the ATT protocol.
	 * Maximum supported notification is ATT_MTU - 3 */
	return bt_gatt_get_mtu(conn) - 3;
}

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* BT_NUS_H_ */
