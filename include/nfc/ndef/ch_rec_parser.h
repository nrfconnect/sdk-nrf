/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_CH_REC_PARSER_H_
#define NFC_NDEF_CH_REC_PARSER_H_

/** @file
 *  @defgroup nfc_ndef_ch_record_parser Parser for NDEF Connection Handover
 *                                      Records
 *  @{
 *  @brief Parser for NDEF Connection Handover Records
 *
 */

#include <nfc/ndef/ch.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NFC NDEF Connection Handover Record type.
 */
enum nfc_ndef_ch_rec_type {
	/** NFC NDEF Connection Handover Select Record. */
	NFC_NDEF_CH_REC_TYPE_HANDOVER_SELECT,

	/** NFC NDEF Connection Handover Request Record. */
	NFC_NDEF_CH_REC_TYPE_HANDOVER_REQUEST,

	/** NFC NDEF Connection Handover Initiate Record. */
	NFC_NDEF_CH_REC_TYPE_HANDOVER_INITIATE,

	/** NFC NDEF Connection Handover Mediation Record. */
	NFC_NDEF_CH_REC_TYPE_HANDOVER_MEDIATION
};

/** @brief Check if an NDEF Record is the Alternative Carrier Record.
 *
 *  @param[in] rec_desc General NDEF Record descriptor.
 *
 *  @retval true If the NDEF Record Type is Alternative Carrier Record.
 *  @retval false If the NDEF Record Type is different.
 */
bool nfc_ndef_ch_ac_rec_check(const struct nfc_ndef_record_desc *rec_desc);

/** @brief Check if an NDEF Record is the Handover Collision Resolution Record.
 *
 *  @param[in] rec_desc General NDEF Record descriptor.
 *
 *  @retval true If the NDEF Record Type is Handover Collision Resolution
 *               Record.
 *  @retval false If the NDEF Record Type is different.
 */
bool nfc_ndef_ch_cr_rec_check(const struct nfc_ndef_record_desc *rec_desc);

/** @brief Check if an NDEF Record is the Handover Carrier Record.
 *
 *  @param[in] rec_desc General NDEF Record descriptor.
 *
 *  @retval true If the NDEF Record Type is the Handover Carrier Record.
 *  @retval false If the NDEF Record Type is different.
 */
bool nfc_ndef_ch_hc_rec_check(const struct nfc_ndef_record_desc *rec_desc);

/** @brief Check if an NDEF Record is the fallowing Handover Connection Record:
 *	- Handover Select Record
 *	- Handover Request Record
 *	- Handover Mediation Record
 *	- Handover Initiate Record
 *
 *  @param[in] rec_desc General NDEF Record descriptor.
 *  @param[in] rec_type Connection Handover Record type to check.
 *
 *  @retval true If the NDEF Record Type is @p rec_type Record.
 *  @retval false If the NDEF Record Type is different.
 */
bool nfc_ndef_ch_rec_check(const struct nfc_ndef_record_desc *rec_desc,
			   enum nfc_ndef_ch_rec_type rec_type);

