/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <nfc/t4t/apdu.h>
#include <nfc/t4t/hl_procedure.h>
#include <nfc/t4t/isodep.h>

LOG_MODULE_REGISTER(nfc_t4t_hl_procedure,
		    CONFIG_NFC_T4T_HL_PROCEDURE_LOG_LEVEL);

#define FILE_ID_SIZE 2
#define RAPDU_MIN_LEN 2
#define NDEF_FILE_NLEN_SIZE 2
#define CC_FILE_ID 0xE103
#define CC_MIN_RAPDU_SIZE 0x0F
#define CC_RAPDU_MAX_SIZE_OFFSET 0x03
#define NFC_T4T_APDU_SELECT_DATA {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01}
#define APDU_LE_MAP_2_MAX_VALUE 0xFF
#define NFC_T4T_APDU_RSP_ALL 256

enum nfc_t4t_hl_transaction_type {
	NFC_T4T_HL_SELECT,
	NFC_T4T_HL_CC_READ,
	NFC_T4T_HL_NDEF_NLEN_READ,
	NFC_T4T_HL_NDEF_READ,
	NFC_T4T_HL_NDEF_NLEN_CLEAR,
	NFC_T4T_HL_NDEF_UPDATE,
	NFC_T4T_HL_NDEF_NLEN_UPDATE
};

struct t4t_hl_ndef {
	struct nfc_t4t_cc_file *cc;
	uint8_t *buff;
	uint16_t buff_size;
	uint16_t nlen;
	uint8_t file_id[FILE_ID_SIZE];
};

struct t4t_hl_cc {
	struct nfc_t4t_cc_file *cc;
	uint16_t len;
	uint8_t data[CONFIG_NFC_T4T_HL_PROCEDURE_CC_BUFFER_SIZE];
};

struct t4t_hl_procedure {
	struct t4t_hl_cc cc_file;
	struct t4t_hl_ndef ndef;
	enum nfc_t4t_hl_transaction_type transaction_type;
	enum nfc_t4t_hl_procedure_select select_type;
	uint16_t file_offset;
	uint8_t apdu_buff[CONFIG_NFC_T4T_HL_PROCEDURE_APDU_BUF_SIZE];
};

static struct t4t_hl_procedure t4t_hl;
static const struct nfc_t4t_hl_procedure_cb *hl_cb;

static int t4t_hl_data_exchange(struct nfc_t4t_apdu_comm *comm)
{
	int err;
	uint16_t apdu_len = sizeof(t4t_hl.apdu_buff);

	err = nfc_t4t_apdu_comm_encode(comm,
				       t4t_hl.apdu_buff,
				       &apdu_len);
	if (err) {
		LOG_ERR("NFC T4T C-APDU encode error: %d", err);
		return err;
	}

	return nfc_t4t_isodep_transmit(t4t_hl.apdu_buff, apdu_len);
}

static int on_cc_read(const struct nfc_t4t_apdu_resp *resp)
{
	__ASSERT_NO_MSG(resp);

	int err;
	struct nfc_t4t_apdu_comm apdu_comm;
	const uint8_t *data = resp->data.buff;
	uint16_t len = resp->data.len;

	if ((resp->data.len) && (t4t_hl.cc_file.len == 0)) {
		t4t_hl.cc_file.len = sys_get_be16(data);
	}

	if ((t4t_hl.file_offset + len) > sizeof(t4t_hl.cc_file.data)) {
		return -ENOMEM;
	}

	memcpy(t4t_hl.cc_file.data + t4t_hl.file_offset, data, len);

	t4t_hl.file_offset += len;

	if (t4t_hl.file_offset < t4t_hl.cc_file.len) {
		nfc_t4t_apdu_comm_clear(&apdu_comm);

		apdu_comm.instruction = NFC_T4T_APDU_COMM_INS_READ;
		apdu_comm.parameter = t4t_hl.file_offset;
		apdu_comm.resp_len = MIN(CC_MIN_RAPDU_SIZE,
				t4t_hl.cc_file.len - t4t_hl.file_offset);

		return t4t_hl_data_exchange(&apdu_comm);
	}

	if (hl_cb->cc_read) {
		__ASSERT_NO_MSG(t4t_hl.cc_file.cc);

		err = nfc_t4t_cc_file_parse(t4t_hl.cc_file.cc,
					    t4t_hl.cc_file.data,
					    t4t_hl.cc_file.len);
		if (err) {
			return err;
		}

		hl_cb->cc_read(t4t_hl.cc_file.cc);
	}

	return 0;
}

