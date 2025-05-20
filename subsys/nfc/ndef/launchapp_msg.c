/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <nfc/ndef/launchapp_rec.h>
#include <nfc/ndef/launchapp_msg.h>
#include <nfc/ndef/uri_msg.h>


int nfc_launchapp_msg_encode(uint8_t  const *android_package_name,
			     uint32_t android_package_name_len,
			     uint8_t  const *universal_link,
			     uint32_t universal_link_len,
			     uint8_t  *buf,
			     size_t   *len)
{
	int err;

	/* Create NFC NDEF message description, capacity - 2 records */
	NFC_NDEF_MSG_DEF(nfc_launchapp_msg, 2);

	/* Create NFC NDEF Android Application Record description */
	NFC_NDEF_ANDROID_LAUNCHAPP_RECORD_DESC_DEF(nfc_and_launchapp_rec,
						   android_package_name,
						   android_package_name_len);

	/* Create NFC NDEF URI Record description */
	NFC_NDEF_URI_RECORD_DESC_DEF(nrf_universal_link_rec,
				     NFC_URI_NONE,
				     universal_link,
				     universal_link_len);

	if (universal_link) {
		/* Add URI Record as first record to message */
		err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_launchapp_msg),
						&NFC_NDEF_URI_RECORD_DESC(nrf_universal_link_rec));

		if (err) {
			return err;
		}
	}

	if (android_package_name) {
		/* Add Android Application Record as second record to message */
		err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_launchapp_msg),
				&NFC_NDEF_ANDROID_LAUNCHAPP_RECORD_DESC(nfc_and_launchapp_rec));

		if (err) {
			return err;
		}
	}

	if (NFC_NDEF_MSG(nfc_launchapp_msg).record_count == 0) {
		return -EINVAL;
	}

	/* Encode whole message into buffer */
	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_launchapp_msg),
				       buf,
				       len);

	return err;
}