/** @brief Parse the fallowing NDEF Connection Handover Records:
 *	- Handover Select Record
 *	- Handover Request Record
 *	- Handover Mediation Record
 *	- Handover Initiate Record
 *
 *  This function only parses NDEF Record descriptors with Connection Handover
 *  Type. Parsing results are stored in the Handover Connection Record
 *  descriptor (@ref nfc_ndef_ch_rec).
 *
 *  @param[in]  rec_desc   General NDEF Record descriptor.
 *  @param[out] result_buf The buffer that will be used to hold the Connection
 *                         Hendover Record descriptor. After parsing is
 *                         completed successfully, the first address
 *                         in the buffer is filled by the NDEF Record
 *                         descriptor (@ref nfc_ndef_ch_rec),
 *                         which provides a full description of the
 *                         parsed NDEF Record.
 *  @param[in,out] result_buf_len As input: size of the @p result_buf buffer
 *                                As output: size of the reserved (used)
 *                                part of the @p result_buf buffer.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			  uint8_t *result_buf, uint32_t *result_buf_len);

/** @brief Parse an NDEF Alternative Carrier Record.
 *
 *  This function only parses NDEF Record descriptors with Alternative Carrier
 *  Record Type. Parsing results are stored in the Alternative Carrier Record
 *  descriptor (@ref nfc_ndef_ch_ac_rec).
 *
 *  @param[in]  rec_desc   General NDEF Record descriptor.
 *  @param[out] result_buf The buffer that will be used to hold the Alternative
 *                         Carrier Record descriptor. After parsing is
 *                         completed successfully, the first address
 *                         in the buffer is filled by the NDEF Record
 *                         descriptor (@ref nfc_ndef_ch_ac_rec),
 *                         which provides a full description of the
 *                         parsed NDEF Record.
 *  @param[in,out] result_buf_len As input: size of the @p result_buf buffer
 *                                As output: size of the reserved (used)
 *                                part of the @p result_buf buffer.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_ac_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			     uint8_t *result_buf, uint32_t *result_buf_len);

/** @brief Parse an NDEF Collision Resolution Record.
 *
 *  This function only parses NDEF Record descriptors with Collision Resolution
 *  Record Type. Parsing results are stored in the Collision Resolution Record
 *  descriptor (@ref nfc_ndef_ch_cr_rec).
 *
 *  @param[in] rec_desc General NDEF Record descriptor.
 *  @param[out] cr_rec Parsed Collision Resolution Record descriptor.
 *                     After parsing is completed successfully it is filled with
 *                     Collision Resolution parsed data.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_cr_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			     struct nfc_ndef_ch_cr_rec *cr_rec);

/** @brief Parse an NDEF Handover Carrier Record.
 *
 *  This function only parses NDEF Record descriptors with Handover Carrier
 *  Record Type. Parsing results are stored in the Handover Carrier Record
 *  descriptor (@ref nfc_ndef_ch_hc_rec).
 *
 *  @param[in]  rec_desc   General NDEF Record descriptor.
 *  @param[out] result_buf The buffer that will be used to hold the Handover
 *                         Carrier Record descriptor. After parsing is
 *                         completed successfully, the first address
 *                         in the buffer is filled by the NDEF Record
 *                         descriptor (@ref nfc_ndef_ch_hc_rec),
 *                         which provides a full description of the
 *                         parsed NDEF Record.
 *  @param[in,out] result_buf_len As input: size of the @p result_buf buffer
 *                                As output: size of the reserved (used)
 *                                part of the @p result_buf buffer.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_hc_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			     uint8_t *result_buf, uint32_t *result_buf_len);

/** @brief Print the parsed contents of the fallowing Connection Handover
 *  Records:
 *	- Handover Select Record
 *	- Handover Request Record
 *	- Handover Mediation Record
 *	- Handover Initiate Record
 *
 *  @param[in] ch_rec Descriptor of the Connection Handover Record that should
 *                    be printed.
 */
void nfc_ndef_ch_rec_printout(const struct nfc_ndef_ch_rec *ch_rec);

/** @brief Print the parsed contents of an NDEF Alternative Carrier Record.
 *
 *  @param[in] ac_rec Descriptor of the Alternative Carrier Record
 *                    that should be printed.
 */
void nfc_ndef_ac_rec_printout(const struct nfc_ndef_ch_ac_rec *ac_rec);

/** @brief Print the parsed contents of an NDEF Handover Carrier Record.
 *
 *  @param[in] hc_rec Descriptor of the Handover Carrier Record that should
 *                    be printed.
 */
void nfc_ndef_hc_rec_printout(const struct nfc_ndef_ch_hc_rec *hc_rec);

/** @brief Print the parsed contents of an NDEF Collision Resolution Record.
 *
 *  @param[in] cr_rec Descriptor of the Collision Resolution Record
 *                    that should be printed.
 */
void nfc_ndef_cr_rec_printout(const struct nfc_ndef_ch_cr_rec *cr_rec);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* NFC_NDEF_CH_REC_PARSER_H_ */