static int on_ndef_nlen_read(const struct nfc_t4t_apdu_resp *resp)
{
	__ASSERT_NO_MSG(resp);

	const uint8_t *data = resp->data.buff;
	uint16_t len = resp->data.len;

	if (len != NDEF_FILE_NLEN_SIZE) {
		LOG_ERR("NDEF NLEN response is to long");
		return -EINVAL;
	}

	t4t_hl.ndef.nlen = sys_get_be16(data);

	return 0;
}

static int t4t_file_assign(uint16_t id)
{
	struct nfc_t4t_tlv_block_file file;

	__ASSERT_NO_MSG(t4t_hl.ndef.cc);

	file.content = t4t_hl.ndef.buff;
	file.len = t4t_hl.file_offset;

	return nfc_t4t_cc_file_content_set(t4t_hl.ndef.cc, &file, id);
}

static int ndef_file_chunk_read(const struct nfc_t4t_apdu_resp *resp)
{
	__ASSERT_NO_MSG(resp);

	int err;
	uint16_t file_id;
	struct nfc_t4t_apdu_comm apdu_comm;
	const uint8_t *data = resp->data.buff;
	uint16_t len = resp->data.len;

	if (t4t_hl.ndef.buff_size < t4t_hl.file_offset + len) {
		return -ENOMEM;
	}

	memcpy(t4t_hl.ndef.buff + t4t_hl.file_offset, data, len);

	t4t_hl.file_offset += len;

	if (t4t_hl.file_offset < (t4t_hl.ndef.nlen + NDEF_FILE_NLEN_SIZE)) {
		nfc_t4t_apdu_comm_clear(&apdu_comm);

		apdu_comm.instruction = NFC_T4T_APDU_COMM_INS_READ;
		apdu_comm.parameter = t4t_hl.file_offset;
		apdu_comm.resp_len = MIN(t4t_hl.ndef.nlen - (t4t_hl.file_offset - NDEF_FILE_NLEN_SIZE),
				MIN(APDU_LE_MAP_2_MAX_VALUE, t4t_hl.ndef.cc->max_rapdu_size));

		t4t_hl.transaction_type = NFC_T4T_HL_NDEF_READ;

		return t4t_hl_data_exchange(&apdu_comm);
	}

	file_id = sys_get_be16(t4t_hl.ndef.file_id);

	err = t4t_file_assign(file_id);
	if (err) {
		return err;
	}

	if (hl_cb->ndef_read) {
		hl_cb->ndef_read(file_id, t4t_hl.ndef.buff,
				 t4t_hl.ndef.nlen + NDEF_FILE_NLEN_SIZE);
	}

	return 0;
}

static int ndef_file_chunk_update(void)
{
	struct nfc_t4t_apdu_comm apdu_comm;
	uint8_t nlen_data[NDEF_FILE_NLEN_SIZE];

	nfc_t4t_apdu_comm_clear(&apdu_comm);

	apdu_comm.instruction = NFC_T4T_APDU_COMM_INS_UPDATE;

	if (t4t_hl.file_offset < t4t_hl.ndef.buff_size) {
		apdu_comm.parameter = t4t_hl.file_offset;
		apdu_comm.data.buff = t4t_hl.ndef.buff + t4t_hl.file_offset;
		apdu_comm.data.len = MIN(t4t_hl.ndef.buff_size - t4t_hl.file_offset,
				MIN(APDU_LE_MAP_2_MAX_VALUE, t4t_hl.ndef.cc->max_capdu_size));

		t4t_hl.file_offset += apdu_comm.data.len;
		t4t_hl.transaction_type = NFC_T4T_HL_NDEF_UPDATE;

		return t4t_hl_data_exchange(&apdu_comm);
	}

	sys_put_be16(t4t_hl.ndef.nlen, nlen_data);

	apdu_comm.parameter = 0;
	apdu_comm.data.buff = nlen_data;
	apdu_comm.data.len = sizeof(nlen_data);

	t4t_hl.transaction_type = NFC_T4T_HL_NDEF_NLEN_UPDATE;

	return t4t_hl_data_exchange(&apdu_comm);
}

