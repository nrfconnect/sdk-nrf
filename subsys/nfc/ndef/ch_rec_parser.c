/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include <nfc/ndef/msg_parser.h>
#include <nfc/ndef/ch_rec_parser.h>
#include <nfc/ndef/payload_type_common.h>

/* Size of the field with CPS data. */
#define AC_REC_CPS_BYTE_SIZE 1

LOG_MODULE_REGISTER(nfc_ndef_ch_rec_parser,
		    CONFIG_NFC_NDEF_CH_REC_PARSER_LOG_LEVEL);

static uint8_t *memory_allocate(struct net_buf_simple *buf,
			     uint32_t alloc_size)
{
	if (net_buf_simple_tailroom(buf) < alloc_size) {
		return NULL;
	}

	return net_buf_simple_add(buf, alloc_size);
}

static bool rec_type_check(const struct nfc_ndef_record_desc *rec_desc,
			   const uint8_t *type, size_t type_len)
{
	if (!rec_desc || !rec_desc->tnf) {
		return false;
	}

	if (rec_desc->tnf != TNF_WELL_KNOWN) {
		return false;
	}

	/* All Connection Handover Record types have the same length. */
	if (rec_desc->type_length != type_len) {
		return false;
	}

	if (memcmp(rec_desc->type, type,
		   type_len) != 0) {
		return false;
	}

	return true;
}

static int hc_rec_payload_parse(struct nfc_ndef_bin_payload_desc *payload_desc,
				uint8_t *result_buf, uint32_t *result_buf_len)
{
	const uint8_t *payload_buf = payload_desc->payload;
	uint32_t payload_len = payload_desc->payload_length;
	const uint32_t buf_size = *result_buf_len;
	struct net_buf_simple buf;
	struct nfc_ndef_ch_hc_rec *hc_rec;

	/* Initialize memory allocator with buffer memory block and its size. */
	net_buf_simple_init_with_data(&buf, result_buf, buf_size);
	net_buf_simple_reset(&buf);

	hc_rec = (struct nfc_ndef_ch_hc_rec *)
			memory_allocate(&buf, sizeof(*hc_rec));
	if (!hc_rec) {
		return -ENOMEM;
	}

	memset(hc_rec, 0, sizeof(*hc_rec));

	if (payload_len < 1) {
		return -EINVAL;
	}

	hc_rec->ctf = (enum nfc_ndef_record_tnf)payload_buf;
	payload_buf++;
	payload_len--;

	if (payload_len < 1) {
		return -EINVAL;
	}

	hc_rec->carrier.type_len = *payload_buf;
	payload_buf++;
	payload_len--;

	if (payload_len < hc_rec->carrier.type_len) {
		return -EINVAL;
	}

	hc_rec->carrier.type = memory_allocate(&buf, hc_rec->carrier.type_len);
	if (!hc_rec->carrier.type) {
		return -ENOMEM;
	}

	memcpy((uint8_t *)hc_rec->carrier.type, payload_buf,
	       hc_rec->carrier.type_len);

	payload_buf += hc_rec->carrier.type_len;
	payload_len -= hc_rec->carrier.type_len;

	if (payload_len < 1) {
		*result_buf_len =
			(buf_size - net_buf_simple_tailroom(&buf));
		return 0;
	}

	hc_rec->carrier.data_len = payload_len;

	hc_rec->carrier.data = memory_allocate(&buf, hc_rec->carrier.data_len);
	if (!hc_rec->carrier.data) {
		return -ENOMEM;
	}

	memcpy(hc_rec->carrier.data, payload_buf,
	       hc_rec->carrier.data_len);

	*result_buf_len =
		(buf_size - net_buf_simple_tailroom(&buf));

	return 0;
}

static int ac_reference_parse(struct nfc_ndef_ch_ac_rec_ref *ac_rec,
			      struct net_buf_simple *buf,
			      const uint8_t **payload_buf,
			      uint32_t *payload_len)
{
	if (*payload_len < 1) {
		return -EINVAL;
	}

	ac_rec->length = **payload_buf;
	(*payload_buf)++;
	(*payload_len)--;

	if (*payload_len < ac_rec->length) {
		return -EINVAL;
	}

	ac_rec->data = memory_allocate(buf, ac_rec->length);
	if (!ac_rec->data) {
		return -ENOMEM;
	}

	memcpy((uint8_t *)ac_rec->data,
	       *payload_buf,
	       ac_rec->length);

	(*payload_buf) += ac_rec->length;
	(*payload_len) -= ac_rec->length;

	return 0;
}

