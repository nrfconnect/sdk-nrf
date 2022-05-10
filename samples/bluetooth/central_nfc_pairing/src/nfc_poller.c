/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <nfc/t2t/parser.h>

#include <nfc/t4t/isodep.h>
#include <nfc/t4t/hl_procedure.h>
#include <nfc/t4t/ndef_file.h>

#include <nfc/ndef/msg_parser.h>
#include <nfc/ndef/text_rec.h>
#include <nfc/ndef/ch_msg.h>
#include <nfc/ndef/le_oob_rec.h>

#include <nfc/tnep/poller.h>
#include <nfc/tnep/ch.h>

#include <st25r3911b_nfca.h>

#include "nfc_poller.h"

#define TNEP_SVC_CNT 2

#define NFCA_BD 128
#define BITS_IN_BYTE 8
#define NFCA_T2T_BUFFER_SIZE 1024
#define NFCA_LAST_BIT_MASK 0x80
#define NFCA_FDT_ALIGN_84 84
#define NFCA_FDT_ALIGN_20 20

#define NFC_T2T_READ_CMD 0x30
#define NFC_T2T_READ_CMD_LEN 0x02

#define T2T_MAX_DATA_EXCHANGE 16
#define TAG_TYPE_2_DATA_AREA_MULTIPLICATOR 8
#define TAG_TYPE_2_DATA_AREA_SIZE_OFFSET (NFC_T2T_CC_BLOCK_OFFSET + 2)
#define TAG_TYPE_2_BLOCKS_PER_EXCHANGE \
	(T2T_MAX_DATA_EXCHANGE / NFC_T2T_BLOCK_SIZE)

#define NFC_TNEP_DATA_SIZE 256
#define NFC_TNEP_SVC_NAME_MAX_LEN 30

#define MAX_TLV_BLOCKS 10
#define MAX_NDEF_RECORDS 10

#define NFC_T4T_ISODEP_FSD 256
#define NFC_T4T_ISODEP_RX_DATA_MAX_SIZE 1024
#define NFC_T4T_APDU_MAX_SIZE 1024

#define NFC_TX_DATA_LEN NFC_T4T_ISODEP_FSD
#define NFC_RX_DATA_LEN NFC_T4T_ISODEP_FSD

#define TRANSMIT_DELAY 3000
#define ALL_REQ_DELAY 2000

static uint8_t tx_data[NFC_TX_DATA_LEN];
static uint8_t rx_data[NFC_RX_DATA_LEN];

static struct k_work_delayable transmit_work;
static ndef_recv_cb_t ndef_cb;

NFC_T4T_CC_DESC_DEF(t4t_cc, MAX_TLV_BLOCKS);

static uint8_t tnep_tx_data[NFC_TNEP_DATA_SIZE];
static uint8_t tnep_rx_data[NFC_TNEP_DATA_SIZE];

static const struct nfc_tnep_buf tnep_tx_buf = {
	.data = tnep_tx_data,
	.size = sizeof(tnep_tx_data)
};

static struct nfc_tnep_buf tnep_rx_buf = {
	.data = tnep_rx_data,
	.size = sizeof(tnep_rx_data)
};

static struct st25r3911b_nfca_buf tx_buf = {
	.data = tx_data,
	.len = sizeof(tx_data)
};

static const struct st25r3911b_nfca_buf rx_buf = {
	.data = rx_data,
	.len = sizeof(rx_data)
};

struct t4t_tag {
	uint8_t data[NFC_T4T_ISODEP_RX_DATA_MAX_SIZE];
	uint8_t ndef[MAX_TLV_BLOCKS][NFC_T4T_APDU_MAX_SIZE];
	uint8_t tlv_index;
};

enum t2t_state {
	T2T_IDLE,
	T2T_HEADER_READ,
	T2T_DATA_READ
};

struct t2t_tag {
	enum t2t_state state;
	uint16_t data_bytes;
	uint8_t data[NFCA_T2T_BUFFER_SIZE];
};

static enum nfc_tnep_tag_type tag_type;
static struct t2t_tag t2t;
static struct t4t_tag t4t;
static struct nfc_ndef_tnep_rec_svc_param services[TNEP_SVC_CNT];
static const struct nfc_ndef_tnep_rec_svc_param *ch_svc;
static uint32_t tnep_msg_max_size;
static bool tnep_mode;
static bool field_on;

