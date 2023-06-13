/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CLOUD_CONNECTION_H_
#define _CLOUD_CONNECTION_H_
#include <net/nrf_cloud.h>

/**
 * @brief nRF Cloud device message handler.
 *
 * @param[in] dev_msg The received device message as a NULL-terminated string
 */
typedef void (*dev_msg_handler_cb_t)(const struct nrf_cloud_data *const dev_msg);

/**
 * @brief Await connection to a network.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if occurred.
 * @return false if timed out.
 */
bool await_network_connection(k_timeout_t timeout);

/**
 * @brief Await a complete and readied connection to nRF Cloud.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if occurred.
 * @return false if timed out.
 */
bool await_connection(k_timeout_t timeout);

/**
 * @brief Await a disconnection from nRF Cloud.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if occurred.
 * @return false if timed out.
 */
bool await_cloud_disconnection(k_timeout_t timeout);

/**
 * @brief Await the determination of current date and time by the modem.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if occurred.
 * @return false if timed out.
 */
bool await_date_time_known(k_timeout_t timeout);

/**
 * @brief Check whether we are currently connected to nRF Cloud.
 *
 * @return bool - Whether we are currently connected to nRF Cloud.
 */
bool cloud_is_connected(void);

/**
 * @brief Check whether we are currently disconnecting from nRF Cloud.
 *
 * @return bool - Whether we are currently disconnecting from nRF Cloud.
 */
bool cloud_is_disconnecting(void);

/**
 * @brief Register a device message handler to receive general device messages from nRF Cloud
 *
 * The callback will be called directly from the connection nRF Cloud connection poll thread,
 * so it will block receipt of data from nRF Cloud until complete. Avoid lengthy operations.
 *
 * @param handler_cb - The callback to handle device messages
 */
void register_general_dev_msg_handler(dev_msg_handler_cb_t handler_cb);

/**
 * @brief Trigger and/or notify of a disconnection from nRF Cloud.
 */
void disconnect_cloud(void);

/**
 * @brief The connection management thread function.
 * Manages our connection to nRF Cloud, resetting and restablishing as necessary.
 */
void connection_management_thread_fn(void);

#endif /* _CLOUD_CONNECTION_H_ */
