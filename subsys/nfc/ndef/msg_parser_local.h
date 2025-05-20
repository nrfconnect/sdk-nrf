/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_MSG_PARSER_LOCAL_H_
#define NFC_NDEF_MSG_PARSER_LOCAL_H_

/**@file
 * @defgroup nfc_ndef_msg_parser_local NDEF message parser (internal)
 * @{
 * @brief    Internal part of the parser for NFC NDEF messages.
 */

#include <stdint.h>
#include <zephyr/types.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/record_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Type for holding descriptors that are used by the NDEF parser.
 */
struct nfc_ndef_parser_memo_desc {
	/* Pointer to the message descriptor. */
	struct nfc_ndef_msg_desc *msg_desc;

	/** Pointer to the array of binary payload descriptors. */
	struct nfc_ndef_bin_payload_desc *bin_pay_desc;

	/** Pointer to the array of record descriptors.*/
	struct nfc_ndef_record_desc *rec_desc;
};


/** @brief Function for resolving data instances in the provided
 *         buffer according to requirements of the function
 *         @ref nfc_ndef_msg_parser_internal.
 *
 *  This internal function distributes the provided memory between certain
 *  data instances that are required by @ref nfc_ndef_msg_parser_internal.
 *
 *  This function should not be used directly.
 *
 *  @param[out] result_buf Pointer to the buffer that will be used to allocate
 *                         data instances.
 *  @param[in,out] result_buf_len As input: size of the buffer specified
 *                                by @p result_buf.
 *                                As output: size of the reserved (used)
 *                                part of the buffer specified by @p result_buf.
 *  @param[out] parser_memo_desc  Pointer to the structure for holding
 *                                descriptors of the allocated data instances.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_msg_parser_memo_resolve(uint8_t *result_buf,
				     uint32_t *result_buf_len,
				     struct nfc_ndef_parser_memo_desc *parser_memo_desc);


/** @brief Parse NFC NDEF messages.
 *
 *  This internal function parses NDEF messages into certain data instances.
 *
 *  This function should not be used directly.
 *
 *  @param[in,out] parser_memo_desc Pointer to the structure that holds
 *                                  descriptors of the allocated data instances
 *                                  for the parser.
 *  @param[in] nfc_data Pointer to the data to be parsed.
 *  @param[in,out] nfc_data_len As input: size of the NFC data in the
 *                              @p nfc_data buffer. As output: size of
 *                              the parsed message.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_msg_parser_internal(struct nfc_ndef_parser_memo_desc *parser_memo_desc,
				 const uint8_t *nfc_data,
				 uint32_t *nfc_data_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_NDEF_MSG_PARSER_LOCAL_H_ */
