/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BT_GATT_NUS_H_
#define BT_GATT_NUS_H_

/**
 * @file
 * @defgroup bt_gatt_nus Nordic UART (NUS) GATT Service
 * @{
 * @brief Nordic UART (NUS) GATT Service API.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UUID of the NUS Service. **/
#define NUS_UUID_SERVICE 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
			 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E

/** @brief UUID of the TX Characteristic. **/
#define NUS_UUID_NUS_TX_CHAR 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
			     0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E

/** @brief UUID of the RX Characteristic. **/
#define NUS_UUID_NUS_RX_CHAR 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
			     0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E

#define BT_UUID_NUS_SERVICE   BT_UUID_DECLARE_128(NUS_UUID_SERVICE)
#define BT_UUID_NUS_RX        BT_UUID_DECLARE_128(NUS_UUID_NUS_RX_CHAR)
#define BT_UUID_NUS_TX        BT_UUID_DECLARE_128(NUS_UUID_NUS_TX_CHAR)

/** @brief Callback type for data received. */
typedef void (*nus_received_cb_t)(struct bt_conn *conn,
				  const u8_t *const data, u16_t len);

/** @brief Callback type for data sent. */
typedef void (*nus_sent_cb_t)(struct bt_conn *conn,
			      const u8_t *data, u16_t len);

/** @brief Pointers to the callback functions for service events. */
struct bt_gatt_nus_cb {

        /** Callback for data received. **/
	nus_received_cb_t received_cb;

        /** Callback for data sent. **/
	nus_sent_cb_t     sent_cb;
};

/**@brief Initialize the service.
 *
 * @details This function registers a BLE service with two characteristics,
 *          TX and RX. A BLE unit that is connected to this service can send
 *          data to the RX Characteristic. When the BLE unit enables
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
int bt_gatt_nus_init(struct bt_gatt_nus_cb *callbacks);

/**@brief Send data.
 *
 * @details This function sends data from the BLE unit that runs this service
 *          to another BLE unit that is connected to it.
 *
 * @param[in] conn Pointer to connection Object.
 * @param[in] data Pointer to a data buffer.
 * @param[in] len  Length of the data in the buffer.
 *
 * @retval 0 If the data is sent.
 *           Otherwise, a negative value is returned.
 */
int bt_gatt_nus_send(struct bt_conn *conn, const u8_t *data, u16_t len);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* BT_GATT_NUS_H_ */
