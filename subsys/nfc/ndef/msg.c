/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nfc/ndef/msg.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <errno.h>

/* Resolve the value of record location flags of the NFC NDEF record
 * within an NFC NDEF message.
 */
static enum nfc_ndef_record_location record_location_get(uint32_t index,
							 uint32_t record_count)
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
			uint8_t *msg_buffer,
			uint32_t *msg_len)
{
	uint32_t sum_of_len = 0;

	if (!ndef_msg_desc || !msg_len) {
		return -EINVAL;
	}

	struct nfc_ndef_record_desc const **pp_record_rec_desc =
						ndef_msg_desc->record;

	if (!ndef_msg_desc->record) {
		return -EINVAL;
	}

	for (uint32_t i = 0; i < ndef_msg_desc->record_count; i++) {
		enum nfc_ndef_record_location record_location;
		uint32_t temp_len;
		int err;

		record_location =
			 record_location_get(i, ndef_msg_desc->record_count);

		temp_len = *msg_len - sum_of_len;

		err = nfc_ndef_record_encode(*pp_record_rec_desc,
					     record_location,
					     msg_buffer ?
						&msg_buffer[sum_of_len] : NULL,
					     &temp_len);
		if (err) {
			return err;
		}

		sum_of_len += temp_len;

		/* next record */
		pp_record_rec_desc++;
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

	msg->record[msg->record_count] = record;
	msg->record_count++;

	return 0;
}
