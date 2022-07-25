/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CONNECTION_H_
#define _CONNECTION_H_
#include <cJSON.h>

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
 * @brief Schedule a (null-terminated) string to be sent as a device message payload. Message will
 *	  be held asynchronously until a valid nRF Cloud connection is established. The entire
 *	  message is copied to the heap, so the original string passed need not be held onto.
 * @return int - 0 on success, -ENOMEM if the outgoing message queue is full.
 */
int send_device_message(const char *const msg);

/**
 * @brief Schedule a cJSON object to be sent as a device message payload. Message will
 *	  be held asynchronously until a valid nRF Cloud connection is established.
 * @return int - 0 on success, otherwise negative error.
 */
int send_device_message_cJSON(cJSON *msg_obj);

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
