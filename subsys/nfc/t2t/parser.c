/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdbool.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <nfc/t2t/parser.h>

LOG_MODULE_REGISTER(nfc_t2t_parser);

/* Gets least significant nibble (a 4-bit value) from a byte. */
#define LSN_GET(val) (val & 0x0F)

/* Gets most significant nibble (a 4-bit value) from a byte. */
#define MSN_GET(val) ((val >> 4) & 0x0F)

/**
 * @brief Function for inserting the TLV block into a @ref nfc_t2t
 *        structure.
 *
 * The content of a TLV block structure pointed by the nfc_t2t_tlv_block is copied
 * into a TLV block array within the structure pointed by the t2t.
 *
 * @param[in,out] t2t Pointer to the structure that contains the
 *                    TLV blocks array.
 * @param[in] tlv_block Pointer to the TLV block to insert.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int nfc_t2t_tlv_block_insert(struct nfc_t2t *t2t,
				    struct nfc_t2t_tlv_block *tlv_block)
{
	if (t2t->tlv_count == t2t->max_tlv_blocks) {
		return -ENOMEM;
	}

	/* Copy contents of the source block. */
	t2t->tlv_block_array[t2t->tlv_count] = *tlv_block;
	t2t->tlv_count++;

	return 0;
}


/**
 * @brief Function for checking if the TLV block length is correct.
 *
 * Some TLV block has predefined length:
 * NFC_T2T_TLV_NULL and NFC_T2T_TLV_TERMINATOR always have a length of 1 byte.
 * NFC_T2T_TLV_LOCK_CONTROL and NFC_T2T_TLV_MEMORY_CONTROL always have a length
 * of 3 bytes.
 *
 * @param[in] block_to_check Pointer to the structure that contains
 *                           the TLV block length.
 *
 * @retval TRUE If the length is correct.
 * @retval FALSE  Otherwise.
 *
 */
static bool tlv_block_is_data_length_correct(struct nfc_t2t_tlv_block *block_to_check)
{
	switch (block_to_check->tag) {
	case NFC_T2T_TLV_NULL:
	case NFC_T2T_TLV_TERMINATOR:
		if (block_to_check->length != NFC_T2T_TLV_NULL_TERMINATOR_LEN) {
			return false;
		}

		break;

	case NFC_T2T_TLV_LOCK_CONTROL:
	case NFC_T2T_TLV_MEMORY_CONTROL:
		if (block_to_check->length != NFC_T2T_TLV_LOCK_MEMORY_CTRL_LEN) {
			return false;
		}

		break;

	case NFC_T2T_TLV_NDEF_MESSAGE:
	case NFC_T2T_TLV_PROPRIETARY:
	default:
	/* Any length will do. */
		break;
	}

	return true;
}

/**
 * @brief Function for checking if the end of the tag data area was reached.
 *
 * @param[in] t2t Pointer to the structure that contains
 *                the data area size.
 * @param[in] offset Current byte offset.
 *
 * @retval TRUE If the offset indicates the end of the data area.
 * @retval FALSE Otherwise.
 *
 */
static bool nfc_t2t_is_end_reached(struct nfc_t2t *t2t,
				   u16_t offset)
{
	return offset == (t2t->cc.data_area_size + NFC_T2T_FIRST_DATA_BLOCK_OFFSET);
}


/**
 * @brief Function for checking if version of Type 2 Tag specification read
 *        from a tag is supported.
 *
 * @param[in] t2t Pointer to the structure that contains
 *                the tag version.
 *
 * @retval TRUE If the version is supported and tag data can be parsed.
 * @retval FALSE Otherwise.
 *
 */
static bool nfc_t2t_is_version_supported(struct nfc_t2t *t2t)
{
	/* Simple check atm, as only 1 major version has been issued so far, so
	 * no backward compatibility is needed, tags with newer version
	 * implemented shall be rejected according to the doc.
	 */
	return t2t->cc.major_version == NFC_T2T_SUPPORTED_MAJOR_VERSION;
}


/**
 * @brief Function for checking if the field fits into the data area specified
 *        in the Capability Container.
 *
 * @param[in] t2t Pointer to the structure that contains
 *                the data area size.
 * @param[in] offset As Offset of the field to check.
 * @param[in] field_length Length of the field to check.
 *
 * @retval TRUE If the field fits into the data area.
 * @retval FALSE If the field exceeds the data area.
 *
 */
