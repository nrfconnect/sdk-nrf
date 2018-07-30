/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <nfc/ndef/nfc_ndef_msg.h>
#include <nrf.h>
#include <misc/byteorder.h>
#include <stdint.h>
/**
 * @brief Resolve the value of record location flags of the NFC NDEF record
 * within an NFC NDEF message.
 */
static enum nfc_ndef_record_location record_location_get(u32_t index,
							 u32_t record_count)
{
	enum nfc_ndef_record_location record_location;

	if (!index) {
		if (record_count == 1) {
			record_location = NDEF_LONE_RECORD;
		} else {
			record_location = NDEF_FIRST_RECORD;
		}
	} else if (index == UINT32_MAX || record_count == index + 1) {
		record_location = NDEF_LAST_RECORD;
	} else {
		record_location = NDEF_MIDDLE_RECORD;
	}

	return record_location;
}

int nfc_ndef_msg_encode(struct nfc_ndef_msg_desc const *ndef_msg_desc,
			u8_t *msg_buffer,
			u32_t *msg_len)
{
	u32_t sum_of_len = 0;

	if (!ndef_msg_desc || !msg_len) {
		return -EINVAL;
	}

	struct nfc_ndef_record_desc **pp_record_rec_desc =
						ndef_msg_desc->record;

	if (!ndef_msg_desc->record) {
		return -EINVAL;
	}
	/* Pointer is used only with Type 4 Tag */
	u8_t *root_msg_buffer = msg_buffer;

	if (CONFIG_NFC_NDEF_MSG_TAG_TYPE == TYPE_4_TAG) {
		if (msg_buffer) {
			if (*msg_len < NLEN_FIELD_SIZE) {
				return -ENOSR;
			}

			msg_buffer += NLEN_FIELD_SIZE;
		}
		sum_of_len += NLEN_FIELD_SIZE;
	}

	for (u32_t i = 0; i < ndef_msg_desc->record_count; i++) {
		enum nfc_ndef_record_location record_location;
		u32_t temp_len;
		int err;

		record_location =
			 record_location_get(i, ndef_msg_desc->record_count);

		temp_len = *msg_len - sum_of_len;

		err = nfc_ndef_record_encode(*pp_record_rec_desc,
						  record_location,
						  msg_buffer,
						  &temp_len);
		if (err) {
			return err;
		}

		sum_of_len += temp_len;
		if (msg_buffer) {
			msg_buffer += temp_len;
		}
		/* next record */
		pp_record_rec_desc++;
	}
	if (CONFIG_NFC_NDEF_MSG_TAG_TYPE == TYPE_4_TAG) {
		if (msg_buffer) {
			if (sum_of_len - NLEN_FIELD_SIZE > UINT16_MAX) {
				return -ENOTSUP;
			}
			*(u16_t *)root_msg_buffer =
				sys_cpu_to_be16(sum_of_len - NLEN_FIELD_SIZE);
		}
	}

	*msg_len = sum_of_len;

	return 0;
}

void nfc_ndef_msg_clear(struct nfc_ndef_msg_desc *msg)
{
	msg->record_count = 0;
}

int nfc_ndef_msg_record_add(struct nfc_ndef_msg_desc *msg,
			    struct nfc_ndef_record_desc const *record)
{
	if (msg->record_count >= msg->max_record_count) {
		return -ENOSR;
	}

	msg->record[msg->record_count] = (struct nfc_ndef_record_desc *)record;
	msg->record_count++;

	return 0;
}