static void nfc_tag_detect(bool all_request)
{
	int err;
	enum st25r3911b_nfca_detect_cmd cmd;

	tag_type = NFC_TNEP_TAG_TYPE_UNSUPPORTED;
	tnep_mode = false;

	cmd = all_request ? ST25R3911B_NFCA_DETECT_CMD_ALL_REQ :
		ST25R3911B_NFCA_DETECT_CMD_SENS_REQ;

	err = st25r3911b_nfca_tag_detect(cmd);
	if (err) {
		printk("Tag detect error: %d.\n", err);
	}
}

static int ftd_calculate(uint8_t *data, size_t len)
{
	uint8_t ftd_align;

	ftd_align = (data[len - 1] & NFCA_LAST_BIT_MASK) ?
		NFCA_FDT_ALIGN_84 : NFCA_FDT_ALIGN_20;

	return len * NFCA_BD * BITS_IN_BYTE + ftd_align;
}

static int nfc_t2t_read_block_cmd_make(uint8_t *tx_data,
				       size_t tx_data_size,
				       uint8_t block_num)
{
	if (!tx_data) {
		return -EINVAL;
	}

	if (tx_data_size < NFC_T2T_READ_CMD_LEN) {
		return -ENOMEM;
	}

	tx_data[0] = NFC_T2T_READ_CMD;
	tx_data[1] = block_num;

	return 0;
}

static int t2t_header_read(void)
{
	int err;
	int ftd;

	err = nfc_t2t_read_block_cmd_make(tx_data, sizeof(tx_data), 0);
	if (err) {
		return err;
	}

	tx_buf.data = tx_data;
	tx_buf.len = NFC_T2T_READ_CMD_LEN;

	ftd = ftd_calculate(tx_data, NFC_T2T_READ_CMD_LEN);

	t2t.state = T2T_HEADER_READ;

	err = st25r3911b_nfca_transfer_with_crc(&tx_buf, &rx_buf, ftd);

	return err;
}

static void ndef_data_analyze(const uint8_t *ndef_msg_buff, size_t nfc_data_len)
{
	int  err;
	struct nfc_ndef_msg_desc *ndef_msg_desc;
	uint8_t desc_buf[NFC_NDEF_PARSER_REQUIRED_MEM(MAX_NDEF_RECORDS)];
	size_t desc_buf_len = sizeof(desc_buf);

	err = nfc_ndef_msg_parse(desc_buf,
				 &desc_buf_len,
				 ndef_msg_buff,
				 &nfc_data_len);
	if (err) {
		printk("Error during parsing a NDEF message, err: %d.\n", err);
	}

	ndef_msg_desc = (struct nfc_ndef_msg_desc *) desc_buf;

	if (ndef_cb) {
		ndef_cb(ndef_msg_desc);
	}
}

static void t2t_data_read_complete(uint8_t *data)
{
	int err;

	if (!data) {
		printk("No T2T data read.\n");
		return;
	}

	/* Declaration of Type 2 Tag structure. */
	NFC_T2T_DESC_DEF(tag_data, MAX_TLV_BLOCKS);
	struct nfc_t2t *t2t = &NFC_T2T_DESC(tag_data);

	err = nfc_t2t_parse(t2t, data);
	if (err) {
		printk("Not enough memory to read whole tag. Printing what have been read.\n");
	}

	nfc_t2t_printout(t2t);

	struct nfc_t2t_tlv_block *tlv_block = t2t->tlv_block_array;

	for (size_t i = 0; i < t2t->tlv_count; i++) {
		if (tlv_block->tag == NFC_T2T_TLV_NDEF_MESSAGE) {
			ndef_data_analyze(tlv_block->value, tlv_block->length);
			tlv_block++;
		}
	}

	st25r3911b_nfca_tag_sleep();
}