static int ac_rec_payload_parse(struct nfc_ndef_bin_payload_desc *payload_desc,
				uint8_t *result_buf, uint32_t *result_buf_len)
{
	int err;
	const uint8_t *payload_buf = payload_desc->payload;
	uint32_t payload_len = payload_desc->payload_length;
	const uint32_t buf_size = *result_buf_len;
	struct net_buf_simple buf;
	struct nfc_ndef_ch_ac_rec *ac_rec;

	/* Initialize memory allocator with buffer memory block and
	 * its size.
	 */
	net_buf_simple_init_with_data(&buf, result_buf, buf_size);
	net_buf_simple_reset(&buf);

	ac_rec = (struct nfc_ndef_ch_ac_rec *)
			memory_allocate(&buf, sizeof(*ac_rec));
	if (!ac_rec) {
		return -ENOMEM;
	}

	memset(ac_rec, 0, sizeof(*ac_rec));

	if (payload_len < AC_REC_CPS_BYTE_SIZE) {
		return -EINVAL;
	}

	ac_rec->cps = (enum nfc_ndef_ch_ac_rec_cps)(*payload_buf);
	payload_buf += AC_REC_CPS_BYTE_SIZE;
	payload_len -= AC_REC_CPS_BYTE_SIZE;

	err = ac_reference_parse(&ac_rec->carrier_data_ref, &buf,
				 &payload_buf, &payload_len);
	if (err) {
		return err;
	}

	if (payload_len < 1) {
		return -EINVAL;
	}

	ac_rec->aux_data_ref_cnt = *payload_buf;
	payload_buf++;
	payload_len--;

	if (!ac_rec->aux_data_ref_cnt) {
		*result_buf_len =
			(buf_size - net_buf_simple_tailroom(&buf));
		return 0;
	}

	ac_rec->aux_data_ref = (struct nfc_ndef_ch_ac_rec_ref *)
		memory_allocate(&buf, ac_rec->aux_data_ref_cnt * sizeof(*ac_rec->aux_data_ref));
	if (!ac_rec->aux_data_ref) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < ac_rec->aux_data_ref_cnt; i++) {
		err = ac_reference_parse(&ac_rec->aux_data_ref[i], &buf,
					 &payload_buf, &payload_len);
		if (err) {
			return err;
		}
	}

	*result_buf_len =
		(buf_size - net_buf_simple_tailroom(&buf));

	return 0;
}

static int ch_rec_payload_parse(struct nfc_ndef_bin_payload_desc *payload_desc,
				uint8_t *result_buf, uint32_t *result_buf_len)
{
	int err;
	const uint32_t buf_size = *result_buf_len;
	const uint8_t *payload_buf = payload_desc->payload;
	uint32_t payload_len = payload_desc->payload_length;
	uint32_t local_msg_size;
	uint8_t *local_msg;
	struct nfc_ndef_ch_rec *ch_rec;
	struct net_buf_simple buf;

	/* Initialize memory allocator with buffer memory block and its size. */
	net_buf_simple_init_with_data(&buf, result_buf, buf_size);
	net_buf_simple_reset(&buf);

	ch_rec = (struct nfc_ndef_ch_rec *)
				memory_allocate(&buf, sizeof(*ch_rec));
	if (!ch_rec) {
		return -ENOMEM;
	}

	memset(ch_rec, 0, sizeof(*ch_rec));

	if (payload_len < 1) {
		return -EINVAL;
	}

	ch_rec->major_version = ((*payload_buf) >> 4) & 0x0F;
	ch_rec->minor_version = (*payload_buf) & 0x0F;
	payload_buf++;
	payload_len--;

	local_msg_size = net_buf_simple_tailroom(&buf);

	local_msg = memory_allocate(&buf, local_msg_size);
	if (!local_msg) {
		return -ENOMEM;
	}

	err = nfc_ndef_msg_parse(local_msg, &local_msg_size,
				 payload_buf, &payload_len);
	if (err) {
		return err;
	}

	ch_rec->local_records = (struct nfc_ndef_msg_desc *)local_msg;

	*result_buf_len =
		(buf_size - net_buf_simple_tailroom(&buf));

	return 0;
}

