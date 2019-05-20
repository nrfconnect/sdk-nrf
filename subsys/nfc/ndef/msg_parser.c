/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <logging/log.h>
#include "msg_parser_local.h"

LOG_MODULE_REGISTER(nfc_ndef_parser);

int nfc_ndef_msg_parse(const u8_t *result_buf,
		       u32_t *result_buf_len,
		       const u8_t *raw_data,
		       u32_t *raw_data_len)
{
	int err;
	struct nfc_ndef_parser_memo_desc parser_memory_helper;

	err = nfc_ndef_msg_parser_memo_resolve(result_buf,
					       result_buf_len,
					       &parser_memory_helper);

	if (err != 0) {
		return err;
	}

	err = nfc_ndef_msg_parser_internal(&parser_memory_helper,
					   raw_data,
					   raw_data_len);

	return err;
}


void nfc_ndef_msg_printout(const struct nfc_ndef_msg_desc *msg_desc)
{
	u32_t i;

	LOG_INF("NDEF message contains %d record(s)",
		msg_desc->record_count);

	for (i = 0; i < msg_desc->record_count; i++) {
		nfc_ndef_record_printout(i, msg_desc->record[i]);
	}
}

