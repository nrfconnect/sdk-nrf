/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_LAUNCHAPP_MSG_H__
#define NFC_LAUNCHAPP_MSG_H__

/**@file
 *
 * @defgroup nfc_launchapp_msg Launch App messages
 * @{
 *
 * @brief Generation of NFC NDEF messages that can be used to launch apps.
 *
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Function for encoding an NFC NDEF launch app message.
 *
 * This function encodes an NFC NDEF message into a buffer.
 *
 * @param[in] android_package_name Pointer to the Android package name string. If NULL, the
 * Android Application Record will be skipped.
 * @param[in] android_package_name_len Length of the Android package name.
 * @param[in] universal_link Pointer to the Universal Link string. If NULL, the
 * Universal Link Record will be skipped.
 * @param[in] universal_link_len Length of the Universal Link.
 * @param[out] buf Pointer to the buffer for the message.
 * @param[in,out] len Size of the available memory for the message as input. Size of the generated
 * message as output.
 *
 * @retval 0 if the message was successfully created.
 * @retval -EINVAL if both android_package_name_len and universal_link were invalid (equal to NULL).
 * @retval -ENOMEM if the predicted message is larger than the provided buffer space.
 * @retval Other codes might be returned depending on the functions @ref nfc_ndef_msg_encode
 * and @ref nfc_ndef_msg_record_add
 */
int nfc_launchapp_msg_encode(uint8_t  const *android_package_name,
			     uint32_t android_package_name_len,
			     uint8_t  const *universal_link,
			     uint32_t universal_link_len,
			     uint8_t  *buf,
			     size_t   *len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

 #endif // NFC_LAUNCHAPP_MSG_H__