static bool nfc_t2t_is_field_within_data_range(struct nfc_t2t *t2t,
					       u16_t offset,
					       u16_t field_length)
{
	/* Invalid argument, return false. */
	if (field_length == 0) {
		return false;
	}

	return ((offset + field_length - 1) <
		(t2t->cc.data_area_size + NFC_T2T_FIRST_DATA_BLOCK_OFFSET))
	       && (offset >= NFC_T2T_FIRST_DATA_BLOCK_OFFSET);
}


/**
 * @brief Function for reading the tag field of a TLV block
 *        from the raw_data buffer.
 *
 * This function reads the tag field containing a TLV block type and inserts
 * its value into a structure pointed by the tlv_buf pointer.
 *
 * @param[in] t2t Pointer to the structure that contains Type 2 Tag
 *                data parsed so far.
 * @param[in] raw_data Pointer to the buffer with a raw data from the tag.
 * @param[in,out] t_offset As input: offset of the tag field to read.
 *                         As output: offset of the first byte after
 *                         the tag field.
 *  @param[out] tlv_buf Pointer to a @ref nfc_t2t_tlv_block structure where the tag
 *                      type will be inserted.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int nfc_t2t_type_extract(struct nfc_t2t *t2t,
				const u8_t *raw_data,
				u16_t *t_offset,
				struct nfc_t2t_tlv_block *tlv_buf)
{
	if (!nfc_t2t_is_field_within_data_range(t2t, *t_offset,
						NFC_T2T_TLV_T_LENGTH)) {
		return -EINVAL;
	}

	tlv_buf->tag = raw_data[*t_offset];
	*t_offset   += NFC_T2T_TLV_T_LENGTH;

	return 0;
}


/**
 * @brief Function for reading the length field of a TLV block from
 *        the raw_data buffer.
 *
 * This function reads the length field of a TLV block and inserts its value
 * into a structure pointed by the tlv_buf pointer.
 *
 * @param[in] t2t Pointer to the structure that contains Type 2 Tag
 *                data parsed so far.
 * @param[in] raw_data Pointer to the buffer with a raw data from the tag.
 * @param[in,out] l_offset As input: offset of the length field to read.
 *                         As output: offset of the first byte after
 *                         the length field.
 * @param[out] tlv_buf Pointer to a @ref nfc_t2t_tlv_block structure where the length
 *                     will be inserted.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int nfc_t2t_length_extract(struct nfc_t2t *t2t,
				  const u8_t *raw_data,
				  u16_t *l_offset,
				  struct nfc_t2t_tlv_block *tlv_buf)
{
	u16_t length;

	if (!nfc_t2t_is_field_within_data_range(t2t, *l_offset,
						NFC_T2T_TLV_L_SHORT_LENGTH)) {
		return -EINVAL;
	}

	length = raw_data[*l_offset];

	if (length == NFC_T2T_TLV_L_FORMAT_FLAG) {
		/* Check another two bytes. */
		if (!nfc_t2t_is_field_within_data_range(t2t,
							*l_offset,
							NFC_T2T_TLV_L_LONG_LENGTH)) {
			return -EINVAL;
		}

		length = sys_get_be16(&raw_data[*l_offset + 1]);

		/* Long length value cannot be lower than 0xFF. */
		if (length < 0xFF) {
			return -EINVAL;
		}

		tlv_buf->length = length;
		*l_offset += NFC_T2T_TLV_L_LONG_LENGTH;

	} else {
		tlv_buf->length = length;
		*l_offset += NFC_T2T_TLV_L_SHORT_LENGTH;
	}

	return 0;
}


