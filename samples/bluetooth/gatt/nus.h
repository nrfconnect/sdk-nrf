/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/conn.h>

#define NUS_UUID_SERVICE     0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
	                         0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E

#define NUS_UUID_NUS_TX_CHAR 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
	                         0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E

#define NUS_UUID_NUS_RX_CHAR 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, \
	                         0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E

/** @brief Callback type for data received */
typedef void (* nus_receive_cb_t)(u8_t * data, u16_t len);

/** @brief Callback type for data sent */
typedef void (* nus_send_cb_t)(u8_t * data, u16_t len);

/**@brief Initialization
 *
 * @details This function will register a BLE service with two characteristics, TX and RX.
 *          Data can be sent from a BLE unit connected to this service to the RX characteristic,
 *          notification can be enabled for the TX characteristic to let a BLE unit connected to 
 *          this service be notified when data sent to it.
 *
 * @param[in] nus_receive_cb  Callback for when data is received, NULL if no callback
 * @param[in] nus_send_cb     Callback for when data is sendt, NULL if no callback
 *
 * @return                    Returns 0 if initialization is successful, othervise negative value
 */
int nus_init(nus_receive_cb_t nus_receive_cb, nus_send_cb_t nus_send_cb);

/**@brief Send data
 *
 * @details This function will send data from the BLE unit running this service to another BLE unit
 *          connected to it.
 *
 * @param[in] data Pointer to a data buffer
 * @param[in] len  Lenght of data in buffer
 *
 * @return         Returns 0 if data is sent, othervise negative value
 */
int nus_data_send(char * data, uint8_t len);

#ifdef __cplusplus
}
#endif


