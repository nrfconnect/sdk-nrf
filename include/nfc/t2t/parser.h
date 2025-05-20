/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_T2T_PARSER_H_
#define NFC_T2T_PARSER_H_

#include <stdint.h>
#include <nfc/t2t/tlv_block.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup nfc_t2t_parser Type 2 Tag
 * @{
 * @brief NFC Type 2 Tag parser.
 */

/** @brief Descriptor for the internal bytes of a Type 2 Tag.
 */
struct nfc_t2t_sn {
	/** Manufacturer ID (the most significant byte of the
	 *  UID/serial number).
	 */
	uint8_t manufacturer_id;

	/** Bytes 5-4 of the tag UID. */
	uint16_t serial_number_part_1;

	/** Bytes 3-0 of the tag UID. */
	uint32_t serial_number_part_2;

	/** First block check character byte (XOR of the cascade tag byte,
	 *  manufacturer ID byte, and the serial_number_part_1 bytes).
	 */
	uint8_t check_byte_0;

	/** Second block check character byte
	 * (XOR of the serial_number_part_2 bytes).
	 */
	uint8_t check_byte_1;

	/** Tag internal bytes. */
	uint8_t internal;
};

/** @brief Descriptor for the Capability Container (CC) bytes of a Type 2 Tag.
 */
struct nfc_t2t_cc {
	/** Major version of the supported Type 2 Tag specification. */
	uint8_t major_version;

	/** Minor version of the supported Type 2 Tag specification. */
	uint8_t minor_version;

	/** Size of the data area in bytes. */
	uint16_t data_area_size;

	/** Read access for the data area. */
	uint8_t read_access;

	/** Write access for the data area. */
	uint8_t write_access;
};

/** @brief Type 2 Tag descriptor.
 */
struct nfc_t2t {
	/** Values within the serial number area of the tag. */
	struct nfc_t2t_sn sn;

	/** Value of the lock bytes. */
	uint16_t lock_bytes;

	/** Values within the Capability Container area of the tag. */
	struct nfc_t2t_cc cc;

	/** Maximum number of TLV blocks that can be stored. */
	const uint16_t max_tlv_blocks;

	/** Pointer to the array for TLV blocks. */
	struct nfc_t2t_tlv_block *tlv_block_array;

	/** Number of TLV blocks stored in the Type 2 Tag. */
	uint16_t tlv_count;
};

/** @brief Create and initialize a Type 2 Tag descriptor.
 *
 * This macro creates and initializes a static instance of an
 * @ref nfc_t2t structure and an array of @ref nfc_t2t_tlv_block descriptors.
 *
 * Use the macro @ref NFC_T2T_DESC to access the Type 2 Tag
 * descriptor instance.
 *
 * @param[in] _name          Name of the created descriptor instance.
 * @param[in] _max_blocks    Maximum number of @ref nfc_t2t_tlv_block
 *                           descriptors that can be stored in the array.
 *
 *
 */
#define NFC_T2T_DESC_DEF(_name, _max_blocks)                                   \
	static struct nfc_t2t_tlv_block  _name##_tlv_block_array[_max_blocks]; \
	static struct nfc_t2t NFC_T2T_DESC(_name) =                            \
	{                                                                      \
		.max_tlv_blocks = _max_blocks,                                 \
		.tlv_block_array = _name##_tlv_block_array,                    \
		.tlv_count = 0                                                 \
	}

/** @brief Access the @ref nfc_t2t instance that was created
 *         with @ref NFC_T2T_DESC_DEF.
 */
#define NFC_T2T_DESC(_name) (_name##_t2t)

/** Value indicating that the Type 2 Tag contains NFC Forum defined data. */
#define NFC_T2T_NFC_FORUM_DEFINED_DATA      0xE1

/** Value used for calculating the first BCC byte of a
 *  Type 2 Tag serial number.
 */
#define NFC_T2T_UID_BCC_CASCADE_BYTE        0x88


/** Supported major version of the Type 2 Tag specification. */
#define NFC_T2T_SUPPORTED_MAJOR_VERSION     1

/** Supported minor version of the Type 2 Tag specification. */
#define NFC_T2T_SUPPORTED_MINOR_VERSION     0

/** Type 2 Tag block size in bytes. */
#define NFC_T2T_BLOCK_SIZE                  4

/** Offset of the Capability Container area in the Type 2 Tag. */
#define NFC_T2T_CC_BLOCK_OFFSET             12

/** Offset of the data area in the Type 2 Tag. */
#define NFC_T2T_FIRST_DATA_BLOCK_OFFSET     16


/** @brief Clear the @ref nfc_t2t structure.
 *
 *  @param[in,out] t2t Pointer to the structure that should be cleared.
 *
 */
void nfc_t2t_clear(struct nfc_t2t *t2t);

/** @brief Parse raw data that was read from a Type 2 Tag.
 *
 *  This function parses the header and the following TLV blocks of a
 *  Type 2 Tag. The data is read from a buffer and stored in a
 *  @ref nfc_t2t structure.
 *
 *  @param[out] t2t            Pointer to the structure that will be filled
 *                             with parsed data.
 *  @param[in]  raw_data       Pointer to the buffer with raw data from the tag
 *                             (should point at the first byte of the first
 *                             block of the tag).
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_t2t_parse(struct nfc_t2t *t2t, const uint8_t *raw_data);

/** @brief Print parsed contents of the Type 2 Tag.
 *
 *  @param[in] t2t Pointer to the structure that should be printed.
 *
 */
void nfc_t2t_printout(const struct nfc_t2t *t2t);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* NFC_T2T_PARSER_H_ */
