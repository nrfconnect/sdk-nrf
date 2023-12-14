/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_CH_MSG_H_
#define NFC_NDEF_CH_MSG_H_

/**@file
 *
 * @defgroup nfc_ndef_ch_msg NFC Connection Hanover messages
 * @{
 * @ingroup nfc_ndef_messages
 *
 * @brief Generation of The Connection Handover NDEF messages.
 *
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <nfc/ndef/ch.h>
#include <nfc/ndef/le_oob_rec.h>

#ifdef _cplusplus
extern "C" {
#endif


#define NFC_NDEF_CH_MSG_MAJOR_VER CONFIG_NFC_NDEF_CH_MAJOR_VERSION
#define NFC_NDEF_CH_MSG_MINOR_VER CONFIG_NFC_NDEF_CH_MINOR_VERSION

/** NFC NDEF Connection Handover message structure.
 *  This structure contains all needed records needed to
 *  create the Connection Handover message. It can contain
 *  one or more the Alternative Carrier Records and the Carrier Records.
 */
struct nfc_ndef_ch_msg_records {
	/** Array of the Alternative Carrier Records. */
	const struct nfc_ndef_record_desc *ac;

	/** Array of the Carrier Records. */
	const struct nfc_ndef_record_desc *carrier;

	/** Count of the Alternative Carrier Records and the Carrier Records. */
	size_t cnt;
};

/**
 * @brief Encode an NFC NDEF LE OOB message.
 *
 * This function encodes an NFC NDEF message into a buffer.
 *
 * @param[in] oob Pointer to the LE OOB Record payload descriptor.
 * @param[out] buf Pointer to the buffer for the message.
 * @param[in, out] len Size of the available memory for the message as input.
 *                     Size of the generated message as output.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_msg_le_oob_encode(const struct nfc_ndef_le_oob_rec_payload_desc *oob,
				  uint8_t *buf, size_t *len);

/**
 * @brief Create an NFC NDEF Handover Select message.
 *
 * This function create an NFC NDEF message. It
 * create a nested NDEF message with the Alternative Carrier Records.
 * Next the Records with the Carrier data are added into the main
 * Connection Hanover Select message. Number of the Alternative
 * Carrier Records must be equal to the number of the Carrier Records.
 *
 * @param[in, out] msg Pointer to NDEF message. This message is initialized
 *                     with Connection Handover Record. The message must
 *                     be able to keep at least count of the Alternative
 *                     Carrier Records + 1 records.
 * @param[out] hs_rec Pointer to the Handover Select Record.
 * @param[in] records Pointer to structure which contains the Alternative
 *                    Carrier Records and the Carrier Records
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_msg_hs_create(struct nfc_ndef_msg_desc *msg,
			      struct nfc_ndef_record_desc *hs_rec,
			      const struct nfc_ndef_ch_msg_records *records);

/**
 * @brief Encode an NFC NDEF Handover Request message.
 *
 * This function create an NFC NDEF message. It
 * create a nested NDEF message with the Alternative Carrier Records.
 * Next the Records with the Carrier data are added into the main
 * Connection Hanover Request message. Number of the Alternative
 * Carrier Records must be equal to the number of the Carrier Records.
 *
 * @param[in, out] msg Pointer to NDEF message. This message is initialized
 *                     with Connection Handover Record. The message must
 *                     be able to keep at least count of the Alternative
 *                     Carrier Records + 1 records.
 * @param[out] hr_rec Pointer to the Handover Request Record.
 * @param[in] cr_rec Pointer to the Collision Resolution Record.
 * @param[in] records Pointer to structure which contains the Alternative
 *                    Carrier Records and the Carrier Records
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_msg_hr_create(struct nfc_ndef_msg_desc *msg,
			      struct nfc_ndef_record_desc *hr_rec,
			      const struct nfc_ndef_record_desc *cr_rec,
			      const struct nfc_ndef_ch_msg_records *records);

/**
 * @brief Create an NFC NDEF Handover Mediation message.
 *
 * This function creates an NFC NDEF message. It
 * create a nested NDEF message with the Alternative Carrier Records.
 * Next the Records with the Carrier data are added into the main
 * Connection Hanover Mediation message. Number of the Alternative
 * Carrier Records must be equal to the number of the Carrier Records.
 *
 * @param[in, out] msg Pointer to NDEF message. This message is initialized
 *                     with Connection Handover Record. The message must
 *                     be able to keep at least count of the Alternative
 *                     Carrier Records + 1 records.
 * @param[out] hm_rec Pointer to the Handover Mediation Record.
 * @param[in] records Pointer to structure which contains the Alternative
 *                    Carrier Records and the Carrier Records
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_msg_hm_create(struct nfc_ndef_msg_desc *msg,
			      struct nfc_ndef_record_desc *hm_rec,
			      const struct nfc_ndef_ch_msg_records *records);

/**
 * @brief Create an NFC NDEF Handover Initiate message.
 *
 * This function creates an NFC NDEF message. It
 * create a nested NDEF message with the Alternative Carrier Records.
 * Next the Records with the Carrier data are added into the main
 * Connection Hanover Initiate message. Number of the Alternative
 * Carrier Records must be equal to the number of the Carrier Records.
 *
 * @param[in, out] msg Pointer to NDEF message. This message is initialized
 *                     with Connection Handover Record. The message must
 *                     be able to keep at least count of the Alternative
 *                     Carrier Records + 1 records.
 * @param[out] hi_rec Pointer to the Handover Initiate Record.
 * @param[in] records Pointer to structure which contains the Alternative
 *                    Carrier Records and the Carrier Records
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_ch_msg_hi_create(struct nfc_ndef_msg_desc *msg,
			      struct nfc_ndef_record_desc *hi_rec,
			      const struct nfc_ndef_ch_msg_records *records);

#ifdef _cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_NDEF_CH_MSG_H_ */
