/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <nfc/ndef/text_rec.h>

/** Size of the status. */
#define TEXT_REC_STATUS_SIZE          1
/** Position of a character encoding type. */
#define TEXT_REC_STATUS_UTF_POS       7
/** Reserved position. */
#define TEXT_REC_RESERVED_POS         6

const uint8_t nfc_ndef_text_rec_type_field[] = {'T'};

/* Function for calculating payload size. */
static uint32_t nfc_text_rec_payload_size_get(
	    struct nfc_ndef_text_rec_payload const *nfc_rec_text_payload_desc)
{
	return (TEXT_REC_STATUS_SIZE +
		nfc_rec_text_payload_desc->lang_code_len +
		nfc_rec_text_payload_desc->data_len);
}

int nfc_ndef_text_rec_payload_encode(
		struct nfc_ndef_text_rec_payload *nfc_rec_text_payload_desc,
		uint8_t *buff,
		uint32_t *len)
{
	if ((!nfc_rec_text_payload_desc->lang_code_len)			||
	    (nfc_rec_text_payload_desc->lang_code_len &
					(1 << TEXT_REC_RESERVED_POS))	||
	    (nfc_rec_text_payload_desc->lang_code_len &
					(1 << TEXT_REC_STATUS_UTF_POS))	||
	    (!nfc_rec_text_payload_desc->lang_code)			||
	    (!nfc_rec_text_payload_desc->data_len)			||
	    (!nfc_rec_text_payload_desc->data)				||
	    (!len)) {

		return -EINVAL;
	}

	uint32_t payload_size =
		    nfc_text_rec_payload_size_get(nfc_rec_text_payload_desc);

	if (buff) {
		if (payload_size > *len) {
			return -ENOSR;
		}
		*buff = (nfc_rec_text_payload_desc->lang_code_len +
			   (nfc_rec_text_payload_desc->utf <<
					   TEXT_REC_STATUS_UTF_POS));
		buff += TEXT_REC_STATUS_SIZE;

		memcpy(buff,
		       nfc_rec_text_payload_desc->lang_code,
		       nfc_rec_text_payload_desc->lang_code_len);
		buff += nfc_rec_text_payload_desc->lang_code_len;

		memcpy(buff,
		       nfc_rec_text_payload_desc->data,
		       nfc_rec_text_payload_desc->data_len);
	}
	*len = payload_size;

	return 0;
}
