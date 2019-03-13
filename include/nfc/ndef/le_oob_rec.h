/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NFC_NDEF_LE_OOB_REC_H_
#define NFC_NDEF_LE_OOB_REC_H_

/**@file
 *
 * @defgroup nfc_ndef_le_oob_rec LE OOB records
 * @{
 * @ingroup  nfc_ndef_messages
 *
 * @brief    Generation of NFC NDEF LE OOB records for NDEF messages.
 *
 */
#include <stddef.h>
#include <zephyr/types.h>
#include <nfc/ndef/nfc_ndef_record.h>
#include <bluetooth/bluetooth.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief External reference to the type field of the Bluetooth LE Carrier
 * Configuration NDEF Record, defined in the file @c le_oob_rec.c.
 */
extern const u8_t nfc_ndef_le_oob_rec_type_field[32];

/**
 * @brief LE role options.
 */
enum nfc_ndef_le_oob_rec_le_role {
	/** Only Peripheral role supported. */
	NFC_NDEF_LE_OOB_REC_LE_ROLE_PERIPH_ONLY,
	/** Only Central role supported. */
	NFC_NDEF_LE_OOB_REC_LE_ROLE_CENTRAL_ONLY,
	/**
	 * Peripheral and Central roles supported. Peripheral role preferred for
	 * connection establishment.
	 */
	NFC_NDEF_LE_OOB_REC_LE_ROLE_PERIPH_PREFFERED,
	/**
	 * Peripheral and Central roles supported. Central role preferred for
	 * connection establishment.
	 */
	NFC_NDEF_LE_OOB_REC_LE_ROLE_CENTRAL_PREFFERED,
	/** Total number of options. */
	NFC_NDEF_LE_OOB_REC_LE_ROLE_OPTIONS_NUM,
};

/**
 * @brief LE OOB record payload descriptor.
 */
struct nfc_ndef_le_oob_rec_payload_desc {
	struct bt_le_oob *oob_data;
	enum nfc_ndef_le_oob_rec_le_role le_role;
	u8_t flags;
	struct {
		bool tk_value;
		bool le_sc_data;
		bool appearance;
		bool flags;
		bool local_name;
	} include;
};

/**
 * @brief Construct the payload for a Bluetooth Carrier
 * Configuration LE Record.
 *
 * This function encodes the record payload according to the payload descriptor.
 * It implements an API compatible with @c payload_constructor_t.
 *
 * @param payload_desc Pointer to the description of the payload.
 * @param buff         Pointer to payload destination. If NULL, function will
 *                     calculate the expected size of the record payload.
 * @param len          Size of available memory to write as input. Size of generated
 *                     payload as output.
 *
 * @retval 0       If the payload was encoded successfully.
 * @retval -EINVAL If parameters in the payload descriptor are invalid or
 *                 missing.
 * @retval -ENOMEM If the predicted payload size is bigger than the provided
 *                 buffer space.
 */
int nfc_ndef_le_oob_rec_payload_constructor(
	const struct nfc_ndef_le_oob_rec_payload_desc *payload_desc, u8_t *buff,
	u32_t *len);

/** @brief Generate a description of an NFC NDEF Bluetooth Carrier
 *  Configuration LE Record.
 *
 * This macro declares and initializes an instance of an NFC NDEF record
 * description for a Bluetooth Carrier Configuration LE record.
 *
 * @note The record descriptor is declared as automatic variable, which implies
 * that the NDEF message encoding must be done in the same variable scope.
 *
 * @param name         Name for accessing the record descriptor.
 * @param payload_id   NDEF record header Payload ID field (limited to one byte).
 *                     If 0, no ID is present in the record description.
 * @param payload_desc Pointer to the description of the payload. This data is
 *                     used to create the record payload.
 */
#define NFC_NDEF_LE_OOB_RECORD_DESC_DEF(name,                                 \
					payload_id,                           \
					payload_desc)                         \
	u8_t name##_nfc_ndef_le_oob_rec_id     = (payload_id);                \
	u8_t name##_nfc_ndef_le_oob_rec_id_len = ((payload_id) != 0) ? 1 : 0; \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(name,                                \
		TNF_MEDIA_TYPE,						      \
		&name##_nfc_ndef_le_oob_rec_id,				      \
		name##_nfc_ndef_le_oob_rec_id_len,			      \
		(nfc_ndef_le_oob_rec_type_field),			      \
		sizeof(nfc_ndef_le_oob_rec_type_field),			      \
		nfc_ndef_le_oob_rec_payload_constructor,		      \
		(payload_desc))

/**
 * @brief Macro for accessing the NFC NDEF Bluetooth Carrier Configuration LE
 * record descriptor instance that was created with
 * @ref NFC_NDEF_LE_OOB_RECORD_DESC_DEF.
 */
#define NFC_NDEF_LE_OOB_RECORD_DESC(NAME) NFC_NDEF_GENERIC_RECORD_DESC(NAME)

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NFC_NDEF_LE_OOB_REC_H_ */