/**
 * @brief Function for reading a pointer to the value field of a TLV block
 *        from the raw_data buffer.
 *
 * This function reads a pointer to the value field of a TLV block and
 * inserts it into a structure pointed by the tlv_buf pointer.
 * If there is no value field present in the TLV block, NULL is inserted.
 *
 * @param[in] t2t Pointer to the structure that contains Type 2 Tag
 *                data parsed so far.
 * @param[in] raw_data Pointer to the buffer with a raw data from the tag.
 * @param[in,out] v_offset As input: offset of the value field to read.
 *                         As output: offset of the first byte after the
 *                         value field.
 * @param[in,out] tlv_buf Pointer to a @ref nfc_t2t_tlv_block structure where the
 *                        value field pointer will be inserted.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int nfc_t2t_value_ptr_extract(struct nfc_t2t *t2t,
				     const u8_t *raw_data,
				     u16_t *v_offset,
				     struct nfc_t2t_tlv_block *tlv_buf)
{
	if (tlv_buf->length == 0) {
		/* Clear the value pointer, don't touch the offset. */
		tlv_buf->value = NULL;
	} else {
		if (!nfc_t2t_is_field_within_data_range(t2t,
							*v_offset,
							tlv_buf->length)) {
			return -EINVAL;
		}

		tlv_buf->value = raw_data + *v_offset;
		*v_offset += tlv_buf->length;
	}

	return 0;
}


/**
 * @brief Function for reading a single TLV block from the raw_data buffer.
 *
 * This function reads a single TLV block from the raw_data buffer and
 * stores its contents in a structure pointed by the tlv_buf.
 *
 * @param[in] t2t Pointer to the structure that contains Type 2 Tag
 *                data parsed so far.
 * @param[in] raw_data Pointer to the buffer with a raw data from the tag.
 * @param[in,out] tlv_offset As input: offset of the TLV block to read.
 *                           As output: offset of the next TLV block, 0 if
 *                           it was the last block.
 * @param[out] tlv_buf Pointer to a @ref nfc_t2t_tlv_block structure that will be
 *                     filled with the data read.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int nfc_t2t_tlv_block_extract(struct nfc_t2t *t2t,
				     const u8_t *raw_data,
				     u16_t *offset,
				     struct nfc_t2t_tlv_block  *tlv_buf)
{
	int err;

	memset(tlv_buf, 0, sizeof(struct nfc_t2t_tlv_block));

	/* TLV Tag field. */
	err = nfc_t2t_type_extract(t2t, raw_data, offset, tlv_buf);
	if (err) {
		return err;
	}

	/* Further processing depends on tag field value. */
	switch (tlv_buf->tag) {
	case NFC_T2T_TLV_NULL:
		/* Simply ignore NULL blocks, leave the incremented offset. */
		break;

	case NFC_T2T_TLV_TERMINATOR:
		/* Write 0 to the offset variable, indicating that last
		 * TLV block was found.
		 */
		*offset = 0;

		break;

	case NFC_T2T_TLV_LOCK_CONTROL:
	case NFC_T2T_TLV_MEMORY_CONTROL:
	case NFC_T2T_TLV_NDEF_MESSAGE:
	case NFC_T2T_TLV_PROPRIETARY:
	default:
		/* Unknown blocks should also be extracted. */
		err = nfc_t2t_length_extract(t2t, raw_data, offset,
					     tlv_buf);
		if (err) {
			return err;
		}

		if (tlv_buf->length > 0) {
			err = nfc_t2t_value_ptr_extract(t2t,
							raw_data, offset,
							tlv_buf);
			if (err) {
				return err;
			}
		}

		break;
	}

	return 0;
}


/**
 * @brief Function for checking the checksum bytes of the UID stored
 *        in internal area.
 *
 * This function calculates the block check character (BCC) bytes based on
 * the parsed serial number and compares them with bytes read from
 * the Type 2 Tag.
 *
 * @param[in] sn Pointer to the @ref nfc_t2t_sn structure
 *               to check.
 *
 * @retval  TRUE  If the calculated BCC matched the BCC from the tag.
 * @retval  FALSE Otherwise.
 *
 */
static bool nfc_t2t_is_bcc_correct(struct nfc_t2t_sn *sn)
{
	u8_t bcc1 = (u8_t)NFC_T2T_UID_BCC_CASCADE_BYTE ^
		    (u8_t)sn->manufacturer_id ^
		    (u8_t)((sn->serial_number_part_1 >> 8) & 0xFF) ^
		    (u8_t)(sn->serial_number_part_1 & 0xFF);

	u8_t bcc2 = (u8_t)((sn->serial_number_part_2 >> 24) & 0xFF) ^
		    (u8_t)((sn->serial_number_part_2 >> 16) & 0xFF) ^
		    (u8_t)((sn->serial_number_part_2 >> 8) & 0xFF) ^
		    (u8_t)(sn->serial_number_part_2 & 0xFF);

	return (bcc1 == sn->check_byte_0) && (bcc2 == sn->check_byte_1);
}


