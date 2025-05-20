/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_LE_OOB_REC_PARSER_H_
#define NFC_NDEF_LE_OOB_REC_PARSER_H_

/** @file
 *  @defgroup nfc_ndef_le_oob_rec_parser Parser for NDEF LE OOB Records
 *  @{
 *  @brief Parser for NDEF LE OOB Records
 *
 */
#include <stddef.h>
#include <zephyr/types.h>
#include <nfc/ndef/le_oob_rec.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Check if an NDEF Record is the LE OOB Record.
 *
 *  @param[in] rec_desc General NDEF Record descriptor.
 *
 *  @retval true  If the NDEF Record Type is LE OOB
 *  @retval false If the NDEF Record Type is different.
 */
bool nfc_ndef_le_oob_rec_check(const struct nfc_ndef_record_desc *rec_desc);

/** @brief Parse an NDEF LE OOB Record.
 *
 *  This function only parses NDEF Record descriptors with LE OOB Record Type.
 *  Parsing results are stored in the LE OOB Record descriptor
 *  (@ref nfc_ndef_le_oob_rec_payload_desc).
 *
 *  @param[in]  rec_desc          General NDEF Record descriptor.
 *  @param[out] result_buf        The buffer that will be used to hold the LE
 *                                OOB Record descriptor. After parsing is
 *                                completed successfully, the first address
 *                                in the buffer is filled by the NDEF Record
 *                                descriptor (@ref nfc_ndef_le_oob_rec_payload_desc),
 *                                which provides a full description of the
 *                                parsed NDEF Record.
 *  @param[in,out] result_buf_len As input: size of the @p result_buf buffer
 *                                As output: size of the reserved (used)
 *                                part of the @p result_buf buffer.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_le_oob_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			      uint8_t *result_buf, uint32_t *result_buf_len);

/** @brief Print the parsed contents of an NDEF LE OOB Record.
 *
 *  @param[in] le_oob_rec_desc Descriptor of the LE OOB Record that should
 *                             be printed.
 */
void nfc_ndef_le_oob_rec_printout(
	const struct nfc_ndef_le_oob_rec_payload_desc *le_oob_rec_desc);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NFC_NDEF_LE_OOB_REC_PARSER_H_ */