static bool ch_rec_check(const struct nfc_ndef_record_desc *rec_desc)
{
	if (!rec_desc || !rec_desc->tnf) {
		return false;
	}

	if (rec_desc->tnf != TNF_WELL_KNOWN) {
		return false;
	}

	/* All Connection Handover Record types have the same length. */
	if (rec_desc->type_length != sizeof(nfc_ndef_ch_hs_rec_type_field)) {
		return false;
	}

	if (memcmp(rec_desc->type, nfc_ndef_ch_hs_rec_type_field,
		   sizeof(nfc_ndef_ch_hs_rec_type_field)) == 0) {
		return true;
	} else if (memcmp(rec_desc->type, nfc_ndef_ch_hr_rec_type_field,
		   sizeof(nfc_ndef_ch_hr_rec_type_field)) == 0) {
		return true;
	} else if (memcmp(rec_desc->type, nfc_ndef_ch_hm_rec_type_field,
		   sizeof(nfc_ndef_ch_hm_rec_type_field)) == 0) {
		return true;
	} else if (memcmp(rec_desc->type, nfc_ndef_ch_hi_rec_type_field,
		   sizeof(nfc_ndef_ch_hi_rec_type_field)) == 0) {
		return true;
	}

	return false;
}

bool nfc_ndef_ch_hc_rec_check(const struct nfc_ndef_record_desc *rec_desc)
{
	return rec_type_check(rec_desc, nfc_ndef_ch_hc_rec_type_field,
			      sizeof(nfc_ndef_ch_hc_rec_type_field));
}

bool nfc_ndef_ch_cr_rec_check(const struct nfc_ndef_record_desc *rec_desc)
{
	return rec_type_check(rec_desc, nfc_ndef_ch_cr_rec_type_field,
			      sizeof(nfc_ndef_ch_cr_rec_type_field));
}

bool nfc_ndef_ch_ac_rec_check(const struct nfc_ndef_record_desc *rec_desc)
{
	return rec_type_check(rec_desc, nfc_ndef_ch_ac_rec_type_field,
			      sizeof(nfc_ndef_ch_ac_rec_type_field));
}

bool nfc_ndef_ch_rec_check(const struct nfc_ndef_record_desc *rec_desc,
			   enum nfc_ndef_ch_rec_type rec_type)
{
	switch (rec_type) {
	case NFC_NDEF_CH_REC_TYPE_HANDOVER_SELECT:
		return rec_type_check(rec_desc, nfc_ndef_ch_hs_rec_type_field,
				sizeof(nfc_ndef_ch_hs_rec_type_field));

	case NFC_NDEF_CH_REC_TYPE_HANDOVER_REQUEST:
		return rec_type_check(rec_desc, nfc_ndef_ch_hr_rec_type_field,
				sizeof(nfc_ndef_ch_hr_rec_type_field));

	case NFC_NDEF_CH_REC_TYPE_HANDOVER_INITIATE:
		return rec_type_check(rec_desc, nfc_ndef_ch_hi_rec_type_field,
				sizeof(nfc_ndef_ch_hi_rec_type_field));

	case NFC_NDEF_CH_REC_TYPE_HANDOVER_MEDIATION:
		return rec_type_check(rec_desc, nfc_ndef_ch_hm_rec_type_field,
				sizeof(nfc_ndef_ch_hm_rec_type_field));
	default:
		return false;
	}
}

int nfc_ndef_ch_hc_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			     uint8_t *result_buf, uint32_t *result_buf_len)
{
	struct nfc_ndef_bin_payload_desc *payload_desc;

	if (!rec_desc || !result_buf || !result_buf_len) {
		return -EINVAL;
	}

	if (!nfc_ndef_ch_hc_rec_check(rec_desc)) {
		return -EINVAL;
	}

	if (rec_desc->payload_constructor !=
		(payload_constructor_t) nfc_ndef_bin_payload_memcopy) {
		return -ENOTSUP;
	}

	payload_desc = (struct nfc_ndef_bin_payload_desc *)
		rec_desc->payload_descriptor;

	return hc_rec_payload_parse(payload_desc, result_buf, result_buf_len);
}

int nfc_ndef_ch_cr_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			     struct nfc_ndef_ch_cr_rec *cr_rec)
{
	struct nfc_ndef_bin_payload_desc *payload_desc;
	const uint8_t *payload_buf;
	uint32_t payload_len;

	if (!rec_desc || !cr_rec) {
		return -EINVAL;
	}

	if (!nfc_ndef_ch_cr_rec_check(rec_desc)) {
		return -EINVAL;
	}

	if (rec_desc->payload_constructor !=
		(payload_constructor_t) nfc_ndef_bin_payload_memcopy) {
		return -ENOTSUP;
	}

	payload_desc = (struct nfc_ndef_bin_payload_desc *)
		rec_desc->payload_descriptor;

	payload_buf = payload_desc->payload;
	payload_len = payload_desc->payload_length;

	if (payload_len < sizeof(cr_rec->random)) {
		return -EINVAL;
	}

	cr_rec->random = sys_get_be16(payload_buf);

	return 0;
}

