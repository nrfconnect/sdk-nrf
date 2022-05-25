/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nfc/ndef/ch_msg.h>
#include <nfc/ndef/ch_rec_parser.h>
#include <nfc/ndef/msg_parser.h>

#include <nfc/tnep/tag.h>

#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_REGISTER(nfc_tnep_ch, CONFIG_NFC_TNEP_CH);

#define HR_RANDOM_NUMBER 0x2684

#if defined(CONFIG_NFC_TNEP_TAG)
#define NFC_CH_MSG_DEF(_name, _max_record_cnt) \
	NFC_TNEP_TAG_APP_MSG_DEF(_name, _max_record_cnt)
#else
#define NFC_CH_MSG_DEF(_name, _max_record_cnt) \
	NFC_NDEF_MSG_DEF(_name, _max_record_cnt)
#endif /* defined(CONFIG_NFC_TNEP_TAG) */

#if defined(CONFIG_NFC_TNEP_POLLER)
#define NFC_TNEP_CH_NON_CARRIER_REC_CNT 2
#else
#define NFC_TNEP_CH_NON_CARRIER_REC_CNT 1
#endif /* defined(CONFIG_NFC_TNEP_POLLER) */

const uint8_t nfc_tnep_ch_svc_uri[] = {
	'u', 'r', 'n', ':', 'n', 'f', 'c', ':', 's', 'n', ':',
	'h', 'a', 'n', 'd', 'o', 'v', 'e', 'r'
};

static uint8_t *memory_allocate(struct net_buf_simple *buf,
			     uint32_t alloc_size)
{
	if (net_buf_simple_tailroom(buf) < alloc_size) {
		return NULL;
	}

	return net_buf_simple_add(buf, alloc_size);
}

static int handover_rec_parse(const struct nfc_ndef_ch_rec **ch_rec,
			      const struct nfc_ndef_record_desc *rec,
			      struct net_buf_simple *buf)
{
	int err;
	uint8_t *rec_data;
	size_t rec_data_size;

	rec_data_size = sizeof(struct nfc_ndef_ch_rec) +
		NFC_NDEF_PARSER_REQUIRED_MEM(CONFIG_NFC_TNEP_CH_MAX_LOCAL_RECORD_COUNT);

	rec_data = memory_allocate(buf, rec_data_size);
	if (!rec_data) {
		return -ENOMEM;
	}

	err = nfc_ndef_ch_rec_parse(rec, rec_data,
				    &rec_data_size);
	if (err) {
		return err;
	}

	*ch_rec = (struct nfc_ndef_ch_rec *)rec_data;

	return 0;
}

static int cr_rec_parse(const struct nfc_ndef_ch_cr_rec **cr_rec,
			const struct nfc_ndef_record_desc *rec,
			struct net_buf_simple *buf)
{
	int err;
	uint8_t *rec_data;
	size_t rec_data_size;

	rec_data_size = sizeof(struct nfc_ndef_ch_cr_rec);

	rec_data = memory_allocate(buf, rec_data_size);
	if (!rec_data) {
		return -ENOMEM;
	}

	err = nfc_ndef_ch_cr_rec_parse(rec,
				       (struct nfc_ndef_ch_cr_rec *)rec_data);
	if (err) {
		return err;
	}

	*cr_rec = (struct nfc_ndef_ch_cr_rec *)rec_data;

	return 0;
}

static int ac_rec_parse(const struct nfc_ndef_ch_ac_rec **ac_rec,
			const struct nfc_ndef_record_desc *rec,
			struct net_buf_simple *buf)
{
	int err;
	uint8_t *rec_data;
	size_t rec_data_size;
	struct net_buf_simple_state state;

	rec_data_size = net_buf_simple_tailroom(buf);

	net_buf_simple_save(buf, &state);
	rec_data = memory_allocate(buf, rec_data_size);
	if (!rec_data) {
		return -ENOMEM;
	}

	err = nfc_ndef_ch_ac_rec_parse(rec, rec_data,
				       &rec_data_size);
	if (err) {
		return err;
	}

	*ac_rec = (struct nfc_ndef_ch_ac_rec *)rec_data;

	net_buf_simple_restore(buf, &state);
	(void *)net_buf_simple_add(buf, rec_data_size);

	return 0;
}