static int t2t_on_data_read(const uint8_t *data, size_t data_len,
			    void (*t2t_read_complete)(uint8_t *))
{
	int err;
	int ftd;
	uint8_t block_to_read;
	uint16_t offset = 0;
	static uint8_t block_num;

	block_to_read = t2t.data_bytes / NFC_T2T_BLOCK_SIZE;
	offset = block_num * NFC_T2T_BLOCK_SIZE;

	memcpy(t2t.data + offset, data, data_len);

	block_num += TAG_TYPE_2_BLOCKS_PER_EXCHANGE;

	if (block_num > block_to_read) {
		block_num = 0;
		t2t.state = T2T_IDLE;

		if (t2t_read_complete) {
			t2t_read_complete(t2t.data);
		}

		return 0;
	}

	err = nfc_t2t_read_block_cmd_make(tx_data, sizeof(tx_data), block_num);
	if (err) {
		return err;
	}

	tx_buf.data = tx_data;
	tx_buf.len = NFC_T2T_READ_CMD_LEN;

	ftd = ftd_calculate(tx_data, NFC_T2T_READ_CMD_LEN);

	err = st25r3911b_nfca_transfer_with_crc(&tx_buf, &rx_buf, ftd);

	return err;
}

static int on_t2t_transfer_complete(const uint8_t *data, size_t len)
{
	switch (t2t.state) {
	case T2T_HEADER_READ:
		t2t.data_bytes = TAG_TYPE_2_DATA_AREA_MULTIPLICATOR *
				 data[TAG_TYPE_2_DATA_AREA_SIZE_OFFSET];

		if ((t2t.data_bytes + NFC_T2T_FIRST_DATA_BLOCK_OFFSET) >
		    sizeof(t2t.data)) {
			return -ENOMEM;
		}

		t2t.state = T2T_DATA_READ;

		return t2t_on_data_read(data, len, t2t_data_read_complete);

	case T2T_DATA_READ:
		return t2t_on_data_read(data, len, t2t_data_read_complete);

	default:
		return -EFAULT;
	}
}

static bool tnep_data_search(const uint8_t *ndef_msg_buff, size_t nfc_data_len)
{
	int  err;
	uint8_t desc_buf[NFC_NDEF_PARSER_REQUIRED_MEM(MAX_NDEF_RECORDS)];
	size_t desc_buf_len = sizeof(desc_buf);
	uint8_t cnt = ARRAY_SIZE(services);

	err = nfc_ndef_msg_parse(desc_buf,
				 &desc_buf_len,
				 ndef_msg_buff,
				 &nfc_data_len);
	if (err) {
		printk("Error during parsing a NDEF message, err: %d.\n", err);
		return false;
	}

	err = nfc_tnep_poller_svc_search((struct nfc_ndef_msg_desc *) desc_buf,
					 services, &cnt);
	if (err) {
		printk("Service search err: %d\n", err);
		return false;
	}

	printk("TNEP Service count: %u\n", cnt);

	if (cnt > 0) {
		ch_svc = nfc_tnep_ch_svc_search(services, cnt);
		if (ch_svc) {
			printk("TNEP Connection Handover service found.\n");
			tnep_mode = true;

			return true;
		}
	}

	if (ndef_cb) {
		ndef_cb((struct nfc_ndef_msg_desc *) desc_buf);
	}

	return false;
}

static void transfer_handler(struct k_work *work)
{
	nfc_tag_detect(false);
}

static int tnep_ndef_read(uint8_t *ndef_buff, uint16_t ndef_len)
{
	return nfc_t4t_hl_procedure_ndef_read(&NFC_T4T_CC_DESC(t4t_cc),
					      ndef_buff, ndef_len);
}

static int tnep_ndef_update(const uint8_t *ndef_buff, uint16_t ndef_len)
{
	return nfc_t4t_hl_procedure_ndef_update(&NFC_T4T_CC_DESC(t4t_cc),
						(uint8_t *)ndef_buff, ndef_len);
}

static struct nfc_tnep_poller_ndef_api tnep_ndef_api = {
	.ndef_read = tnep_ndef_read,
	.ndef_update = tnep_ndef_update
};

static void nfc_field_on(void)
{
	field_on = true;

	printk("NFC field on.\n");

	nfc_tag_detect(false);
}

static void nfc_timeout(bool tag_sleep)
{
	if (tag_type == NFC_TNEP_TAG_TYPE_T4T) {
		nfc_t4t_isodep_on_timeout();

		return;
	}

	/* Sleep will block processing loop. Accepted as it is short. */
	k_sleep(K_MSEC(ALL_REQ_DELAY));

	nfc_tag_detect(true);
}