/**
 * @brief Function for parsing an internal area of a Type 2 Tag.
 *
 * This function reads data from an internal area in the raw data
 * buffer and fills the @ref nfc_t2t_sn structure
 * within @ref nfc_t2t.
 *
 * @param[in,out] t2t Pointer to the structure that will be filled
 *                    with parsed data.
 * @param[in] raw_data Pointer to the buffer with raw data from the tag.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int nfc_t2t_internal_parse(struct nfc_t2t *t2t,
				  const u8_t *raw_data)
{
	t2t->sn.manufacturer_id      = raw_data[0];
	t2t->sn.serial_number_part_1 = sys_get_be16(&raw_data[1]);
	t2t->sn.check_byte_0         = raw_data[3];
	t2t->sn.serial_number_part_2 = sys_get_be32(&raw_data[4]);
	t2t->sn.check_byte_1         = raw_data[8];
	t2t->sn.internal             = raw_data[9];

	t2t->lock_bytes = sys_get_be16(&raw_data[10]);

	if (!nfc_t2t_is_bcc_correct(&t2t->sn)) {
		LOG_WRN("Warning! BCC of the serial number is not correct!");
	}

	return 0;
}


/**
 * @brief Function for parsing a Capabiliy Container area of a Type 2 Tag.
 *
 * This function reads data from a Capability Container area in the
 * raw data buffer and fills the @ref nfc_t2t_cc structure
 * within @ref nfc_t2t.
 *
 * @param[in,out] t2t Pointer to the structure that will be filled
 *                    with parsed data.
 * @param[in] raw_data Pointer to the buffer with raw data from the tag.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
static int nfc_t2t_cc_parse(struct nfc_t2t *t2t, const u8_t *raw_data)
{
	const u8_t *cc_block = raw_data + NFC_T2T_CC_BLOCK_OFFSET;

	if (cc_block[0] != NFC_T2T_NFC_FORUM_DEFINED_DATA) {
		return -EINVAL;
	}

	t2t->cc.major_version  = MSN_GET(cc_block[1]);
	t2t->cc.minor_version  = LSN_GET(cc_block[1]);
	t2t->cc.data_area_size = cc_block[2] * 8;
	t2t->cc.read_access    = MSN_GET(cc_block[3]);
	t2t->cc.write_access   = LSN_GET(cc_block[3]);

	return 0;
}


/**
 * @brief Function for parsing a single TLV block.
 *
 * This function reads a single TLV block from the raw data buffer,
 * from the position indicated by the tlv_offset, and adds it to
 * the @ref nfc_t2t structure.
 *
 * @param[in,out] t2t Pointer to the structure that will be filled
 *                    with parsed data.
 * @param[in] raw_data Pointer to the buffer with raw data from the tag.
 * @param[in,out] tlv_offset As input: offset of the TLV block to parse.
 *                           As output: offset of the next TLV block,
 *                           0 if it was the last block.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.*
 */
static int nfc_t2t_tlv_parse(struct nfc_t2t *t2t,
			     const u8_t *raw_data,
			     u16_t *tlv_offset)
{
	int err;
	struct nfc_t2t_tlv_block new_block;

	/* Get tag field. */
	err = nfc_t2t_tlv_block_extract(t2t, raw_data, tlv_offset,
					&new_block);
	if (err) {
		return err;
	}

	if (!tlv_block_is_data_length_correct(&new_block)) {
		return -EINVAL;
	}

	/* Further action depends on tag type. */
	switch (new_block.tag) {
	case NFC_T2T_TLV_NULL:
	case NFC_T2T_TLV_TERMINATOR:
		/* Ignore them. */
		break;

	case NFC_T2T_TLV_LOCK_CONTROL:
	case NFC_T2T_TLV_MEMORY_CONTROL:
	case NFC_T2T_TLV_NDEF_MESSAGE:
	case NFC_T2T_TLV_PROPRIETARY:
	default:
		/* Unknown tag types are also added. */
		err = nfc_t2t_tlv_block_insert(t2t, &new_block);
		if (err) {
			LOG_WRN("Warning! Not enough memory  to insert all of the blocks!");
			return err;
		}

		break;
	}

	return 0;
}


