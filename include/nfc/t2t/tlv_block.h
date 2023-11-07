/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_T2T_TLV_BLOCK_H_
#define NFC_T2T_TLV_BLOCK_H_

/**@file
 *
 * @defgroup nfc_t2t_tlv_block Type 2 Tag TLV blocks
 * @{
 * @ingroup  nfc_t2t_parser
 *
 * @brief Descriptor for a Type 2 Tag TLV block.
 *
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Tag field values.
 *
 * Possible values for the tag field in a TLV block.
 */
enum nfc_t2t_tlv_block_types {
	/** Can be used for padding of memory areas. */
	NFC_T2T_TLV_NULL = 0x00,

	/** Defines details of the lock bits. */
	NFC_T2T_TLV_LOCK_CONTROL = 0x01,

	/** Identifies reserved memory areas. */
	NFC_T2T_TLV_MEMORY_CONTROL = 0x02,

	/** Contains an NDEF message. */
	NFC_T2T_TLV_NDEF_MESSAGE = 0x03,

	/** Tag proprietary information. */
	NFC_T2T_TLV_PROPRIETARY = 0xFD,

	/** Last TLV block in the data area. */
	NFC_T2T_TLV_TERMINATOR = 0xFE
};

/** @brief TLV block descriptor.
 */
struct nfc_t2t_tlv_block {
	/** Type of the TLV block. */
	uint8_t tag;

	/** Length of the value field. */
	uint16_t length;

	/** Pointer to the value field (NULL if no value field is present
	 *  in the block).
	 */
	const uint8_t *value;
};

/** Length of a tag field. */
#define NFC_T2T_TLV_T_LENGTH        1

/** Length of a short length field. */
#define NFC_T2T_TLV_L_SHORT_LENGTH  1

/** Length of an extended length field. */
#define NFC_T2T_TLV_L_LONG_LENGTH   3

/** Value indicating the use of an extended length field. */
#define NFC_T2T_TLV_L_FORMAT_FLAG   0xFF

/** Predefined length of the NULL and TERMINATOR TLV blocks. */
#define NFC_T2T_TLV_NULL_TERMINATOR_LEN     0

/** Predefined length of the LOCK CONTROL and MEMORY CONTROL blocks. */
#define NFC_T2T_TLV_LOCK_MEMORY_CTRL_LEN    3

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_T2T_TLV_BLOCK_H_ */
