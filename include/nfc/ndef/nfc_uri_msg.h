/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _NFC_URI_MSG_H__
#define _NFC_URI_MSG_H__

/**@file
 *
 * @defgroup nfc_uri_msg URI messages
 * @{
 * @ingroup  nfc_ndef_messages
 *
 * @brief    Generation of NFC NDEF messages with a URI record.
 *
 */

#include <nfc/ndef/nfc_ndef_msg.h>
#include <nfc/ndef/nfc_uri_rec.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Function for encoding an NFC NDEF URI message.
 *
 * This function encodes an NFC NDEF message into a buffer.
 *
 * @param uri_id_code URI identifier code that defines the protocol field
 * of the URI.
 * @param uri_data Pointer to the URI string. The string should not contain
 * the protocol field if the protocol was specified in @p uri_id_code.
 * @param uri_data_len Length of the URI string.
 * @param buf  Pointer to the buffer for the message.
 * @param len Size of the available memory for the message as input. Size
 * of the generated message as output.
 *
 * @return 0 If the description was successfully created.
 * @return -EINVAL If the URI string was invalid (equal to NULL).
 * @return -ENOMEM If the predicted message size is bigger than the provided
 * buffer space.
 * @return Other Other codes might be returned depending on the function
 * @ref nfc_ndef_msg_encode.
 */
int nfc_uri_msg_encode(enum nfc_uri_id uri_id_code,
		       u8_t const *const uri_data,
		       u8_t uri_data_len,
		       u8_t *buf,
		       u32_t *len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _NFC_URI_MSG_H__ */
