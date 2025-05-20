/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_MSG_PARSER_H_
#define NFC_NDEF_MSG_PARSER_H_

/**
 * @file
 * @defgroup nfc_ndef_msg_parser Parser for NDEF messages
 * @{
 * @brief Parser for NFC NDEF messages and records.
 */

#include <stdint.h>
#include <zephyr/types.h>
#include <nfc/ndef/record_parser.h>
#include <nfc/ndef/msg.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Memory allocated for a one-record message.
 */
struct nfc_ndef_msg_parser_msg_1 {
	struct nfc_ndef_msg_desc msg_desc;
	struct nfc_ndef_record_desc *record_desc_array[1];
	struct nfc_ndef_bin_payload_desc bin_pay_desc[1];
	struct nfc_ndef_record_desc rec_desc[1];
};

/** @brief Memory allocated for a two-record message.
 */
struct nfc_ndef_msg_parser_msg_2 {
	struct nfc_ndef_msg_desc msg_desc;
	struct nfc_ndef_record_desc *record_desc_array[2];
	struct nfc_ndef_bin_payload_desc bin_pay_desc[2];
	struct nfc_ndef_record_desc rec_desc[2];
};

/** @brief Amount of memory that is required per record in addition to the
 *         memory allocated for the message descriptor.
 */
#define NFC_NDEF_MSG_PARSER_DELTA (sizeof(struct nfc_ndef_msg_parser_msg_2) - \
				   sizeof(struct nfc_ndef_msg_parser_msg_1))

/** @brief Calculate the memory size required for holding the
 *         description of a message that consists of a certain number of
 *         NDEF records.
 *
 *  @param[in] max_count_of_records Maximum number of records to hold.
 */
#define NFC_NDEF_PARSER_REQUIRED_MEM(max_count_of_records)          \
	(sizeof(struct nfc_ndef_msg_parser_msg_1) +                           \
	 ((NFC_NDEF_MSG_PARSER_DELTA) * ((uint32_t)(max_count_of_records) - 1)))

/** @brief Parse NFC NDEF messages.
 *
 *  This function parses NDEF messages using NDEF binary record descriptors.
 *
 *  @note The @p result_buf parameter must point to a word-aligned address.
 *
 *  @param[out] result_buf Pointer to the buffer that will be used to hold
 *                         the NDEF message descriptor. After parsing is
 *                         completed successfully, the first address
 *                         in the buffer is filled by the NDEF message
 *                         descriptor (@ref nfc_ndef_msg_desc), which provides
 *                         a full description of the parsed NDEF message.
 *  @param[in,out] result_buf_len As input: size of the buffer specified by
 *                                @p result_buf. As output: size of the reserved
 *                                (used) part of the buffer specified by
 *                                @p result_buf.
 *  @param[in] raw_data Pointer to the data to be parsed.
 *  @param[in,out] raw_data_len As input: size of the NFC data in
 *                 the @p nfc_data buffer. As output: size of the
 *                 parsed message.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_msg_parse(uint8_t  *result_buf,
		       uint32_t *result_buf_len,
		       const uint8_t *raw_data,
		       uint32_t *raw_data_len);

/** @brief Print the parsed contents of an NDEF message.
 *
 *  @param[in] msg_desc Pointer to the descriptor of the message that should
 *                      be printed.
 */
void nfc_ndef_msg_printout(const struct nfc_ndef_msg_desc *msg_desc);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_NDEF_MSG_PARSER_H_ */