static void on_ndef_nlen_update(void)
{
	uint16_t file_id = sys_get_be16(t4t_hl.ndef.file_id);

	if (hl_cb->ndef_updated) {
		hl_cb->ndef_updated(file_id);
	}
}

static int on_rapdu_succes(const struct nfc_t4t_apdu_resp *resp)
{
	int err = 0;

	switch (t4t_hl.transaction_type) {
	case NFC_T4T_HL_SELECT:
		if (hl_cb->selected) {
			hl_cb->selected(t4t_hl.select_type);
		}
		break;

	case NFC_T4T_HL_CC_READ:
		err = on_cc_read(resp);
		break;

	case NFC_T4T_HL_NDEF_NLEN_READ:
		err = on_ndef_nlen_read(resp);
		if (err) {
			LOG_ERR("NDEF NLEN field read failed.");
			break;
		}

		err = ndef_file_chunk_read(resp);
		break;

	case NFC_T4T_HL_NDEF_READ:
		err = ndef_file_chunk_read(resp);
		break;

	case NFC_T4T_HL_NDEF_NLEN_CLEAR:
	case NFC_T4T_HL_NDEF_UPDATE:
		err = ndef_file_chunk_update();
		break;

	case NFC_T4T_HL_NDEF_NLEN_UPDATE:
		on_ndef_nlen_update();
		break;

	default:
		err = -EAGAIN;

	}

	return err;
}

int nfc_t4t_hl_procedure_cb_register(const struct nfc_t4t_hl_procedure_cb *cb)
{
	if (!cb) {
		return -EINVAL;
	}

	hl_cb = cb;

	return 0;
}

int nfc_t4t_hl_procedure_on_data_received(const uint8_t *data, size_t len)
{
	int err = 0;
	struct nfc_t4t_apdu_resp apdu_resp;

	if ((!data) || (len < RAPDU_MIN_LEN)) {
		return -EINVAL;
	}

	nfc_t4t_apdu_resp_clear(&apdu_resp);

	err = nfc_t4t_apdu_resp_decode(&apdu_resp, data, len);
	if (err) {
		LOG_ERR("NFC T4T R-APDU decode error: %d", err);
		return err;
	}

	nfc_t4t_apdu_resp_printout(&apdu_resp);

	if (apdu_resp.status == NFC_T4T_APDU_RAPDU_STATUS_CMD_COMPLETED) {
		err = on_rapdu_succes(&apdu_resp);
	} else {
		LOG_ERR("NFC T4T R-APDU received status: %d different than command completed.",
			apdu_resp.status);
		return -EPERM;
	}

	return err;
}

int nfc_t4t_hl_procedure_ndef_tag_app_select(void)
{
	struct nfc_t4t_apdu_comm apdu_comm;
	uint8_t t4t_app_name[] = NFC_T4T_APDU_SELECT_DATA;

	nfc_t4t_apdu_comm_clear(&apdu_comm);

	apdu_comm.instruction = NFC_T4T_APDU_COMM_INS_SELECT;
	apdu_comm.parameter = NFC_T4T_APDU_SELECT_BY_NAME;
	apdu_comm.data.buff = t4t_app_name;
	apdu_comm.data.len = sizeof(t4t_app_name);
	apdu_comm.resp_len = NFC_T4T_APDU_RSP_ALL;

	t4t_hl.transaction_type = NFC_T4T_HL_SELECT;
	t4t_hl.select_type = NFC_T4T_HL_PROCEDURE_NDEF_APP_SELECT;

	return t4t_hl_data_exchange(&apdu_comm);
}

int nfc_t4t_hl_procedure_ndef_file_select(uint16_t id)
{
	struct nfc_t4t_apdu_comm apdu_comm;

	nfc_t4t_apdu_comm_clear(&apdu_comm);

	sys_put_be16(id, t4t_hl.ndef.file_id);

	apdu_comm.instruction = NFC_T4T_APDU_COMM_INS_SELECT;
	apdu_comm.parameter = NFC_T4T_APDU_SELECT_BY_FILE_ID;
	apdu_comm.data.buff = t4t_hl.ndef.file_id;
	apdu_comm.data.len = sizeof(t4t_hl.ndef.file_id);

	t4t_hl.transaction_type = NFC_T4T_HL_SELECT;
	t4t_hl.select_type = (id == CC_FILE_ID) ?
		NFC_T4T_HL_PROCEDURE_CC_SELECT : NFC_T4T_HL_PROCEDURE_NDEF_FILE_SELECT;

	return t4t_hl_data_exchange(&apdu_comm);
}

