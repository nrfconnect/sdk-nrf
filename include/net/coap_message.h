/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file coap_message.h
 *
 * @defgroup iot_sdk_coap_msg CoAP Message
 * @ingroup iot_sdk_coap
 * @{
 * @brief TODO.
 */

#ifndef COAP_MESSAGE_H__
#define COAP_MESSAGE_H__

#include <stdint.h>

#include <net/coap_api.h>
#include <net/coap_codes.h>
#include <net/coap_transport.h>
#include <net/coap_option.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COAP_PAYLOAD_MARKER 0xFF

/**@brief Create a new CoAP message.
 *
 * @details The function will allocate memory for the message internally and
 *          return a CoAP message structure. The callback provided will be
 *          called if a matching message ID/Token occurs in a response message.
 * @param[out] message     Pointer to set the generated coap_message_t
 *                         structure to.
 * @param[in]  init_config Initial configuration parameters of the message to
 *                         generate.
 *
 * @retval 0      If memory for the new message was allocated and the
 *                initialization of the message went successfully.
 * @retval EINVAL Either the message or init_config parameter was NULL.
 */
u32_t coap_message_create(coap_message_t *message,
			  coap_message_conf_t *init_config);

/**@brief Decode a message from a raw buffer.
 *
 * @details When the underlying transport layer receives a message, it has to
 *          be decoded into a CoAP message type structure. This functions
 *          returns a decoded message if decoding was successfully, or NULL
 *          otherwise.
 *
 * @param[out] message     The generated coap_message_t after decoding the raw
 *                         message.
 * @param[in]  raw_message Pointer to the encoded message memory buffer.
 * @param[in]  message_len Length of the raw_message.
 *
 * @retval 0        If the decoding of the message succeeds.
 * @retval EINVAL   If pointer to the message or raw_message were NULL or the
 *                  message could not be decoded successfully. This could happen
 *                  if message length provided is larger than what is possible
 *                  to decode (ex. missing payload marker).
 * @retval EMSGSIZE If the message is less than 4 bytes, not containing a full
 *                  header.
 */
u32_t coap_message_decode(coap_message_t *message, const u8_t *raw_message,
			  u16_t message_len);

/**@brief Encode a CoAP message into a byte buffer.
 *
 * @details This functions has two operations. One is the actual encoding into a
 *          byte buffer. The other is to query the size of a potential encoding.
 *          If buffer variable is omitted, the return value will be the size of
 *          a potential serialized message. This can be used to get some
 *          persistent memory from transport layer. The message have to be kept
 *          until all potential retransmissions has been attempted.
 *
 *          The message can be deleted after this point if the function
 *          succeeds.
 *
 * @param[in]    message Message to encode.
 * @param[in]    buffer  Pointer to the byte buffer where to put the encoded
 *                       message.
 * @param[inout] length  Length of the provided byte buffer passed in by
 *                       reference. If the value 0 is supplied, the encoding
 *                       will not take place, but only the dry run calculating
 *                       the expected length of the encoded message.
 *
 * @retval 0        If the encoding of the message succeeds.
 * @retval EINVAL   If message or length parameter is NULL pointer or the
 *                  message has indicated the length of data, but memory
 *                  pointer is NULL.
 * @retval EMSGSIZE If the provided buffer is not sufficient for the encoded
 *                  message.
 */
u32_t coap_message_encode(coap_message_t *message, u8_t *buffer, u16_t *length);

/**@brief Get the content format mask of the message.
 *
 * @param[in]  message Pointer to the message which to generate the content
 *                     format mask from. Should not be NULL.
 * @param[out] mask    Value by reference to the variable to fill the result
 *                     mask into.
 *
 * @retval 0      If the mask could be generated.
 * @retval EINVAL If the message pointer or the mask pointer given was NULL.
 */
u32_t coap_message_ct_mask_get(coap_message_t *message, u32_t *mask);

/**@brief Get the accept mask of the message.
 *
 * @param[in]  message Pointer to the message which to generate the accept mask
 *                     from. Should not be NULL.
 * @param[out] mask    Value by reference to the variable to fill the result
 *                     mask into.
 *
 * @retval 0      If the mask could be generated.
 * @retval EINVAL If the message pointer or the mask pointer given was NULL.
 */
u32_t coap_message_accept_mask_get(coap_message_t *message, u32_t *mask);

#ifdef __cplusplus
}
#endif

#endif /* COAP_MESSAGE_H__ */

/** @} */