static int ac_rec_get(const struct nfc_ndef_record_desc **local_rec,
		      struct nfc_tnep_ch_record *ch_data,
		      struct net_buf_simple *buf,
		      uint32_t rec_count)
{
	int err;

	for (size_t i = 0; i < rec_count; i++) {
		const struct nfc_ndef_record_desc *rec =
					local_rec[i];
		if (nfc_ndef_ch_ac_rec_check(rec)) {
			const struct nfc_ndef_ch_ac_rec *ac_rec;

			err = ac_rec_parse(&ac_rec, rec, buf);
			if (err) {
				return err;
			}

			/* Set pointer to the first AC records */
			if (!ch_data->ac) {
				ch_data->ac = ac_rec;
			}

			ch_data->count++;

		} else {
			return -ENOTSUP;
		}
	}

	return 0;
}

int nfc_tnep_ch_hc_rec_parse(const struct nfc_ndef_ch_hc_rec **hc_rec,
			     const struct nfc_ndef_record_desc *rec,
			     struct net_buf_simple *buf)
{
	int err;
	uint8_t *rec_data;
	size_t rec_data_size;
	struct net_buf_simple_state state;

	rec_data_size = net_buf_simple_tailroom(buf);

	net_buf_simple_save(buf, &state);
	rec_data = memory_allocate(buf, rec_data_size);
	if (!rec_data) {
		return -ENOMEM;
	}

	err = nfc_ndef_ch_hc_rec_parse(rec, rec_data,
				       &rec_data_size);
	if (err) {
		return err;
	}

	*hc_rec = (struct nfc_ndef_ch_hc_rec *)rec_data;

	net_buf_simple_restore(buf, &state);
	(void *)net_buf_simple_add(buf, rec_data_size);

	return 0;
}

int nfc_tnep_ch_request_msg_parse(const struct nfc_ndef_msg_desc *msg,
				  struct nfc_tnep_ch_request *ch_req,
				  struct net_buf_simple *buf)
{
	int err;
	const struct nfc_ndef_ch_rec *ch_rec;
	const struct nfc_ndef_record_desc **rec;
	const struct nfc_ndef_record_desc **local_rec;

	memset(ch_req, 0, sizeof(*ch_req));

	rec = msg->record;

	err = handover_rec_parse(&ch_rec, *rec, buf);
	if (err) {
		return err;
	}

	ch_req->ch_record.major_ver = ch_rec->major_version;
	ch_req->ch_record.minor_ver = ch_rec->minor_version;

	local_rec = ch_rec->local_records->record;
	rec++;

	/* Get collision resolution value */
	if (nfc_ndef_ch_cr_rec_check(*local_rec)) {
		err = cr_rec_parse(&ch_req->cr, *local_rec, buf);
		if (err) {
			return err;
		}
	} else {
		return -EFAULT;
	}

	local_rec++;

	err = ac_rec_get(local_rec, &ch_req->ch_record,
			 buf, (ch_rec->local_records->record_count - 1));
	if (err) {
		return err;
	}

	if (ch_req->ch_record.count !=
	    (msg->record_count - NFC_TNEP_CH_NON_CARRIER_REC_CNT)) {
		return -EFAULT;
	}

	ch_req->ch_record.carrier = rec;

	return 0;
}

int nfc_tnep_ch_select_msg_parse(const struct nfc_ndef_msg_desc *msg,
				 struct nfc_tnep_ch_record *ch_select,
				 struct net_buf_simple *buf)
{
	int err;
	const struct nfc_ndef_ch_rec *ch_rec;
	const struct nfc_ndef_record_desc **rec;

	memset(ch_select, 0, sizeof(*ch_select));

	rec = msg->record;

	err = handover_rec_parse(&ch_rec, *rec, buf);
	if (err) {
		return err;
	}

	ch_select->major_ver = ch_rec->major_version;
	ch_select->minor_ver = ch_rec->minor_version;

	rec++;

	err = ac_rec_get(ch_rec->local_records->record, ch_select,
			 buf, ch_rec->local_records->record_count);
	if (err) {
		return err;
	}

	if (ch_select->count !=
	    (msg->record_count - NFC_TNEP_CH_NON_CARRIER_REC_CNT)) {
		return -EFAULT;
	}

	ch_select->carrier = rec;

	return 0;
}

int nfc_tnep_ch_initiate_msg_parse(const struct nfc_ndef_msg_desc *msg,
				   struct nfc_tnep_ch_record *ch_init,
				   struct net_buf_simple *buf)
{
	int err;
	const struct nfc_ndef_ch_rec *ch_rec;
	const struct nfc_ndef_record_desc **rec;

	memset(ch_init, 0, sizeof(*ch_init));

	rec = msg->record;