static void nfc_field_off(void)
{
	field_on = false;

	printk("NFC field off.\n");
}

static void tag_detected(const struct st25r3911b_nfca_sens_resp *sens_resp)
{
	int err;

	printk("Anticollision: 0x%x Platform: 0x%x.\n",
		sens_resp->anticollison,
		sens_resp->platform_info);

	err = st25r3911b_nfca_anticollision_start();
	if (err) {
		printk("Anticollision error: %d.\n", err);
	}
}

static void anticollision_completed(const struct st25r3911b_nfca_tag_info *tag_info,
				    int err)
{
	if (err) {
		printk("Error during anticollision avoidance.\n");

		nfc_tag_detect(false);

		return;
	}

	printk("Tag info, type: %d.\n", tag_info->type);

	if (tag_info->type == ST25R3911B_NFCA_TAG_TYPE_T2T) {
		printk("Type 2 Tag.\n");

		tag_type = NFC_TNEP_TAG_TYPE_T2T;

		err = t2t_header_read();
		if (err) {
			printk("Type 2 Tag data read error %d.\n", err);
		}
	} else if (tag_info->type == ST25R3911B_NFCA_TAG_TYPE_T4T) {
		printk("Type 4 Tag.\n");

		tag_type = NFC_TNEP_TAG_TYPE_T4T;

		err = nfc_tnep_poller_api_set(&tnep_ndef_api,
					      NFC_TNEP_TAG_TYPE_T4T);
		if (err) {
			printk("TNEP API set error: %d\n", err);
			return;
		}

		/* Send RATS command */
		err = nfc_t4t_isodep_rats_send(NFC_T4T_ISODEP_FSD_256, 0);
		if (err) {
			printk("Type 4 Tag RATS sending error %d.\n", err);
		}
	} else {
		printk("Unsupported tag type.\n");

		tag_type = NFC_TNEP_TAG_TYPE_UNSUPPORTED;

		nfc_tag_detect(false);

		return;
	}
}

static void transfer_completed(const uint8_t *data, size_t len, int err)
{
	if (err) {
		printk("NFC Transfer error: %d.\n", err);
		return;
	}

	switch (tag_type) {
	case NFC_TNEP_TAG_TYPE_T2T:
		err = on_t2t_transfer_complete(data, len);
		if (err) {
			printk("NFC-A T2T read error: %d.\n", err);
		}

		break;

	case NFC_TNEP_TAG_TYPE_T4T:
		err = nfc_t4t_isodep_data_received(data, len, err);
		if (err) {
			printk("NFC-A T4T read error: %d.\n", err);
		}

		break;

	default:
		break;
	}
}

static void tag_sleep(void)
{
	int err;

	printk("Tag entered the Sleep state.\n");

	err = st25r3911b_nfca_field_off();
	if (err) {
		printk("NFC Field off error: %d\n", err);
	}
}

static const struct st25r3911b_nfca_cb cb = {
	.field_on = nfc_field_on,
	.field_off = nfc_field_off,
	.tag_detected = tag_detected,
	.anticollision_completed = anticollision_completed,
	.rx_timeout = nfc_timeout,
	.transfer_completed = transfer_completed,
	.tag_sleep = tag_sleep
};

static void t4t_isodep_selected(const struct nfc_t4t_isodep_tag *t4t_tag)
{
	int err;

	printk("NFC T4T selected.\n");

	if ((t4t_tag->lp_divisor != 0) &&
	    (t4t_tag->lp_divisor != t4t_tag->pl_divisor)) {
		printk("Unsupported bitrate divisor by Reader/Writer.\n");
	}

	err = nfc_t4t_hl_procedure_ndef_tag_app_select();
	if (err) {
		printk("NFC T4T app select err %d.\n", err);
		return;
	}
}

static void t4t_isodep_error(int err)
{
	printk("ISO-DEP Protocol error %d.\n", err);

	nfc_tag_detect(false);
}

static void t4t_isodep_data_send(uint8_t *data, size_t data_len, uint32_t ftd)
{
	int err;

	tx_buf.data = data;
	tx_buf.len = data_len;

	err = st25r3911b_nfca_transfer_with_crc(&tx_buf, &rx_buf, ftd);
	if (err) {
		printk("NFC T4T ISO-DEP transfer error: %d.\n", err);
	}
}

