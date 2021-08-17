/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <nfc/ndef/uri_msg.h>

int nfc_ndef_uri_msg_encode(enum nfc_ndef_uri_rec_id uri_id_code,
			    uint8_t const *const uri_data,
			    uint16_t uri_data_len,
			    uint8_t *buf,
			    uint32_t *len)
{
	int err;

	/* Create NFC NDEF message description with URI record */
	NFC_NDEF_MSG_DEF(nfc_uri_msg, 1);
	NFC_NDEF_URI_RECORD_DESC_DEF(nfc_uri_rec,
				     uri_id_code,
				     uri_data,
				     uri_data_len);

	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_uri_msg),
				      &NFC_NDEF_URI_RECORD_DESC(nfc_uri_rec));
	if (err < 0) {
		return err;
	}

	if (!uri_data) {
		return -EINVAL;
	}
	/* Encode whole message into buffer */
	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_uri_msg),
				  buf,
				  len);
	return err;
}
