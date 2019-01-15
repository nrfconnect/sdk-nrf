/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file coap_queue.h
 *
 * @defgroup iot_sdk_coap_queue CoAP Message Queue
 * @ingroup iot_sdk_coap
 * @{
 * @brief TODO.
 */

#ifndef COAP_QUEUE_H__
#define COAP_QUEUE_H__

#include <stdint.h>

#include <net/coap_message.h>
#include <net/coap_transport.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	/** Miscellaneous pointer to application provided data that is
	 *  associated with the message. Copied from the coap_message_t when
	 *  creating the item.
	 */
	void *arg;

	/** Quick reference to the handle value of the current item. */
	u32_t handle;

	/** Message ID. */
	u16_t mid;

	/** Message Token length. */
	u8_t token_len;

	/** Message Token value up to 8 bytes. */
	u8_t token[8];

	/** Re-transmission attempt count. */
	u8_t retrans_count;

	/** Time until new re-transmission attempt. */
	u16_t timeout;

	/** Last timeout value used. */
	u16_t timeout_val;

	/** Source port to use when re-transmitting. */
	coap_transport_handle_t transport;

	/** Pointer to the data buffer containing the encoded CoAP message. */
	u8_t *buffer;

	/** Size of the data buffer containing the encoded CoAP message. */
	u32_t buffer_len;

	/** Callback function to be called upon response or transmission
	 *  timeout.
	 */
	coap_response_callback_t callback;

	/** Destination address and port number to the remote. Used sockaddr_in6
	 *  to reserve memory for maximum possible address size.
	 */
	struct sockaddr_in6 remote;
} coap_queue_item_t;

/**@brief Initialize the CoAP message queue.
 *
 * @retval 0 If initialization completed successfully.
 */
u32_t coap_queue_init(void);

/**@brief Add item to the queue.
 *
 * @param[in] item Pointer to an item which to add to the queue. The function
 *                 will copy all data provided.
 *
 * @retval 0       If adding the item was successful.
 * @retval ENOMEM  If max number of queued elements has been reached. This is
 *                 configured by CONFIG_NRF_COAP_MESSAGE_QUEUE_SIZE.
 * @retval EACCESS If the element could not be added.
 */
u32_t coap_queue_add(coap_queue_item_t *item);

/**@brief Remove item from the queue.
 *
 * @param[in] item Pointer to an item which to remove from the queue.
 *                 Should not be NULL.
 *
 * @retval 0      If the item was successfully removed from the queue.
 * @retval EINVAL If item pointer is NULL.
 * @retval ENOENT If the item was not located in the queue.
 */
u32_t coap_queue_remove(coap_queue_item_t *item);

/**@brief Search for item by token.
 *
 * @details Search the items for any item matching the token.
 *
 * @param[out] item      Pointer to be filled by the function if item matching
 *                       the token has been found. Should not be NULL.
 * @param[in]  token     Pointer to token array to be matched.
 * @param[in]  token_len Length of the token to be matched.
 *
 * @retval 0      If an item was successfully located.
 * @retval EINVAL If item pointer is NULL.
 * @retval ENOENT If no item was found.
 */
u32_t coap_queue_item_by_token_get(coap_queue_item_t **item, u8_t *token,
				   u8_t token_len);

/**@brief Search for item by message id.
 *
 * @details Search the items for any item matching the message id.
 *
 * @param[out] item       Pointer to be filled by the function if item matching
 *                        the message id has been found. Should not be NULL.
 * @param[in]  message_id Message id to be matched.
 *
 * @retval 0      If an item was successfully located.
 * @retval EINVAL If item pointer is NULL.
 * @retval ENOENT If no item was found.
 */
u32_t coap_queue_item_by_mid_get(coap_queue_item_t **item, u16_t message_id);

/**@brief Iterate through items.
 *
 * @param[out] item  Pointer to be filled by the search function upon finding
 *                   the next queued item starting from the item pointer
 *                   provided. Should not be NULL.
 * @param[in]  start Pointer to the item where to start the search.
 *
 * @retval 0      If item was found.
 * @retval EINVAL If item pointer is NULL.
 * @retval ENOENT If next item was not found.
 */
u32_t coap_queue_item_next_get(coap_queue_item_t **item,
			       coap_queue_item_t *start);

#ifdef __cplusplus
}
#endif

#endif /* COAP_QUEUE_H__ */

/** @} */
