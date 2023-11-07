/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_TEXT_REC_H_
#define NFC_NDEF_TEXT_REC_H_

/**@file
 *
 * @defgroup nfc_text_rec Text records
 * @{
 * @ingroup  nfc_ndef_messages
 *
 * @brief    Generation of NFC NDEF Text record descriptions.
 *
 */

#include <zephyr/types.h>
#include <nfc/ndef/record.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of the Unicode Transformation Format.
 *
 * Values to specify the type of UTF for an NFC NDEF Text record.
 */
enum nfc_ndef_text_rec_utf {
	/** Unicode Transformation Format 8. */
	UTF_8  = 0,
	/** Unicode Transformation Format 16. */
	UTF_16 = 1,
};

/**
 * @brief Text record payload descriptor.
 */
struct nfc_ndef_text_rec_payload {
	/** Type of the Unicode Transformation Format. */
	enum nfc_ndef_text_rec_utf utf;
	/** Pointer to the IANA language code. */
	uint8_t const *lang_code;
	/** Length of the IANA language code. */
	uint8_t lang_code_len;
	/** Pointer to the user text. */
	uint8_t const *data;
	/** Length of the user text. */
	uint32_t data_len;
};

/**
 * @brief Constructor for an NFC NDEF Text record payload.
 *
 * @param nfc_rec_text_payload_desc Pointer to the Text record description.
 * @param buff Pointer to the payload destination. If NULL, function will
 * calculate the expected size of the Text record payload.
 * @param len Size of the available memory to write as input. Size of the
 * generated record payload as output.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_text_rec_payload_encode(
		struct nfc_ndef_text_rec_payload *nfc_rec_text_payload_desc,
		uint8_t *buff,
		uint32_t *len);

/**
 * @brief External reference to the type field of the Text record, defined in
 * the file @c text_rec.c. It is used in the
 * @ref NFC_NDEF_TEXT_RECORD_DESC_DEF macro.
 */
extern const uint8_t nfc_ndef_text_rec_type_field[];

/**
 * @brief Size of the type field of the Text record, defined in the file
 * @c text_rec.c. It is used in the @ref NFC_NDEF_TEXT_RECORD_DESC_DEF
 * macro.
 */
#define NFC_NDEF_TEXT_REC_TYPE_LENGTH 1

/**
 *@brief Macro for creating and initializing an NFC NDEF record descriptor for
 *	 a Text record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc and an instance of type
 * @ref nfc_ndef_text_rec_payload, which together constitute an instance of
 * a Text record.
 *
 * Use the macro @ref NFC_NDEF_TEXT_RECORD_DESC to access the NDEF Text record
 * descriptor instance.
 *
 * @param name Name of the created record descriptor instance.
 * @param utf_arg Unicode Transformation Format.
 * @param lang_code_arg Pointer to the IANA language code.
 * @param lang_code_len_arg Length of the IANA language code.
 * @param data_arg Pointer to the user text.
 * @param data_len_arg Length of the user text.
 */
#define NFC_NDEF_TEXT_RECORD_DESC_DEF(name,				      \
				      utf_arg,				      \
				      lang_code_arg,			      \
				      lang_code_len_arg,		      \
				      data_arg,				      \
				      data_len_arg)			      \
	struct nfc_ndef_text_rec_payload name##_nfc_ndef_text_rec_payload =   \
	{								      \
		.utf = utf_arg,						      \
		.lang_code = lang_code_arg,				      \
		.lang_code_len = lang_code_len_arg,			      \
		.data = data_arg,					      \
		.data_len = data_len_arg,				      \
	};								      \
	NFC_NDEF_GENERIC_RECORD_DESC_DEF(name,				      \
					 TNF_WELL_KNOWN,		      \
					 0,				      \
					 0,				      \
					 nfc_ndef_text_rec_type_field,	      \
					 NFC_NDEF_TEXT_REC_TYPE_LENGTH,	      \
					 nfc_ndef_text_rec_payload_encode,    \
					 &(name##_nfc_ndef_text_rec_payload))

/**
 * @brief Macro for accessing the NFC NDEF Text record descriptor
 * instance that was created with @ref NFC_NDEF_TEXT_RECORD_DESC_DEF.
 */
#define NFC_NDEF_TEXT_RECORD_DESC(name) NFC_NDEF_GENERIC_RECORD_DESC(name)

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* NFC_NDEF_TEXT_REC_H_ */
