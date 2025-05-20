/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <nfc/t4t/tlv_block.h>
#include <errno.h>

LOG_MODULE_DECLARE(nfc_t4t_cc_file, CONFIG_NFC_T4T_CC_FILE_LOG_LEVEL);

/* Length of a type field. */
#define TLV_TYPE_FIELD_LEN 1U

/* Length of a short length field. */
#define TLV_LEN_SHORT_FIELD_LEN 1U

/* Length of an extended length field. */
#define TLV_LEN_LONG_FIELD_LEN 3U

/* Value indicating the use of an extended length field. */
#define TLV_LEN_LONG_FORMAT_TOKEN 0xFF

/* Size of long format token. */
#define TLV_LEN_LONG_FORMAT_TOKEN_SIZE 1U

/* The minimal value of length field that can be used in long format. */
#define TLV_LEN_LONG_FORMAT_MIN_VALUE 0xFF

#define TLV_MIN_TL_FIELD_LEN (TLV_TYPE_FIELD_LEN + TLV_LEN_SHORT_FIELD_LEN)
#define TLV_MIN_LONG_FORMAT_TL_FIELD_LEN (TLV_TYPE_FIELD_LEN + TLV_LEN_LONG_FIELD_LEN)
#define TLV_MIN_VALUE_FIELD_SIZE 6U

#define FILE_CONTROL_FILE_ID_FIELD_SIZE 2U
#define FILE_CONTROL_READ_ACCESS_FIELD_SIZE 1U
#define FILE_CONTROL_WRITE_ACCESS_FIELD_SIZE 1U
#define FILE_CONTROL_COMMON_FIELDS_SIZE (FILE_CONTROL_FILE_ID_FIELD_SIZE        \
					 + FILE_CONTROL_READ_ACCESS_FIELD_SIZE  \
					 + FILE_CONTROL_WRITE_ACCESS_FIELD_SIZE)

#define FILE_ID_INVALID_VALUE_0 0x0000
#define FILE_ID_INVALID_VALUE_1 0xE102
#define FILE_ID_INVALID_VALUE_2 0xE103
#define FILE_ID_INVALID_VALUE_3 0x3F00
#define FILE_ID_INVALID_VALUE_4 0x3FFF
#define FILE_ID_INVALID_VALUE_5 0xFFFF

#define NDEF_FILE_MAX_SIZE_FIELD_SIZE 2U
#define NDEF_FILE_MAX_SIZE_MIN_VAL 0x0005
#define NDEF_FILE_MAX_SIZE_MAX_VAL 0xFFFE
#define NDEF_FILE_CONTROL_TLV_LEN (FILE_CONTROL_COMMON_FIELDS_SIZE \
				   + NDEF_FILE_MAX_SIZE_FIELD_SIZE)

#define PROPRIETARY_FILE_MAX_SIZE_FIELD_SIZE 2U
#define PROPRIETARY_FILE_MAX_SIZE_MIN_VAL 0x0003
#define PROPRIETARY_FILE_MAX_SIZE_MAX_VAL 0xFFFE
#define PROPRIETARY_FILE_CONTROL_TLV_LEN (FILE_CONTROL_COMMON_FIELDS_SIZE        \
					  + PROPRIETARY_FILE_MAX_SIZE_FIELD_SIZE)

#define EXTENDED_NDEF_FILE_MAX_SIZE_FIELD_SIZE 4U
#define EXTENDED_NDEF_FILE_MAX_SIZE_MIN_VAL 0x0000FFFFUL
#define EXTENDED_NDEF_FILE_MAX_SIZE_MAX_VAL 0xFFFFFFFEUL
#define EXTENDED_NDEF_FILE_CONTROL_TLV_LEN (FILE_CONTROL_COMMON_FIELDS_SIZE          \
					    + EXTENDED_NDEF_FILE_MAX_SIZE_FIELD_SIZE)

/** @brief Validates maximum file size field range. This field is present
 *         in every File Control TLV.
 */
