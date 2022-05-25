/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stddef.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <nfc/ndef/ch.h>

#define HS_REC_VERSION_SIZE 1

/* Size of the field with CPS data. */
#define AC_REC_CPS_BYTE_SIZE 1

/* Size of the Data Reference Length field. */
#define AC_REC_DATA_REF_LEN_SIZE 1

/* Size of the Data Reference Length field. */
#define AC_REC_AUX_DATA_REF_COUNT_SIZE 1

/* Size of the field with CTF data.*/
#define HC_REC_CTF_BYTE_SIZE 1

/* Size of the field with Carrier Type Length. */
#define HC_REC_CARRIER_TYPE_LEN_SIZE 1

static uint32_t nfc_ac_rec_payload_size_get(const struct nfc_ndef_ch_ac_rec *ac_rec)
{
	/* Initialize with size of byte with CPS. */
	uint32_t payload_size = AC_REC_CPS_BYTE_SIZE;

	/* Add Carrier Data Reference size. */
	payload_size +=  ac_rec->carrier_data_ref.length +
			 AC_REC_DATA_REF_LEN_SIZE;

	/* Add Auxiliary Data Reference Count size. */
	payload_size += AC_REC_AUX_DATA_REF_COUNT_SIZE;

	for (size_t i = 0; i < ac_rec->aux_data_ref_cnt; i++) {
		/* Add Auxiliary Data Reference size. */
		payload_size += ac_rec->aux_data_ref[i].length +
				AC_REC_DATA_REF_LEN_SIZE;
	}

	return payload_size;
}

static uint32_t nfc_hc_payload_size_get(const struct nfc_ndef_ch_hc_rec *hc_rec)
{
	uint32_t payload_size = HC_REC_CTF_BYTE_SIZE;

	payload_size += hc_rec->carrier.type_len +
			HC_REC_CARRIER_TYPE_LEN_SIZE;

	payload_size += hc_rec->carrier.data_len;

	return payload_size;
}

int nfc_ndef_ch_rec_payload_encode(const struct nfc_ndef_ch_rec *ch_rec,
				   uint8_t *buf, uint32_t *len)
{
	int err;

	if (buf) {
		/* There must be at least 1 free byte in buffer for
		 *version byte.
		 */
		if (*len < HS_REC_VERSION_SIZE) {
			return -ENOMEM;
		}

		/* Major/minor version byte. */
		*buf = ((ch_rec->major_version << 4) & 0xF0) |
			(ch_rec->minor_version & 0x0F);

		buf += HS_REC_VERSION_SIZE;

		/* Decrement remaining buffer size. */
		*len -= HS_REC_VERSION_SIZE;
	}

	/* Encode local records encapsulated in a message. */
	err = nfc_ndef_msg_encode(ch_rec->local_records,
				  buf, len);
	if (err) {
		return err;
	}

	/* Add version byte to the total record size. */
	*len += HS_REC_VERSION_SIZE;

	return 0;
}

void nfc_ndef_ch_rec_local_record_clear(struct nfc_ndef_record_desc *ch_rec)
{
	struct nfc_ndef_ch_rec *ch_payload =
		(struct nfc_ndef_ch_rec *)ch_rec->payload_descriptor;

	nfc_ndef_msg_clear(ch_payload->local_records);
}

int nfc_ndef_ch_rec_local_record_add(struct nfc_ndef_record_desc *ch_rec,
				     const struct nfc_ndef_record_desc *local_rec)
{
	struct nfc_ndef_ch_rec *ch_payload =
		(struct nfc_ndef_ch_rec *)ch_rec->payload_descriptor;

	return nfc_ndef_msg_record_add(ch_payload->local_records, local_rec);
}

int nfc_ndef_ch_hc_rec_payload_encode(const struct nfc_ndef_ch_hc_rec *hc_rec,
				      uint8_t *buf, uint32_t *len)
{
	uint32_t payload_size = nfc_hc_payload_size_get(hc_rec);

	if (!buf) {
		*len = payload_size;
		return 0;
	}

	/* Not enough space in the buffer, return an error. */
	if (payload_size > *len) {
		return -ENOMEM;
	}

	/* Invalid CTF value. */
	if ((hc_rec->ctf < TNF_WELL_KNOWN) ||
	    (hc_rec->ctf > TNF_EXTERNAL_TYPE)) {
		return -EFAULT;
	}

	*buf = hc_rec->ctf;
	buf += HC_REC_CTF_BYTE_SIZE;

	/* Copy Carrier data */
	*buf = hc_rec->carrier.type_len;
	buf += HC_REC_CARRIER_TYPE_LEN_SIZE;

	memcpy(buf, hc_rec->carrier.type, hc_rec->carrier.type_len);
	buf += hc_rec->carrier.type_len;

	/* Copy Optionall Carrier data. */
	memcpy(buf, hc_rec->carrier.data, hc_rec->carrier.data_len);
	buf += hc_rec->carrier.data_len;

	*len = payload_size;

	return 0;
}