int nfc_t4t_hl_procedure_cc_select(void)
{
	LOG_INF("Capability Container Select Procedure");

	return nfc_t4t_hl_procedure_ndef_file_select(CC_FILE_ID);
}

int nfc_t4t_hl_procedure_cc_read(struct nfc_t4t_cc_file *cc)
{
	struct nfc_t4t_apdu_comm apdu_comm;

	if (!cc) {
		return -EINVAL;
	}

	nfc_t4t_apdu_comm_clear(&apdu_comm);
	memset(&t4t_hl.cc_file, 0, sizeof(t4t_hl.cc_file));
	t4t_hl.file_offset = 0;

	t4t_hl.cc_file.cc = cc;

	apdu_comm.instruction = NFC_T4T_APDU_COMM_INS_READ;
	apdu_comm.parameter = 0;
	apdu_comm.resp_len = CC_MIN_RAPDU_SIZE;

	t4t_hl.transaction_type = NFC_T4T_HL_CC_READ;

	return t4t_hl_data_exchange(&apdu_comm);
}

int nfc_t4t_hl_procedure_ndef_read(struct nfc_t4t_cc_file *cc,
				   uint8_t *ndef_buff,
				   uint16_t ndef_len)
{
	struct nfc_t4t_apdu_comm apdu_comm;

	t4t_hl.file_offset = 0;

	if (!cc || !ndef_buff || !ndef_len) {
		return -EINVAL;
	}

	nfc_t4t_apdu_comm_clear(&apdu_comm);

	apdu_comm.instruction = NFC_T4T_APDU_COMM_INS_READ;
	apdu_comm.parameter = 0;
	apdu_comm.resp_len = NDEF_FILE_NLEN_SIZE;

	t4t_hl.ndef.buff = ndef_buff;
	t4t_hl.ndef.buff_size = ndef_len;
	t4t_hl.ndef.cc = cc;
	t4t_hl.transaction_type = NFC_T4T_HL_NDEF_NLEN_READ;

	return t4t_hl_data_exchange(&apdu_comm);
}

int nfc_t4t_hl_procedure_ndef_update(struct nfc_t4t_cc_file *cc,
				     uint8_t *ndef_data, uint16_t ndef_len)
{
	struct nfc_t4t_apdu_comm apdu_comm;
	uint16_t file_id;
	uint16_t nlen;
	struct nfc_t4t_tlv_block *tlv_block;
	uint8_t nlen_val[] = {0x00, 0x00};

	if (!cc || !ndef_data || (ndef_len < NDEF_FILE_NLEN_SIZE)) {
		return -EINVAL;
	}

	file_id = sys_get_be16(t4t_hl.ndef.file_id);

	tlv_block = nfc_t4t_cc_file_content_get(cc, file_id);
	if (!tlv_block) {
		return -EACCES;
	}

	nlen = sys_get_be16(ndef_data);

	if (((nlen + NDEF_FILE_NLEN_SIZE) != ndef_len) ||
	    (ndef_len > tlv_block->value.max_file_size)) {
		return -ENOMEM;
	}

	t4t_hl.ndef.buff = ndef_data;
	t4t_hl.ndef.buff_size = ndef_len;
	t4t_hl.ndef.nlen = nlen;
	t4t_hl.ndef.cc = cc;

	nfc_t4t_apdu_comm_clear(&apdu_comm);

	/* Set NDEF NLEN to 0. */
	apdu_comm.instruction = NFC_T4T_APDU_COMM_INS_UPDATE;
	apdu_comm.parameter = 0;
	apdu_comm.data.buff = nlen_val;
	apdu_comm.data.len = sizeof(nlen_val);

	t4t_hl.file_offset = NDEF_FILE_NLEN_SIZE;
	t4t_hl.transaction_type = NFC_T4T_HL_NDEF_NLEN_CLEAR;

	return t4t_hl_data_exchange(&apdu_comm);
}