int nfc_ndef_ch_ac_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			     uint8_t *result_buf, uint32_t *result_buf_len)
{
	struct nfc_ndef_bin_payload_desc *payload_desc;

	if (!rec_desc || !result_buf || !result_buf_len) {
		return -EINVAL;
	}

	if (!nfc_ndef_ch_ac_rec_check(rec_desc)) {
		return -EINVAL;
	}

	if (rec_desc->payload_constructor !=
		(payload_constructor_t) nfc_ndef_bin_payload_memcopy) {
		return -ENOTSUP;
	}

	payload_desc = (struct nfc_ndef_bin_payload_desc *)
		rec_desc->payload_descriptor;

	return ac_rec_payload_parse(payload_desc,
				    result_buf, result_buf_len);
}

int nfc_ndef_ch_rec_parse(const struct nfc_ndef_record_desc *rec_desc,
			  uint8_t *result_buf, uint32_t *result_buf_len)
{
	struct nfc_ndef_bin_payload_desc *payload_desc;

	if (!rec_desc || !result_buf || !result_buf_len) {
		return -EINVAL;
	}

	if (!ch_rec_check(rec_desc)) {
		return -EINVAL;
	}

	if (rec_desc->payload_constructor !=
		(payload_constructor_t) nfc_ndef_bin_payload_memcopy) {
		return -ENOTSUP;
	}

	payload_desc = (struct nfc_ndef_bin_payload_desc *)
		rec_desc->payload_descriptor;

	return ch_rec_payload_parse(payload_desc,
				    result_buf, result_buf_len);
}

static const char * const cps_strings[] = {
	"Carrier is currently off",
	"Carrier is currently on",
	"Carrier is in the process of activating",
	"Carrier unknown state"
};

static const char * const tnf_strings[] = {
	"Empty",
	"NFC Forum well-known type",
	"Media-type (RFC 2046)",
	"Absolute URI (RFC 3986)",
	"NFC Forum external type (NFC RTD)"
};

void nfc_ndef_ch_rec_printout(const struct nfc_ndef_ch_rec *ch_rec)
{
	if (!ch_rec) {
		return;
	}

	LOG_INF("NDEF Connection Handover record payload");

	LOG_INF("\tConnection Handover Major version:\t%d",
		ch_rec->major_version);
	LOG_INF("\tConnection Handover Minor version:\t%d",
		ch_rec->minor_version);
	LOG_INF("\tLocal records count:\t%d",
		ch_rec->local_records->record_count);
}

void nfc_ndef_ac_rec_printout(const struct nfc_ndef_ch_ac_rec *ac_rec)
{
	if (!ac_rec) {
		return;
	}

	LOG_INF("Alternative Carrier Record payload");

	LOG_INF("\tCarrier Power State:\t%s", cps_strings[ac_rec->cps]);
	LOG_HEXDUMP_INF(ac_rec->carrier_data_ref.data,
			ac_rec->carrier_data_ref.length,
			"\tCarrier data reference:");
	LOG_INF("\tAuxiliary data reference count:\t%d",
		ac_rec->aux_data_ref_cnt);

	for (size_t i = 0; i < ac_rec->aux_data_ref_cnt; i++) {
		LOG_HEXDUMP_INF(ac_rec->aux_data_ref[i].data,
				ac_rec->aux_data_ref[i].length,
				"\tAuxiliary carrier data reference:");
	}
}

void nfc_ndef_hc_rec_printout(const struct nfc_ndef_ch_hc_rec *hc_rec)
{
	if (!hc_rec) {
		return;
	}

	LOG_INF("Handover Carrier record");
	LOG_INF("\tCarrier type format:\t%s", tnf_strings[hc_rec->ctf]);
	LOG_HEXDUMP_INF(hc_rec->carrier.type, hc_rec->carrier.type_len,
			"\tCarrier type:");

	if (hc_rec->carrier.data_len > 0) {
		LOG_HEXDUMP_INF(hc_rec->carrier.data, hc_rec->carrier.data_len,
				"\n Carrier data:");
	}
}

void nfc_ndef_cr_rec_printout(const struct nfc_ndef_ch_cr_rec *cr_rec)
{
	if (!cr_rec) {
		return;
	}

	LOG_INF("Collision Resolution Record payload");
	LOG_INF("\tRandom number:%d\t", cr_rec->random);
}