void nfc_t2t_clear(struct nfc_t2t *t2t)
{
	t2t->tlv_count = 0;
	memset(&t2t->cc, 0, sizeof(t2t->cc));
	memset(&t2t->sn, 0, sizeof(t2t->sn));
}


int nfc_t2t_parse(struct nfc_t2t *t2t, const u8_t *raw_data)
{
	int err;

	nfc_t2t_clear(t2t);

	err = nfc_t2t_internal_parse(t2t, raw_data);
	if (err) {
		return err;
	}

	err = nfc_t2t_cc_parse(t2t, raw_data);
	if (err) {
		return err;
	}

	if (!nfc_t2t_is_version_supported(t2t)) {
		return -EFAULT;
	}

	u16_t offset = NFC_T2T_FIRST_DATA_BLOCK_OFFSET;

	while (offset > 0) {
		/* Check if end of tag is reached
		 * (no terminator block was present).
		 */
		if (nfc_t2t_is_end_reached(t2t, offset)) {
			LOG_DBG("No terminator block was found in the tag!");
			break;
		}

		err = nfc_t2t_tlv_parse(t2t, raw_data, &offset);
		if (err) {
			return err;
		}
	}

	return 0;
}


void nfc_t2t_printout(const struct nfc_t2t *t2t)
{
	LOG_INF("Type 2 Tag contents:");
	LOG_INF("Number of TLV blocks: %d", t2t->tlv_count);

	LOG_DBG("Internal data:");
	LOG_DBG("    Manufacturer ID:      0x%02x",
		t2t->sn.manufacturer_id);
	LOG_DBG("    Serial number part 1: 0x%04x",
		t2t->sn.serial_number_part_1);
	LOG_DBG("    Check byte 0:         0x%02x",
		t2t->sn.check_byte_0);
	LOG_DBG("    Serial number part 2: 0x%08x",
		t2t->sn.serial_number_part_2);
	LOG_DBG("    Check byte 1:         0x%02x",
		t2t->sn.check_byte_1);
	LOG_DBG("    Internal byte:        0x%02x",
		t2t->sn.internal);
	LOG_DBG("    Lock bytes:           0x%04x",
		t2t->lock_bytes);

	LOG_DBG("Capability Container data:");
	LOG_DBG("    Major version number: %d", t2t->cc.major_version);
	LOG_DBG("    Minor version number: %d", t2t->cc.minor_version);
	LOG_DBG("    Data area size:       %d", t2t->cc.data_area_size);
	LOG_DBG("    Read access:          0x%02X",
		t2t->cc.read_access);
	LOG_DBG("    Write access:         0x%02X",
		t2t->cc.write_access);

	for (size_t i = 0; i < t2t->tlv_count; i++) {
		LOG_INF("TLV block 0x%02X: ", t2t->tlv_block_array[i].tag);

		switch (t2t->tlv_block_array[i].tag) {
		case NFC_T2T_TLV_LOCK_CONTROL:
			LOG_INF("Lock Control");
			break;
		case NFC_T2T_TLV_MEMORY_CONTROL:
			LOG_INF("Memory Control");
			break;
		case NFC_T2T_TLV_NDEF_MESSAGE:
			LOG_INF("NDEF Message");
			break;
		case NFC_T2T_TLV_PROPRIETARY:
			LOG_INF("Proprietary");
			break;
		case NFC_T2T_TLV_NULL:
			LOG_INF("Null");
			break;
		case NFC_T2T_TLV_TERMINATOR:
			LOG_INF("Terminator");
			break;
		default:
			LOG_INF("Unknown");
			break;
		}

		LOG_INF("    Data length: %d", t2t->tlv_block_array[i].length);

		if (t2t->tlv_block_array[i].length > 0) {
			LOG_DBG(" Data:");
			LOG_HEXDUMP_DBG(t2t->tlv_block_array[i].value,
					t2t->tlv_block_array[i].length,
					"Data: ");
		}
	}
}

