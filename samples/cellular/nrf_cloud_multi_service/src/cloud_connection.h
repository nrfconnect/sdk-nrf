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
 * @brief Sleep until the network is ready for use, or the specified timeout elapses.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if the network became ready in time.
 * @return false if timed out.
 */
bool await_network_ready(k_timeout_t timeout);

/**
 * @brief Sleep until the nRF Cloud connection is ready for use, or the specified timeout elapses.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if nRF Cloud became ready in time.
 * @return false if timed out.
 */
bool await_cloud_ready(k_timeout_t timeout);

/**
 * @brief Sleep until connection to nRF Cloud is lost, or the specified timeout elapses.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if the connection was lost within the timeout.
 * @return false if timed out.
 */
bool await_cloud_disconnected(k_timeout_t timeout);

/**
 * @brief Sleep until the current date-time is known, or the specified timeout elapses.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if the current date-time was determined within the timeout.
 * @return false if timed out.
 */
bool await_date_time_known(k_timeout_t timeout);

/**
 * @brief Register a device message handler to receive general device messages from nRF Cloud.
 *
 * The callback will be called directly from the nRF Cloud connection poll thread, so it will block
 * receipt of data from nRF Cloud until complete. Avoid lengthy operations.
 *
 * @param handler_cb - The callback to handle device messages
 */
void register_general_dev_msg_handler(dev_msg_handler_cb_t handler_cb);

/**
 * @brief Disconnect from nRF Cloud and update internal state accordingly.
 *
 * May also be called to report an external disconnection.
 */
void disconnect_cloud(void);

/**
 * @brief Cloud connection thread function.
 *
 * This function manages the cloud connection thread. Once called, it begins monitoring network
 * status and persistently maintains a connection to nRF Cloud whenever Internet is available.
 */
void cloud_connection_thread_fn(void);

#endif /* _CLOUD_CONNECTION_H_ */