static bool max_field_range_verify(uint32_t value, uint32_t min, uint32_t max)
{
	return !((value < min) || (value > max));
}


/** @brief Function for validating all possible types of File Control TLV.
 */
static int nfc_t4t_file_control_tl_validate(struct nfc_t4t_tlv_block *file_control_tlv)
{
	switch (file_control_tlv->type) {
	case NFC_T4T_TLV_BLOCK_TYPE_NDEF_FILE_CONTROL_TLV:
		if (file_control_tlv->length != NDEF_FILE_CONTROL_TLV_LEN) {
			return -EINVAL;
		}

		return 0;

	case NFC_T4T_TLV_BLOCK_TYPE_PROPRIETARY_FILE_CONTROL_TLV:
		if (file_control_tlv->length != PROPRIETARY_FILE_CONTROL_TLV_LEN) {
			return -EINVAL;
		}

		return 0;

	case NFC_T4T_TLV_BLOCK_TYPE_EXTENDED_NDEF_FILE_CONTROL_TLV:
		if (file_control_tlv->length != EXTENDED_NDEF_FILE_CONTROL_TLV_LEN) {
			return -EINVAL;
		}

		return 0;

	default:
		return -EINVAL;
	}
}


/**@brief Function for parsing value field of File Control TLV.
 */
static int nfc_t4t_file_control_value_parse(struct nfc_t4t_tlv_block *file_control_tlv,
					    const uint8_t *value_buff)
{
	struct nfc_t4t_tlv_block_file_control_val *control_tlv_val;

	/* Handle File Identifier field. */
	control_tlv_val = &file_control_tlv->value;
	control_tlv_val->file_id = sys_get_be16(value_buff);
	value_buff += FILE_CONTROL_FILE_ID_FIELD_SIZE;

	switch (control_tlv_val->file_id) {
	case FILE_ID_INVALID_VALUE_0:
	case FILE_ID_INVALID_VALUE_1:
	case FILE_ID_INVALID_VALUE_2:
	case FILE_ID_INVALID_VALUE_3:
	case FILE_ID_INVALID_VALUE_4:
	case FILE_ID_INVALID_VALUE_5:
		return -EINVAL;

	default:
		 break;
	}

	/* Handle Max file size field. */
	switch (file_control_tlv->type) {
	case NFC_T4T_TLV_BLOCK_TYPE_NDEF_FILE_CONTROL_TLV:
		control_tlv_val->max_file_size = sys_get_be16(value_buff);
		value_buff += NDEF_FILE_MAX_SIZE_FIELD_SIZE;

		if (!max_field_range_verify(control_tlv_val->max_file_size,
					    NDEF_FILE_MAX_SIZE_MIN_VAL,
					    NDEF_FILE_MAX_SIZE_MAX_VAL)) {
			return -EINVAL;
		}

		break;

	case NFC_T4T_TLV_BLOCK_TYPE_PROPRIETARY_FILE_CONTROL_TLV:
		control_tlv_val->max_file_size = sys_get_be16(value_buff);
		value_buff += PROPRIETARY_FILE_MAX_SIZE_FIELD_SIZE;

		if (!max_field_range_verify(control_tlv_val->max_file_size,
					    PROPRIETARY_FILE_MAX_SIZE_MIN_VAL,
					    PROPRIETARY_FILE_MAX_SIZE_MAX_VAL)) {
			return -EINVAL;
		}

		break;

	case NFC_T4T_TLV_BLOCK_TYPE_EXTENDED_NDEF_FILE_CONTROL_TLV:
		control_tlv_val->max_file_size = sys_get_be32(value_buff);
		value_buff += EXTENDED_NDEF_FILE_MAX_SIZE_FIELD_SIZE;

		if (!max_field_range_verify(control_tlv_val->max_file_size,
					    EXTENDED_NDEF_FILE_MAX_SIZE_MIN_VAL,
					    EXTENDED_NDEF_FILE_MAX_SIZE_MAX_VAL)) {
			return -EINVAL;
		}

		break;
	}

	/* Handle read access condition field. */
	control_tlv_val->read_access = *value_buff;
	value_buff += FILE_CONTROL_READ_ACCESS_FIELD_SIZE;

	/* Handle write access condition field. */
	control_tlv_val->write_access = *value_buff;

	return 0;
}


