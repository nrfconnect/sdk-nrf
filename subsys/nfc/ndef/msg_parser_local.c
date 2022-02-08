/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <errno.h>
#include <nfc/ndef/msg_parser.h>
#include "msg_parser_local.h"

int nfc_ndef_msg_parser_internal(struct nfc_ndef_parser_memo_desc *parser_memo_desc,
				 const uint8_t *nfc_data,
				 uint32_t *nfc_data_len)
{
	enum nfc_ndef_record_location record_location;

	int err;

	uint32_t nfc_data_left = *nfc_data_len;
	uint32_t temp_nfc_data_len = 0;

	/* Want to modify -> use local copy. */
	struct nfc_ndef_bin_payload_desc *bin_pay_desc =
		parser_memo_desc->bin_pay_desc;
	struct nfc_ndef_record_desc *rec_desc = parser_memo_desc->rec_desc;


	while (nfc_data_left > 0) {
		temp_nfc_data_len = nfc_data_left;

		err = nfc_ndef_record_parse(bin_pay_desc,
					    rec_desc,
					    &record_location,
					    nfc_data,
					    &temp_nfc_data_len);
		if (err != 0) {
			return err;
		}

		/* Verify the records location flags. */
		if (parser_memo_desc->msg_desc->record_count == 0) {
			if ((record_location != NDEF_FIRST_RECORD) &&
			    (record_location != NDEF_LONE_RECORD)) {
				return -EFAULT;
			}
		} else {
			if ((record_location != NDEF_MIDDLE_RECORD) &&
			    (record_location != NDEF_LAST_RECORD)) {
				return -EFAULT;
			}
		}

		err = nfc_ndef_msg_record_add(parser_memo_desc->msg_desc,
					      rec_desc);
		if (err != 0) {
			return err;
		}

		nfc_data_left -= temp_nfc_data_len;

		if ((record_location == NDEF_LAST_RECORD) ||
		    (record_location == NDEF_LONE_RECORD)) {
			*nfc_data_len = *nfc_data_len - nfc_data_left;
			return 0;
		} else {
			if (parser_memo_desc->msg_desc->record_count ==
			    parser_memo_desc->msg_desc->max_record_count) {
				return -ENOMEM;
			}

			nfc_data += temp_nfc_data_len;
			bin_pay_desc++;
			rec_desc++;
		}
	}

	return -EFAULT;
}


int nfc_ndef_msg_parser_memo_resolve(uint8_t *result_buf,
				     uint32_t *result_buf_len,
				     struct nfc_ndef_parser_memo_desc *parser_memo_desc)
{
	uint32_t max_rec_num;
	uint32_t memory_last;
	uint8_t *end;
	const struct nfc_ndef_record_desc **record_desc_array;

	if (*result_buf_len < sizeof(struct nfc_ndef_msg_parser_msg_1)) {
		return -ENOMEM;
	}

	memory_last = (*result_buf_len) - sizeof(struct nfc_ndef_msg_parser_msg_1);
	max_rec_num = (memory_last / (NFC_NDEF_MSG_PARSER_DELTA)) + 1;

	parser_memo_desc->msg_desc = (struct nfc_ndef_msg_desc *) result_buf;
	record_desc_array =
		(const struct nfc_ndef_record_desc **) &parser_memo_desc->msg_desc[1];
	parser_memo_desc->bin_pay_desc =
		(struct nfc_ndef_bin_payload_desc *) &record_desc_array[max_rec_num];
	parser_memo_desc->rec_desc =
		(struct nfc_ndef_record_desc *)&parser_memo_desc->bin_pay_desc[max_rec_num];

	/* Initialize message description. */
	parser_memo_desc->msg_desc->record = record_desc_array;
	parser_memo_desc->msg_desc->max_record_count = max_rec_num;
	parser_memo_desc->msg_desc->record_count = 0;

	end = (uint8_t *) &parser_memo_desc->rec_desc[max_rec_num];

	*result_buf_len = end - result_buf;

	return 0;
}