	err = handover_rec_parse(&ch_rec, *rec, buf);
	if (err) {
		return err;
	}

	ch_init->major_ver = ch_rec->major_version;
	ch_init->minor_ver = ch_rec->minor_version;

	rec++;

	err = ac_rec_get(ch_rec->local_records->record, ch_init,
			 buf, ch_rec->local_records->record_count);
	if (err) {
		return err;
	}

	if (ch_init->count !=
	    (msg->record_count - NFC_TNEP_CH_NON_CARRIER_REC_CNT)) {
		return -EFAULT;
	}

	ch_init->carrier = rec;

	return 0;
}

int nfc_tnep_ch_request_msg_prepare(const struct nfc_ndef_ch_msg_records *records)
{
	int err;
	/* It is ok to use constant number here since the TNEP does not use it
	 * to resolve collision.
	 */
	uint16_t random = HR_RANDOM_NUMBER;

	NFC_CH_MSG_DEF(hr_msg, records->cnt + 1);
	NFC_NDEF_CH_CR_RECORD_DESC_DEF(cr_rec, random);
	NFC_NDEF_CH_HR_RECORD_DESC_DEF(hr_rec, NFC_NDEF_CH_MSG_MAJOR_VER,
				       NFC_NDEF_CH_MSG_MINOR_VER,
				       records->cnt + 1);

	err = nfc_ndef_ch_msg_hr_create(&NFC_NDEF_MSG(hr_msg),
					&NFC_NDEF_CH_RECORD_DESC(hr_rec),
					&NFC_NDEF_CR_RECORD_DESC(cr_rec),
					records);
	if (err) {
		return err;
	}

	return nfc_tnep_ch_msg_write(&NFC_NDEF_MSG(hr_msg));
}

int nfc_tnep_ch_select_msg_prepare(const struct nfc_ndef_ch_msg_records *records)
{
	int err;

	NFC_CH_MSG_DEF(hs_msg, records->cnt + 1);
	NFC_NDEF_CH_HS_RECORD_DESC_DEF(hs_rec, NFC_NDEF_CH_MSG_MAJOR_VER,
				       NFC_NDEF_CH_MSG_MINOR_VER,
				       records->cnt);

	err = nfc_ndef_ch_msg_hs_create(&NFC_NDEF_MSG(hs_msg),
					&NFC_NDEF_CH_RECORD_DESC(hs_rec),
					records);
	if (err) {
		return err;
	}

	return nfc_tnep_ch_msg_write(&NFC_NDEF_MSG(hs_msg));
}

int nfc_tnep_ch_tnep_mediation_msg_prepare(const struct nfc_ndef_ch_msg_records *records)
{
	int err;

	NFC_CH_MSG_DEF(hm_msg, records->cnt + 1);
	NFC_NDEF_CH_HM_RECORD_DESC_DEF(hm_rec, NFC_NDEF_CH_MSG_MAJOR_VER,
				       NFC_NDEF_CH_MSG_MINOR_VER,
				       records->cnt);

	err = nfc_ndef_ch_msg_hs_create(&NFC_NDEF_MSG(hm_msg),
					&NFC_NDEF_CH_RECORD_DESC(hm_rec),
					records);
	if (err) {
		return err;
	}

	return nfc_tnep_ch_msg_write(&NFC_NDEF_MSG(hm_msg));
}

int nfc_tnep_ch_initiate_msg_prepare(const struct nfc_ndef_ch_msg_records *records)
{
	int err;

	NFC_CH_MSG_DEF(hi_msg, records->cnt + 1);
	NFC_NDEF_CH_HI_RECORD_DESC_DEF(hi_rec, NFC_NDEF_CH_MSG_MAJOR_VER,
				       NFC_NDEF_CH_MSG_MINOR_VER,
				       records->cnt);

	err = nfc_ndef_ch_msg_hs_create(&NFC_NDEF_MSG(hi_msg),
					&NFC_NDEF_CH_RECORD_DESC(hi_rec),
					records);
	if (err) {
		return err;
	}

	return nfc_tnep_ch_msg_write(&NFC_NDEF_MSG(hi_msg));
}

bool nfc_tnep_ch_active_carrier_check(struct nfc_tnep_ch_record *ch_rec)
{
	for (size_t i = 0; i < ch_rec->count; i++) {
		if ((ch_rec->ac[i].cps == NFC_AC_CPS_ACTIVE) ||
		    (ch_rec->ac[i].cps == NFC_AC_CPS_ACTIVATING)) {
			return true;
		}
	}

	return false;
}