int nfc_t4t_tlv_block_parse(struct nfc_t4t_tlv_block *file_control_tlv,
			    const uint8_t *raw_data,
			    uint16_t *len)
{
	int err;
	const uint8_t *offset = raw_data;

	if (*len < TLV_MIN_TL_FIELD_LEN) {
		return -ENOMEM;
	}

	memset(file_control_tlv, 0, sizeof(*file_control_tlv));

	/* Handle type field of TLV block. */
	file_control_tlv->type = *offset;
	offset += TLV_TYPE_FIELD_LEN;

	/* Handle length field of TLV block. */
	if (*offset == TLV_LEN_LONG_FORMAT_TOKEN) {
		if (*len < TLV_MIN_LONG_FORMAT_TL_FIELD_LEN) {
			return -ENOMEM;
		}

		file_control_tlv->length = sys_get_be16(offset + TLV_LEN_LONG_FORMAT_TOKEN_SIZE);
		offset += TLV_LEN_LONG_FIELD_LEN;

		if (file_control_tlv->length < TLV_LEN_LONG_FORMAT_MIN_VALUE) {
			return -EINVAL;
		}
	} else {
		file_control_tlv->length = *offset;
		offset += TLV_LEN_SHORT_FIELD_LEN;
	}

	/* Calculate the total TLV block size. */
	uint16_t tlv_block_len = (offset - raw_data) + file_control_tlv->length;

	if (*len < tlv_block_len) {
		return -ENOMEM;
	}

	*len = tlv_block_len;

	/* Validate if type and length fields contain values
	 * supported by Type 4 Tag.
	 */
	err = nfc_t4t_file_control_tl_validate(file_control_tlv);
	if (err) {
		return err;
	}

	/* Handle value field of TLV block. */
	return nfc_t4t_file_control_value_parse(file_control_tlv, offset);
}


void nfc_t4t_tlv_block_printout(uint8_t num, const struct nfc_t4t_tlv_block *t4t_tlv_block)
{
	LOG_INF("%d file Control TLV", num);
	switch (t4t_tlv_block->type) {
	case NFC_T4T_TLV_BLOCK_TYPE_NDEF_FILE_CONTROL_TLV:
		LOG_INF("Type: NDEF File Control (0x%02x)",
			t4t_tlv_block->type);

		break;

	case NFC_T4T_TLV_BLOCK_TYPE_PROPRIETARY_FILE_CONTROL_TLV:
		LOG_INF("Type: Proprietary File Control (0x%02x)",
			t4t_tlv_block->type);

		break;

	case NFC_T4T_TLV_BLOCK_TYPE_EXTENDED_NDEF_FILE_CONTROL_TLV:
		LOG_INF("Type: Extended NDEF File Control (0x%02x)",
			t4t_tlv_block->type);

		break;

	default:
		LOG_INF("Type: Unknown (0x%02x)", t4t_tlv_block->type);
	}

	LOG_INF("Length (in bytes): %d", t4t_tlv_block->length);

	const struct nfc_t4t_tlv_block_file_control_val *tlv_val =
		&t4t_tlv_block->value;

	LOG_INF("File Identifier: 0x%04X ", tlv_val->file_id);
	LOG_INF("Maximum file size: %d ", tlv_val->max_file_size);
	LOG_INF("Read access condition: 0x%02X ", tlv_val->read_access);
	LOG_INF("Write access condition: 0x%02x ", tlv_val->write_access);

	if (tlv_val->file.content) {
		LOG_INF("NDEF file content present. Length: %d",
			tlv_val->file.len);
		LOG_HEXDUMP_INF(tlv_val->file.content, tlv_val->file.len,
				"NDEF file content:\n");
	} else {
		LOG_INF("NDEF file content is not present");
	}
}