static void t4t_isodep_received(const uint8_t *data, size_t data_len)
{
	int err;

	err = nfc_t4t_hl_procedure_on_data_received(data, data_len);
	if (err) {
		printk("NFC Type 4 Tag HL data received error: %d.\n", err);
	}
}

static void t4t_isodep_deselected(void)
{
	st25r3911b_nfca_tag_sleep();
}

static const struct nfc_t4t_isodep_cb t4t_isodep_cb = {
	.selected = t4t_isodep_selected,
	.deselected = t4t_isodep_deselected,
	.error = t4t_isodep_error,
	.ready_to_send = t4t_isodep_data_send,
	.data_received = t4t_isodep_received
};

static void t4t_hl_selected(enum nfc_t4t_hl_procedure_select type)
{
	int err = 0;

	switch (type) {
	case NFC_T4T_HL_PROCEDURE_NDEF_APP_SELECT:
		printk("NFC T4T NDEF Application selected.\n");

		err = nfc_t4t_hl_procedure_cc_select();
		if (err) {
			printk("NFC T4T Capability Container select error: %d.\n",
				err);
		}

		break;

	case NFC_T4T_HL_PROCEDURE_CC_SELECT:
		printk("NFC T4T Capability Container file selected.\n");

		err = nfc_t4t_hl_procedure_cc_read(&NFC_T4T_CC_DESC(t4t_cc));
		if (err) {
			printk("Capability Container read error: %d.\n", err);
		}

		break;

	case NFC_T4T_HL_PROCEDURE_NDEF_FILE_SELECT:
		if (tnep_mode) {
			printk("NFC T4T NDEF file contains the TNEP Initial Message selected.\n");

			err = nfc_tnep_poller_svc_select(&tnep_rx_buf,
							 ch_svc,
							 tnep_msg_max_size);
			if (err) {
				printk("Service select err: %d\n", err);
			}

			return;
		}

		printk("NFC T4T NDEF file selected.\n");

		err = nfc_t4t_hl_procedure_ndef_read(&NFC_T4T_CC_DESC(t4t_cc),
						     t4t.ndef[t4t.tlv_index],
						     NFC_T4T_APDU_MAX_SIZE);
		if (err) {
			printk("NFC T4T NDEF file read error %d.\n", err);
		}

		break;

	default:
		break;
	}

	if (err) {
		st25r3911b_nfca_tag_sleep();

		k_work_reschedule(&transmit_work, K_MSEC(TRANSMIT_DELAY));
	}
}

static void t4t_hl_cc_read(struct nfc_t4t_cc_file *cc)
{
	int err;
	struct nfc_t4t_tlv_block *tlv_block;

	printk("NFC T4T Capability Container file read.\n");

	for (size_t i = 0; i < cc->tlv_count; i++) {
		tlv_block = &cc->tlv_block_array[i];

		if ((tlv_block->type == NFC_T4T_TLV_BLOCK_TYPE_NDEF_FILE_CONTROL_TLV) &&
		    (tlv_block->value.read_access == NFC_T4T_TLV_BLOCK_CONTROL_FILE_READ_ACCESS_GRANTED)) {
			err = nfc_t4t_hl_procedure_ndef_file_select(tlv_block->value.file_id);
			if (err) {
				printk("NFC T4T NDEF file select error: %d.\n",
				       err);
			}

			return;
		}
	}

	printk("No NDEF File TLV in Capability Container.");
}

