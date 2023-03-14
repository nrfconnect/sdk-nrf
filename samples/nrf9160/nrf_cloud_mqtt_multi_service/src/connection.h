/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CONNECTION_H_
#define _CONNECTION_H_
#include <net/nrf_cloud.h>

/**
 * @brief nRF Cloud device message handler.
 *
 * @param[in] dev_msg The received device message as a NULL-terminated string
 */
typedef void (*dev_msg_handler_cb_t)(const struct nrf_cloud_data *const dev_msg);

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
 * @brief Trigger and/or notify of a disconnection from nRF Cloud.
 */
void disconnect_cloud(void);

/**
 * @brief Await a connection to an LTE carrier.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if occurred.
 * @return false if timed out.
 */
bool await_lte_connection(k_timeout_t timeout);

/**
 * @brief Await a disconnection from nRF Cloud.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if occurred.
 * @return false if timed out.
 */
bool await_cloud_disconnection(k_timeout_t timeout);

/**
 * @brief Await a complete and readied connection to nRF Cloud.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if occurred.
 * @return false if timed out.
 */
bool await_connection(k_timeout_t timeout);


/**
 * @brief Await the determination of current date and time by the modem.
 *
 * @param timeout - The time to wait before timing out.
 * @return true if occurred.
 * @return false if timed out.
 */
bool await_date_time_known(k_timeout_t timeout);

/**
 * @brief Consume (attempt to send) a single device message from the device message queue. Will wait
 *        until connection to nRF Cloud is established before actually sending. If message fails
 *	  to send, it will be dropped.
 *
 * @return int - 0 on success, otherwise negative error code.
 */
int consume_device_message(void);

/**
 * @brief Schedule a cloud object to be sent as a device message payload. Message will
 *	  be held asynchronously until a valid nRF Cloud connection is established.
 *	  Caller is no longer responsible for device message memory after function returns.
 * @return int - 0 on success, otherwise negative error.
 */
int send_device_message(struct nrf_cloud_obj *const msg_obj);

/**
 * @brief The message queue thread function.
 * Continually consumes device messages from the device message queue.
 */
void message_queue_thread_fn(void);

/**
 * @brief The connection management thread function.
 * Manages our connection to nRF Cloud, resetting and restablishing as necessary.
 */
void connection_management_thread_fn(void);

#endif /* _CONNECTION_H_ */
