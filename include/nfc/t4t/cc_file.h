/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_T4T_CC_FILE_H_
#define NFC_T4T_CC_FILE_H_

/**@file
 *
 * @defgroup nfc_t4t_cc_file CC file parser
 * @{
 * @brief Capability Container file parser for Type 4 Tag.
 *
 */

#include <stdint.h>
#include <nfc/t4t/tlv_block.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Descriptor for the Capability Container (CC) file of Type 4 Tag. */
struct nfc_t4t_cc_file {
	/** Pointer to the array for TLV blocks. */
	struct nfc_t4t_tlv_block *tlv_block_array;

	/** Number of TLV blocks stored in the Type 4 Tag. */
	uint16_t tlv_count;

	/** Maximum number of TLV blocks. */
	uint16_t max_tlv_blocks;

	/** Size (bytes) of a Capability Container including this field. */
	uint16_t len;

	/** MLe field - maximum R-APDU data size (bytes). */
	uint16_t max_rapdu_size;

	/** MLc field - maximum C-APDU data size (bytes). */
	uint16_t max_capdu_size;

	/** Major version of the supported Type 4 Tag specification. */
	uint8_t major_version;

	/** Minor version of the supported Type 4 Tag specification. */
	uint8_t minor_version;
};

/** @brief Macro for creating and initializing a Type 4 Tag
 *         Capability Container descriptor.
 *
 *  This macro creates and initializes a static instance of a
 *  @ref nfc_t4t_cc_file structure and an array of @ref nfc_t4t_tlv_block
 *  descriptors.
 *
 *  Use the macro @ref NFC_T4T_CC_DESC to access the Type 4 Tag
 *  descriptor instance.
 *
 *  @param[in] _name Name of the created descriptor instance.
 *  @param[in] _max_blocks Maximum number of @ref nfc_t4t_tlv_block descriptors
 *                         that can be stored in the array.
 *
 */
#define NFC_T4T_CC_DESC_DEF(_name, _max_blocks)                               \
	static struct nfc_t4t_tlv_block _name##_tlv_block_array[_max_blocks]; \
	static struct nfc_t4t_cc_file _name##_type_4_tag = {                  \
		.max_tlv_blocks = _max_blocks,                                \
		.tlv_block_array = _name##_tlv_block_array,                   \
		.tlv_count = 0                                                \
	}

/** @brief Macro for accessing the @ref nfc_t4t_cc_file instance that
 *         was created with @ref NFC_T4T_CC_DESC_DEF.
 *
 *  @param[in] _name Name of the created descriptor instance.
 */
#define NFC_T4T_CC_DESC(_name) (_name##_type_4_tag)

/** @brief Function for parsing raw data of a CC file, read from a Type 4 Tag.
 *
 *  This function parses raw data of a Capability Container file and stores
 *  the results in its descriptor.
 *
 *  @param[in,out] t4t_cc_file Pointer to the CC file descriptor that will be
 *                             filled with parsed data.
 *  @param[in] raw_data Pointer to the buffer with raw data.
 *  @param[in] len Buffer length.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_cc_file_parse(struct nfc_t4t_cc_file *t4t_cc_file,
			  const uint8_t *raw_data,
			  uint16_t len);

/** @brief Function for finding File Control TLV block within the
 *         CC file descriptor.
 *
 *  This function finds File Control TLV block that matches
 *  the specified file ID within the CC file descriptor.
 *
 *  @param[in] t4t_cc_file Pointer to the CC file descriptor.
 *  @param[in] file_id File identifier.
 *
 *  @retval TLV  Pointer to the File Control TLV.
 *  @retval NULL If TLV with the specified File ID was not found.
 */
struct nfc_t4t_tlv_block *nfc_t4t_cc_file_content_get(struct nfc_t4t_cc_file *t4t_cc_file,
						      uint16_t file_id);

/** @brief Function for binding a file with its File Control TLV block.
 *
 *  This function binds file content with its File Control TLV block, in which
 *  maximal file size and access conditions are stored.
 *
 *  @param[in,out] t4t_cc_file Pointer to the CC file descriptor.
 *  @param[in] file Pointer to File descriptor.
 *  @param[in] file_id File identifier.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_cc_file_content_set(struct nfc_t4t_cc_file *t4t_cc_file,
				const struct nfc_t4t_tlv_block_file *file,
				uint16_t file_id);

/** @brief Function for printing the CC file descriptor.
 *
 *  This function prints the CC file descriptor.
 *
 *  @param[in] t4t_cc_file Pointer to the CC file.
 */
void nfc_t4t_cc_file_printout(const struct nfc_t4t_cc_file *t4t_cc_file);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* NFC_T4T_CC_FILE_H_ */