static void t4t_hl_ndef_read(uint16_t file_id, const uint8_t *data, size_t len)
{
	int err;
	struct nfc_t4t_cc_file *cc;
	struct nfc_t4t_tlv_block *tlv_block;

	printk("NDEF file read, id: 0x%x.\n", file_id);

	if (tnep_mode) {
		err = nfc_tnep_poller_on_ndef_read(nfc_t4t_ndef_file_msg_get(data),
						   nfc_t4t_ndef_file_msg_size_get(len));
		if (err) {
			printk("TNEP Read data error: %d\n", err);
		}

		return;
	}

	t4t.tlv_index++;
	cc = &NFC_T4T_CC_DESC(t4t_cc);

	for (size_t i = t4t.tlv_index; i < cc->tlv_count; i++) {
		tlv_block = &cc->tlv_block_array[i];

		if ((tlv_block->type == NFC_T4T_TLV_BLOCK_TYPE_NDEF_FILE_CONTROL_TLV) &&
		    (tlv_block->value.read_access == NFC_T4T_TLV_BLOCK_CONTROL_FILE_READ_ACCESS_GRANTED)) {

			err = nfc_t4t_hl_procedure_ndef_file_select(tlv_block->value.file_id);
			if (err) {
				printk("NFC T4T NDEF file select error: %d.\n",
				       err);
			}

			return;
		}

		t4t.tlv_index++;
	}

	t4t.tlv_index = 0;

	nfc_t4t_cc_file_printout(cc);

	tlv_block = cc->tlv_block_array;
	for (size_t i = 0; i < cc->tlv_count; i++) {
		if ((tlv_block[i].type == NFC_T4T_TLV_BLOCK_TYPE_NDEF_FILE_CONTROL_TLV) ||
		    (tlv_block[i].value.file.content != NULL)) {
			/* Look for first message contains TNEP Service
			 * Parameter Records.
			 */
			if (tnep_data_search(nfc_t4t_ndef_file_msg_get(tlv_block[i].value.file.content),
					     nfc_t4t_ndef_file_msg_size_get(tlv_block[i].value.file.len))) {
				tnep_msg_max_size = tlv_block[i].value.max_file_size;

				/* In case when NFC Tag device contains more
				 * than one NDEF Message, select the
				 * NDEF file which contains it.
				 */
				err = nfc_t4t_hl_procedure_ndef_file_select(tlv_block->value.file_id);
				if (err) {
					printk("TNEP Initial NDEF Message select error: %d.\n",
					       err);
				}

				return;
			}
		}
	}

	err = nfc_t4t_isodep_tag_deselect();
	if (err) {
		printk("NFC T4T Deselect error: %d.\n", err);
	}
}

static void t4t_hl_ndef_update(uint16_t file_id)
{
	if (tnep_mode) {
		nfc_tnep_poller_on_ndef_write();
	}
}

static const struct nfc_t4t_hl_procedure_cb t4t_hl_procedure_cb = {
	.selected = t4t_hl_selected,
	.cc_read = t4t_hl_cc_read,
	.ndef_read = t4t_hl_ndef_read,
	.ndef_updated = t4t_hl_ndef_update
};

void tnep_svc_deselected(void)
{
	int err;

	printk("TNEP Service deselected.\n");

	err = nfc_t4t_isodep_tag_deselect();
	if (err) {
		printk("NFC T4T Deselect error: %d.\n", err);
	}
}

void tnep_error(int err)
{
	printk("TNEP error: %d.\n", err);
}

static struct nfc_tnep_poller_cb tnep_cb = {
	.svc_deselected = tnep_svc_deselected,
	.error = tnep_error
};

int nfc_poller_init(struct k_poll_event *events, ndef_recv_cb_t ndef_recv_cb)
{
	int err;

	if (!events || !ndef_recv_cb) {
		return -EINVAL;
	}

	ndef_cb = ndef_recv_cb;

	nfc_t4t_hl_procedure_cb_register(&t4t_hl_procedure_cb);
	k_work_init_delayable(&transmit_work, transfer_handler);
	nfc_tnep_poller_cb_register(&tnep_cb);

	err = nfc_tnep_poller_init(&tnep_tx_buf);
	if (err) {
		printk("NFC TNEP Protocol initialization err: %d\n", err);
		return err;
	}

	err = nfc_t4t_isodep_init(tx_data, sizeof(tx_data),
				  t4t.data, sizeof(t4t.data),
				  &t4t_isodep_cb);
	if (err) {
		printk("NFC T4T ISO-DEP Protocol initialization failed err: %d.\n",
		       err);
		return err;
	}

	err = st25r3911b_nfca_init(events, ST25R3911B_NFCA_EVENT_CNT, &cb);
	if (err) {
		printk("NFCA initialization failed err: %d.\n", err);
		return err;
	}

	err = st25r3911b_nfca_field_on();
	if (err) {
		printk("Field on error %d.", err);
		return err;
	}

	return 0;
}

int nfc_poller_process(void)
{
	return st25r3911b_nfca_process();
}

bool nfc_poller_field_active(void)
{
	return field_on;
}