int nfc_ndef_ch_ac_rec_payload_encode(const struct nfc_ndef_ch_ac_rec *nfc_rec_ac,
				      uint8_t *buf, uint32_t *len)
{
	uint32_t payload_size = nfc_ac_rec_payload_size_get(nfc_rec_ac);

	if (!buf) {
		*len = payload_size;
		return 0;
	}

	/* Not enough space in the buffer, return an error. */
	if (payload_size > *len) {
		return -ENOMEM;
	}

	/* Invalid CPS value. */
	if (nfc_rec_ac->cps & ~NFC_NDEF_CH_AC_CPS_MASK) {
		return -EFAULT;
	}

	/* Copy CPS. */
	*buf = nfc_rec_ac->cps;
	buf += AC_REC_CPS_BYTE_SIZE;

	/* Copy Carrier Data Reference. */
	*buf = nfc_rec_ac->carrier_data_ref.length;
	buf += AC_REC_DATA_REF_LEN_SIZE;

	memcpy(buf,
	       nfc_rec_ac->carrier_data_ref.data,
	       nfc_rec_ac->carrier_data_ref.length);
	buf += nfc_rec_ac->carrier_data_ref.length;

	/* Copy Auxiliary Data Reference. */
	*buf = nfc_rec_ac->aux_data_ref_cnt;
	buf += AC_REC_AUX_DATA_REF_COUNT_SIZE;

	for (size_t i = 0; i < nfc_rec_ac->aux_data_ref_cnt; i++) {
		*buf = nfc_rec_ac->aux_data_ref[i].length;
		buf += AC_REC_DATA_REF_LEN_SIZE;

		memcpy(buf,
		       nfc_rec_ac->aux_data_ref[i].data,
		       nfc_rec_ac->aux_data_ref[i].length);
		buf += nfc_rec_ac->aux_data_ref[i].length;
	}

	/* Assign payload size to the return buffer. */
	*len = payload_size;

	return 0;
}

void nfc_ndef_ch_ac_rec_auxiliary_data_ref_clear(struct nfc_ndef_record_desc *ac_rec)
{
	struct nfc_ndef_ch_ac_rec *ac_rec_payload =
		(struct nfc_ndef_ch_ac_rec *)ac_rec->payload_descriptor;

	ac_rec_payload->aux_data_ref_cnt = 0;
}

int nfc_ndef_ch_ac_rec_auxiliary_data_ref_add(struct nfc_ndef_record_desc *ac_rec,
					      const uint8_t *aux_data,
					      uint8_t aux_length)
{
	struct nfc_ndef_ch_ac_rec *ac_rec_payload =
		(struct nfc_ndef_ch_ac_rec *)ac_rec->payload_descriptor;

	if (ac_rec_payload->aux_data_ref_cnt >= ac_rec_payload->max_aux_data_ref_cnt) {
		return -ENOMEM;
	}

	ac_rec_payload->aux_data_ref[ac_rec_payload->aux_data_ref_cnt].data = aux_data;
	ac_rec_payload->aux_data_ref[ac_rec_payload->aux_data_ref_cnt].length = aux_length;
	ac_rec_payload->aux_data_ref_cnt++;

	return 0;
}

int nfc_ndef_ch_cr_rec_payload_encode(const struct nfc_ndef_ch_cr_rec *nfc_rec_cr,
				      uint8_t *buf, uint32_t *len)
{
	uint32_t payload_size = 0;

	if (buf) {
		if (sizeof(nfc_rec_cr->random) > *len) {
			return -ENOMEM;
		}

		sys_put_be16(nfc_rec_cr->random, buf);
		payload_size += sizeof(nfc_rec_cr->random);
	}

	*len = payload_size;

	return 0;
}
