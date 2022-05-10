/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "msg_parser_local.h"

LOG_MODULE_REGISTER(nfc_ndef_parser, CONFIG_NFC_NDEF_PARSER_LOG_LEVEL);

int nfc_ndef_msg_parse(uint8_t *result_buf,
		       uint32_t *result_buf_len,
		       const uint8_t *raw_data,
		       uint32_t *raw_data_len)
{
	__ASSERT((POINTER_TO_UINT(result_buf) % 4) == 0,
			"Buffer address for the msg parser must be word-aligned");

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
	uint32_t i;

	LOG_INF("NDEF message contains %d record(s)",
		msg_desc->record_count);

	for (i = 0; i < msg_desc->record_count; i++) {
		nfc_ndef_record_printout(i, msg_desc->record[i]);
	}
}
