/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_NDEF_RECORD_H_
#define NFC_NDEF_RECORD_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@file
 *
 * @defgroup nfc_ndef_record Custom NDEF records
 * @{
 * @ingroup nfc_ndef_msg
 *
 * @brief Generation of NFC NDEF records for NFC messages.
 *
 */

/** Mask of the ID field presence bit in the flags byte of an NDEF record. */
#define NDEF_RECORD_IL_MASK                0x08
/** Mask of the TNF value field in the first byte of an NDEF record. */
#define NDEF_RECORD_TNF_MASK               0x07
/** Mask of the SR flag. If set, this flag indicates that the PAYLOAD_LENGTH
 *  field has a size of 1 byte. Otherwise, PAYLOAD_LENGTH has 4 bytes.
 */
#define NDEF_RECORD_SR_MASK                0x10
/** Size of the Payload Length field in a long NDEF record. */
#define NDEF_RECORD_PAYLOAD_LEN_LONG_SIZE  4
/** Size of the Payload Length field in a short NDEF record. */
#define NDEF_RECORD_PAYLOAD_LEN_SHORT_SIZE 1
#define NDEF_RECORD_ID_LEN_SIZE            1

/**
 * @brief Payload constructor type.

 * A payload constructor is a function for constructing the payload of an NDEF
 * record.
 *
 * @param payload_descriptor Pointer to the input data for the constructor.
 * @param buffer Pointer to the payload destination. If NULL, function will
 *  calculate the expected size of the record payload. *
 * @param len Size of the available memory to write as input. Size of the
 * generated record payload as output. The implementation must check if the
 * payload will fit in the provided buffer. This must be checked by the caller
 * function.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
typedef int (*payload_constructor_t)(void *payload_descriptor,
				     uint8_t *buffer,
				     uint32_t *len);
/**
 * @brief Type Name Format (TNF) Field Values.
 *
 * Values to specify the TNF of a record.
 */
enum nfc_ndef_record_tnf {
	/** The value indicates that there is no type or payload associated
	 *  with this record.
	 */
	TNF_EMPTY         = 0x00,
	/** NFC Forum well-known type [NFC RTD]. */
	TNF_WELL_KNOWN    = 0x01,
	/** Media-type as defined in RFC 2046 [RFC 2046]. */
	TNF_MEDIA_TYPE    = 0x02,
	/** Absolute URI as defined in RFC 3986 [RFC 3986]. */
	TNF_ABSOLUTE_URI  = 0x03,
	/** NFC Forum external type [NFC RTD]. */
	TNF_EXTERNAL_TYPE = 0x04,
	/** The value indicates that there is no type associated with this
	 *  record.
	 */
	TNF_UNKNOWN_TYPE  = 0x05,
	/** The value is used for the record chunks used in chunked payload. */
	TNF_UNCHANGED     = 0x06,
	/** The value is reserved for future use. */
	TNF_RESERVED      = 0x07,
};

/**
 * @brief NDEF record descriptor.
 */
struct nfc_ndef_record_desc {
	/** Value of the Type Name Format (TNF) field. */
	enum nfc_ndef_record_tnf tnf;
	/** Length of the ID field. If 0, a record format without ID field is
	 *  assumed.
	 */
	uint8_t id_length;
	/** Pointer to the ID field data. Not relevant if id_length is 0. */
	uint8_t const *id;
	/** Length of the type field.*/
	uint8_t type_length;
	/** Pointer to the type field data. Not relevant if type_length is 0. */
	uint8_t const *type;
	/** Pointer to the payload constructor function. */
	payload_constructor_t payload_constructor;
	/** Pointer to the data for the payload constructor function. */
	void *payload_descriptor;
};

/**
 * @brief Record position within the NDEF message.
 *
 * Values to specify the location of a record within the NDEF message.
 */
enum nfc_ndef_record_location {
	NDEF_FIRST_RECORD  = 0x80, /**< First record. */
	NDEF_MIDDLE_RECORD = 0x00, /**< Middle record. */
	NDEF_LAST_RECORD   = 0x40, /**< Last record. */
	NDEF_LONE_RECORD   = 0xC0  /**< Only one record in the message. */
};

/** Mask of the Record Location bits in the NDEF record's flags byte. */
#define NDEF_RECORD_LOCATION_MASK (NDEF_LONE_RECORD)

/**
 * @brief Binary data descriptor containing the payload for the record.
 */
struct nfc_ndef_bin_payload_desc {
	uint8_t const *payload;  /**< Pointer to the buffer with the data. */
	uint32_t payload_length; /**< Length of data in bytes. */
};

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor for
 * a generic record.
 *
 * This macro creates and initializes an instance of type
 * @ref nfc_ndef_record_desc.
 *
 * Use the macro @ref NFC_NDEF_GENERIC_RECORD_DESC to access the NDEF record
 * descriptor instance.
 *
 * @note The record descriptor is declared as automatic variable, which implies
 * that the NDEF record encoding must be done in the same variable scope.
 *
 * @param name Name of the created descriptor instance.
 * @param tnf_arg Type Name Format (TNF) value for the record.
 * @param id_arg Pointer to the ID string.
 * @param id_len Length of the ID string.
 * @param type_arg Pointer to the type string.
 * @param type_len Length of the type string.
 * @param payload_constructor_arg Pointer to the payload constructor function.
 * The constructor must be of type @ref payload_constructor_t.
 * @param payload_descriptor_arg Pointer to the data for the payload
 * constructor.
 */
#define NFC_NDEF_GENERIC_RECORD_DESC_DEF(name,				    \
					 tnf_arg,			    \
					 id_arg,			    \
					 id_len,			    \
					 type_arg,			    \
					 type_len,			    \
					 payload_constructor_arg,	    \
					 payload_descriptor_arg)	    \
	struct nfc_ndef_record_desc name##_ndef_generic_record_desc =	    \
	{								    \
		.tnf = tnf_arg,						    \
		.id_length = id_len,					    \
		.id = id_arg,						    \
		.type_length = type_len,				    \
		.type = type_arg,					    \
		.payload_constructor  =					    \
			(payload_constructor_t)payload_constructor_arg,	    \
		.payload_descriptor = (void *) payload_descriptor_arg	    \
	}

