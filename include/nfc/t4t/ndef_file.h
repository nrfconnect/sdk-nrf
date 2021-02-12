/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_T4T_NDEF_FILE_H_
#define NFC_T4T_NDEF_FILE_H_

/**@file
 *
 * @defgroup nfc_t4t_ndef_file NFC NDEF File
 * @{
 * @brief Generation of NFC NDEF File for the NFC Type 4 Tag.
 *
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Size of NLEN field, used to encode NDEF message for Type 4 Tag. */
#define NFC_NDEF_FILE_NLEN_FIELD_SIZE 2U

/**@brief Get the available size for the NDEF message into the NDEF File.
 *
 * @param[in] _file_buf_size Size of the NDEF File buffer.
 *
 * @return Maximum size of the NDEF Message which can be encoded into
 *         the buffer.
 */
#define nfc_t4t_ndef_file_msg_size_get(_file_buf_size) \
	((_file_buf_size) - NFC_NDEF_FILE_NLEN_FIELD_SIZE)

/**@brief Get the NDEF Message from NFC NDEF File.
 *
 * @param[in] _file_buf Pointer to buffer which stores the NDEF file.
 *
 * @return Pointer to the NDEF Message.
 */
#define nfc_t4t_ndef_file_msg_get(_file_buf) \
	((uint8_t *)((_file_buf) + NFC_NDEF_FILE_NLEN_FIELD_SIZE))

/**@brief Encode the NFC NDEF File
 *
 * @param[in] file_buf Pointer to the NFC NDEF File destination.
 * @param[in, out] size Size of the encoded NDEF Message as input.
 *                      Size of the generated NDEF file as output.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_ndef_file_encode(uint8_t *file_buf, uint32_t *size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_T4T_NDEF_FILE_H_ */
