/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_T4T_TLV_BLOCK_H_
#define NFC_T4T_TLV_BLOCK_H_

/**@file
 *
 * @defgroup nfc_t4t_tlv_block File Control TLV block parser for Type 4 Tag.
 * @{
 * @brief File Control TLV block parser for Type 4 Tag (T4T).
 */

#include <stdint.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@defgroup nfc_t4t_tlv_block_file_access Possible values of file
 *                                         read/write access condition field.
 * @{
 */

/** Read access granted without any security. */
#define NFC_T4T_TLV_BLOCK_CONTROL_FILE_READ_ACCESS_GRANTED 0x00

/** Write access granted without any security. */
#define NFC_T4T_TLV_BLOCK_CONTROL_FILE_WRITE_ACCESS_GRANTED  0x00

/** No write access granted without any security (read-only). */
#define NFC_T4T_TLV_BLOCK_CONTROL_FILE_WRITE_ACCESS_DISABLED 0xFF

/**
 *@}
 */

/**@brief Possible types of File Control TLV for Type 4 Tag.
 */
enum nfc_t4t_tlv_block_type {
	/** Control information concerning the EF file with short NDEF message. */
	NFC_T4T_TLV_BLOCK_TYPE_NDEF_FILE_CONTROL_TLV = 0x04,

	/** Control information concerning the Proprietary file with proprietary data. */
	NFC_T4T_TLV_BLOCK_TYPE_PROPRIETARY_FILE_CONTROL_TLV = 0x05,

	/** Control information concerning the EF file with long NDEF message. */
	NFC_T4T_TLV_BLOCK_TYPE_EXTENDED_NDEF_FILE_CONTROL_TLV = 0x06
};

/**@brief File content descriptor.
 */
struct nfc_t4t_tlv_block_file {
	/** Pointer to the file content. */
	uint8_t *content;

	/** Length of file content. */
	uint16_t len;
};

/**@brief Extended NDEF/NDEF/Proprietary File Control Value descriptor.
 */
struct nfc_t4t_tlv_block_file_control_val {

	/** Pointer to the described file content. */
	struct nfc_t4t_tlv_block_file file;

	/** Maximum size (in bytes) of the file. */
	uint32_t max_file_size;

	/** File identifier. */
	uint16_t file_id;

	/** File read access condition. */
	uint8_t read_access;

	/** File write access condition. */
	uint8_t write_access;
};

/**@brief File Control TLV block descriptor.
 */
struct nfc_t4t_tlv_block {
	/** Value field descriptor. */
	struct nfc_t4t_tlv_block_file_control_val value;

	/** Length of the value field. */
	uint16_t length;

	/** Type of the TLV block. */
	uint8_t type;
};

/**@brief Function for parsing raw data of File Control TLV, read from
 *        a Type 4 Tag.
 *
 *  This function parses raw data of File Control TLV and stores the results in its
 *  descriptor.
 *
 *  @param[in,out] file_control_tlv Pointer to the File Control TLV that will
 *                                  be filled with parsed data.
 *  @param[in] raw_data Pointer to the buffer with raw TLV data.
 *  @param[in,out] len In: Buffer length with TLV blocks.
 *                     Out: Total length of first identified TLV
 *                     within the buffer.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_tlv_block_parse(struct nfc_t4t_tlv_block *file_control_tlv,
			    const uint8_t *raw_data,
			    uint16_t *len);

/**@brief Function for printing TLV block descriptor.
 *
 *  This function prints TLV block descriptor.
 *
 *  @param[in] num TLV block number.
 *  @param[in] t4t_tlv_block Pointer to the TLV block descriptor.
 */
void nfc_t4t_tlv_block_printout(uint8_t num, const struct nfc_t4t_tlv_block *t4t_tlv_block);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* NFC_T4T_TLV_BLOCK_H_ */