/** @brief Macro for accessing the NFC NDEF record descriptor instance that you
 *  created with @ref NFC_NDEF_GENERIC_RECORD_DESC_DEF.
 */
#define NFC_NDEF_GENERIC_RECORD_DESC(name) (name##_ndef_generic_record_desc)

/**
 * @brief Macro for creating and initializing an NFC NDEF record descriptor for
 * a record with binary payload.
 *
 * This macro creates and initializes an instance of type nfc_ndef_record_desc
 * and a binary data descriptor containing the payload data.
 *
 * Use the macro @ref NFC_NDEF_RECORD_BIN_DATA to access the NDEF record
 * descriptor instance.
 *
 * @note The record descriptor is declared as automatic variable, which implies
 * that the NDEF record encoding must be done in the same variable scope.
 *
 * @param name Name of the created descriptor instance.
 * @param tnf_arg Type Name Format (TNF) value for the record.
 * @param id_arg Pointer to the ID string.
 * @param id_len Length of the ID string.
 * @param type_arg Pointer to the type string.
 * @param type_len Length of the type string.
 * @param payload_arg Pointer to the payload data that will be copied to the
 * payload field.
 * @param payload_len Length of the payload.
 */
#define NFC_NDEF_RECORD_BIN_DATA_DEF(name,				    \
				     tnf_arg,				    \
				     id_arg,				    \
				     id_len,				    \
				     type_arg,				    \
				     type_len,				    \
				     payload_arg,			    \
				     payload_len)			    \
	struct nfc_ndef_bin_payload_desc name##_nfc_ndef_bin_payload_desc = \
	{								    \
		.payload  = payload_arg,				    \
		.payload_length = payload_len				    \
	};								    \
									    \
	struct nfc_ndef_record_desc name##_nfc_ndef_bin_record_desc =	    \
	{								    \
		.tnf = tnf_arg,						    \
		.id_length = id_len,					    \
		.id = id_arg,						    \
		.type_length = type_len,				    \
		.type = type_arg,					    \
		.payload_constructor  =					    \
		    (payload_constructor_t) nfc_ndef_bin_payload_memcopy,   \
		.payload_descriptor =					    \
		    (void *) &name##_nfc_ndef_bin_payload_desc		    \
	}

/** @brief Macro for accessing the NFC NDEF record descriptor instance
 *  that you created with @ref NFC_NDEF_RECORD_BIN_DATA_DEF.
 */
#define NFC_NDEF_RECORD_BIN_DATA(name) (name##_nfc_ndef_bin_record_desc)

/** @brief Macro for accessing the binary data descriptor that contains
 *  the payload of the record that you created with
 *  @ref NFC_NDEF_RECORD_BIN_DATA_DEF.
 */
#define NFC_NDEF_BIN_PAYLOAD_DESC(name) (name##_nfc_ndef_bin_payload_desc)

/**
 * @brief Encode an NDEF record.
 *
 * @details This function encodes an NDEF record according to the provided
 * record descriptor.
 *
 * @param ndef_record_desc Pointer to the record descriptor.
 * @param record_location Location of the record within the NDEF message.
 * @param record_buffer Pointer to the record destination. If NULL, function
 * will calculate the expected size of the record.
 * @param record_len Size of the available memory for the record as input.
 * Size of the generated record as output.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_record_encode(struct nfc_ndef_record_desc const *ndef_record_desc,
			   enum nfc_ndef_record_location const record_location,
			   uint8_t *record_buffer,
			   uint32_t *record_len);

/**
 * @brief Construct the payload for an NFC NDEF record from binary data.
 *
 * This function copies data from a binary buffer to the payload field of the
 * NFC NDEF record.
 *
 * @param payload_descriptor Pointer to the descriptor of the binary data
 * location and size.
 * @param buffer Pointer to the payload destination. If NULL, function will
 * calculate the expected size of the record payload.
 * @param len Size of the available memory for the payload as input. Size of
 * the copied payload as output.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_ndef_bin_payload_memcopy(
			struct nfc_ndef_bin_payload_desc *payload_descriptor,
			uint8_t *buffer,
			uint32_t *len);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* NFC_NDEF_RECORD_H_ */
