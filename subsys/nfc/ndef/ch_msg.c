/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/entropy.h>
#include <zephyr/sys/byteorder.h>

#include <nfc/ndef/ch_msg.h>
#include <nfc/ndef/msg.h>

static int alternate_rec_encode(struct nfc_ndef_record_desc *hc,
				const struct nfc_ndef_record_desc *ac,
				size_t cnt)
{
	int err;

	for (size_t i = 0; i < cnt; i++) {
		err = nfc_ndef_ch_rec_local_record_add(hc,
						       &ac[i]);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int ch_rec_create(struct nfc_ndef_msg_desc *msg,
			 struct nfc_ndef_record_desc *ch,
			 const struct nfc_ndef_record_desc *carrier,
			 size_t cnt)
{
	int err;

	err = nfc_ndef_msg_record_add(msg, ch);
	if (err) {
		return err;
	}

	for (size_t i = 0; i < cnt; i++) {
		err = nfc_ndef_msg_record_add(msg, &carrier[i]);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int ch_msg_create(struct nfc_ndef_msg_desc *msg,
			 struct nfc_ndef_record_desc *ch_rec,
			 const struct nfc_ndef_ch_msg_records *records)
{
	int err;

	err = alternate_rec_encode(ch_rec, records->ac, records->cnt);
	if (err) {
		return err;
	}

	return ch_rec_create(msg, ch_rec, records->carrier, records->cnt);
}

int nfc_ndef_ch_msg_le_oob_encode(const struct nfc_ndef_le_oob_rec_payload_desc *oob,
				  uint8_t *buf, size_t *len)
{
	int err;

	NFC_NDEF_MSG_DEF(oob_msg, 1);
	NFC_NDEF_LE_OOB_RECORD_DESC_DEF(oob_rec, 0, oob);

	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(oob_msg),
				      &NFC_NDEF_LE_OOB_RECORD_DESC(oob_rec));
	if (err) {
		return err;
	}

	return nfc_ndef_msg_encode(&NFC_NDEF_MSG(oob_msg), buf, len);
}

int nfc_ndef_ch_msg_hs_create(struct nfc_ndef_msg_desc *msg,
			      struct nfc_ndef_record_desc *hs_rec,
			      const struct nfc_ndef_ch_msg_records *records)
{
	if (!msg || !hs_rec || !records) {
		return -EINVAL;
	}

	if (!records->ac || !records->carrier || records->cnt < 1) {
		return -EINVAL;
	}

	return ch_msg_create(msg, hs_rec, records);
}

int nfc_ndef_ch_msg_hr_create(struct nfc_ndef_msg_desc *msg,
			      struct nfc_ndef_record_desc *hr_rec,
			      const struct nfc_ndef_record_desc *cr_rec,
			      const struct nfc_ndef_ch_msg_records *records)
{
	int err;

	if (!msg || !hr_rec || !records || !cr_rec) {
		return -EINVAL;
	}

	if (!records->ac || !records->carrier || records->cnt < 1) {
		return -EINVAL;
	}

	err = nfc_ndef_ch_rec_local_record_add(hr_rec, cr_rec);
	if (err) {
		return err;
	}

	return ch_msg_create(msg, hr_rec, records);
}

int nfc_ndef_ch_msg_hm_create(struct nfc_ndef_msg_desc *msg,
			      struct nfc_ndef_record_desc *hm_rec,
			      const struct nfc_ndef_ch_msg_records *records)
{
	if (!msg || !hm_rec || !records) {
		return -EINVAL;
	}

	if (!records->ac || !records->carrier || records->cnt < 1) {
		return -EINVAL;
	}

	return ch_msg_create(msg, hm_rec, records);
}

int nfc_ndef_ch_msg_hi_create(struct nfc_ndef_msg_desc *msg,
			      struct nfc_ndef_record_desc *hi_rec,
			      const struct nfc_ndef_ch_msg_records *records)
{
	if (!msg || !hi_rec || !records) {
		return -EINVAL;
	}

	if (!records->ac || !records->carrier || records->cnt < 1) {
		return -EINVAL;
	}

	return ch_msg_create(msg, hi_rec, records);
}
