/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

#ifdef __cplusplus
extern "C" {
#endif

#define NUS_UUID_SERVICE 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
			 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E

#define NUS_UUID_NUS_TX_CHAR 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
			     0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E

#define NUS_UUID_NUS_RX_CHAR 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
			     0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E

#define BT_UUID_NUS_SERVICE   BT_UUID_DECLARE_128(NUS_UUID_SERVICE)
#define BT_UUID_NUS_RX        BT_UUID_DECLARE_128(NUS_UUID_NUS_RX_CHAR)
#define BT_UUID_NUS_TX        BT_UUID_DECLARE_128(NUS_UUID_NUS_TX_CHAR)

/** @brief Callback type for data received */
typedef void (*nus_received_cb_t)(const u8_t *const data, u16_t len);

/** @brief Callback type for data sent */
typedef void (*nus_sent_cb_t)(const u8_t *data, u16_t len);

/** @brief Struct contaning pointers to callback function for service events */
struct bt_nus_cb {
	nus_received_cb_t received_cb;
	nus_sent_cb_t     sent_cb;
};

/**@brief Initialization
 *
 * @details This function will register a BLE service with two characteristics,
 *          TX and RX. Data can be sent from a BLE unit connected to this
 *          service to the RX characteristic, notification can be enabled for
 *          the TX characteristic to let a BLE unit connected to this service
 *          be notified when data sent to it.
 *
 * @param[in] callbacks  Struct with function pointers to callbacks for service
 *                       events. If no callbacks are needed this parameter can
 *                       be NULL
 *
 * @return                    Returns 0 if initialization is successful,
 *                            otherwise a negative value
 */
int nus_init(struct bt_nus_cb *callbacks);

/**@brief Send data
 *
 * @details This function will send data from the BLE unit running this service
 *          to another BLE unit connected to it.
 *
 * @param[in] data Pointer to a data buffer
 * @param[in] len  Length of data in buffer
 *
 * @return         Returns 0 if data is sent, othervise negative value
 */
int nus_send(const u8_t *data, u16_t len);

#ifdef __cplusplus
}
#endif

