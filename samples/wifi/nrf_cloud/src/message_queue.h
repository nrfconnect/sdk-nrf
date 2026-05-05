/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MESSAGE_QUEUE_H_
#define _MESSAGE_QUEUE_H_
#include <net/nrf_cloud_codec.h>

/**
 * @brief Schedule a cloud object to be sent as a device message payload. The message will
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

#endif /* _MESSAGE_QUEUE_H_ */
