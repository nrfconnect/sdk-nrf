/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_MSG_H_
#define NFC_NDEF_MSG_H_

#include <zephyr/types.h>
#include <nfc/ndef/record.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file
 *
 * @defgroup nfc_ndef_msg Custom NDEF messages
 * @{
 * @ingroup  nfc_modules
 *
 * @brief    Generation of NFC NDEF messages for the NFC tag.
 *
 */

/**
 * @brief NDEF message descriptor.
 */
struct nfc_ndef_msg_desc {
	/** Pointer to an array of pointers to NDEF record descriptors. */
	struct nfc_ndef_record_desc const **record;
	/** Number of elements in the allocated record array, which defines
	 *  the maximum number of records within the NDEF message.
	 */
	uint32_t max_record_count;
	/** Number of records in the NDEF message. */
	uint32_t record_count;
};

/**
 * @brief Encode an NDEF message.
 *
 * This function encodes an NDEF message according to the provided message
 * descriptor.
 *
 * @param ndef_msg_desc Pointer to the message descriptor.
 * @param msg_buffer Pointer to the message destination. If NULL, function
 * will calculate the expected size of the message.
 * @param msg_len Size of the available memory for the message as input. Size
 * of the generated message as output.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_msg_encode(struct nfc_ndef_msg_desc const *ndef_msg_desc,
			uint8_t *msg_buffer,
			uint32_t *msg_len);

/**
 * @brief Clear an NDEF message.
 *
 * This function clears an NDEF message descriptor, thus empties the NDEF
 * message.
 *
 * @param msg Pointer to the message descriptor.
 */
void nfc_ndef_msg_clear(struct nfc_ndef_msg_desc *msg);

/**
 * @brief Add a record to an NDEF message.
 *
 * @param record Pointer to the record descriptor.
 * @param msg Pointer to the message descriptor.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_msg_record_add(struct nfc_ndef_msg_desc *msg,
			    struct nfc_ndef_record_desc const *record);

/**
 * @brief Macro for creating and initializing an NFC NDEF message descriptor.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_msg_desc and an array of pointers to record descriptors
 * (@ref nfc_ndef_record_desc) used by the message.
 *
 * Use the macro @ref NFC_NDEF_MSG to access the NDEF message descriptor
 * instance.
 *
 * @note The message descriptor is declared as automatic variable, which implies
 * that the NDEF message encoding must be done in the same variable scope.
 *
 * @param name Name of the related instance.
 * @param max_record_cnt Maximal count of records in the message.
 */
#define NFC_NDEF_MSG_DEF(name, max_record_cnt)				    \
	struct nfc_ndef_record_desc const				    \
		*name##_nfc_ndef_record_desc_array[max_record_cnt];	    \
	struct nfc_ndef_msg_desc name##_nfc_ndef_msg_desc =		    \
	{								    \
		.record = name##_nfc_ndef_record_desc_array,		    \
		.max_record_count = max_record_cnt,			    \
		.record_count = 0					    \
	}

/** @brief Macro for accessing the NFC NDEF message descriptor instance
 *  that you created with @ref NFC_NDEF_MSG_DEF.
 */
#define NFC_NDEF_MSG(name) (name##_nfc_ndef_msg_desc)

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor with
 * an encapsulated NDEF message.
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc that contains an encapsulated NDEF message as
 * payload. @ref nfc_ndef_msg_encode is used as payload constructor to encode
 * the message. The encoded message is then used as payload for the record.
 *
 * Use the macro @ref NFC_NDEF_NESTED_NDEF_MSG_RECORD to access the NDEF record
 * descriptor instance.
 *
 * @note The message descriptor is declared as automatic variable, which implies
 * that the NDEF message encoding must be done in the same variable scope.
 *
 * @param name Name of the created record descriptor instance.
 * @param tnf_arg Type Name Format (TNF) value for the record.
 * @param id_arg Pointer to the ID string.
 * @param id_len Length of the ID string.
 * @param type_arg Pointer to the type string.
 * @param type_len Length of the type string.
 * @param nested_message Pointer to the message descriptor to encapsulate
 * as the record's payload.
 */
#define NFC_NDEF_NESTED_NDEF_MSG_RECORD_DEF(name,			\
					    tnf_arg,			\
					    id_arg,			\
					    id_len,			\
					    type_arg,			\
					    type_len,			\
					    nested_message)		\
	struct nfc_ndef_record_desc name##_ndef_record_nested_desc =	\
	{								\
		.tnf = tnf_arg,						\
		.id_length = id_len,					\
		.id =  id_arg,						\
		.type_length = type_len,				\
		.type = type_arg,					\
		.payload_constructor =					\
			(payload_constructor_t)(nfc_ndef_msg_encode),	\
		.payload_descriptor = (void *) (nested_message)		\
	}

/** @brief Macro for accessing the NFC NDEF record descriptor instance
 *  that you created with @ref NFC_NDEF_NESTED_NDEF_MSG_RECORD_DEF.
 */
#define NFC_NDEF_NESTED_NDEF_MSG_RECORD(name) (name##_ndef_record_nested_desc)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NFC_NDEF_MSG_H_*/
