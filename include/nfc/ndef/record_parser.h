/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_RECORD_PARSER_H_
#define NFC_NDEF_RECORD_PARSER_H_

#include <stdint.h>
#include <zephyr/types.h>
#include <nfc/ndef/record.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file
 *  @defgroup nfc_ndef_record_parser Parser for NDEF records
 *  @{
 *  @brief Parser for NFC NDEF records.
 */


/** @brief Parse NDEF records.
 *
 *  This parsing implementation uses the binary payload descriptor
 *  (@ref nfc_ndef_bin_payload_desc) to describe the payload for the record.
 *
 *  @param[out] bin_pay_desc Pointer to the binary payload descriptor that
 *                           will be filled and referenced by the record
 *                           descriptor.
 *  @param[out] rec_desc Pointer to the record descriptor that will be filled
 *                       with parsed data.
 *  @param[out] record_location Pointer to the record location.
 *  @param[in] nfc_data Pointer to the raw data to be parsed.
 *  @param[in,out] nfc_data_len As input: size of the NFC data in the
 *                              @p nfc_data buffer. As output: size of the
 *                              parsed record.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_record_parse(struct nfc_ndef_bin_payload_desc *bin_pay_desc,
			  struct nfc_ndef_record_desc *rec_desc,
			  enum nfc_ndef_record_location *record_location,
			  const uint8_t *nfc_data,
			  uint32_t *nfc_data_len);

/** @brief Print the parsed contents of the NDEF record.
 *
 *  @param[in] num Sequence number of the record within the NDEF message.
 *  @param[in] rec_desc Pointer to the descriptor of the record that should
 *                      be printed.
 *
 */
void nfc_ndef_record_printout(uint32_t num,
			      const struct nfc_ndef_record_desc *rec_desc);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NFC_NDEF_RECORD_PARSER_H_ */
