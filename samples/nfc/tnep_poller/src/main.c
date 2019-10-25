/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <st25r3911b_nfca.h>
#include <nfc/ndef/msg_parser.h>
#include <nfc/ndef/nfc_text_rec.h>
#include <nfc/t4t/isodep.h>
#include <nfc/t4t/hl_procedure.h>
#include <nfc/tnep/poller.h>

#define NFC_TNEP_MAX_RECORD 2
#define NFC_TNEP_DATA_SIZE 256
#define NFC_TNEP_SVC_NAME_MAX_LEN 30

#define MAX_TLV_BLOCKS 10
#define MAX_NDEF_RECORDS 10

#define NFC_T4T_ISODEP_FSD 256
#define NFC_T4T_ISODEP_RX_DATA_MAX_SIZE 1024
#define NFC_T4T_APDU_MAX_SIZE 1024
#define TYPE_4_TAG_NLEN_FIELD_SIZE 2

#define NFC_TX_DATA_LEN NFC_T4T_ISODEP_FSD
#define NFC_RX_DATA_LEN NFC_T4T_ISODEP_FSD

#define TRANSMIT_DELAY 3000
#define ALL_REQ_DELAY 2000

static u8_t tx_data[NFC_TX_DATA_LEN];
static u8_t rx_data[NFC_RX_DATA_LEN];

static struct k_poll_event events[ST25R3911B_NFCA_EVENT_CNT];
static struct k_delayed_work transmit_work;

NFC_NDEF_MSG_DEF(poller_msg, NFC_TNEP_MAX_RECORD);
NFC_T4T_CC_DESC_DEF(t4t_cc, MAX_TLV_BLOCKS);

static u8_t tnep_tx_data[NFC_TNEP_DATA_SIZE];
static u8_t tnep_rx_data[NFC_TNEP_DATA_SIZE];

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
	u8_t data[NFC_T4T_ISODEP_RX_DATA_MAX_SIZE];
	u8_t ndef[MAX_TLV_BLOCKS][NFC_T4T_APDU_MAX_SIZE];
	u8_t tlv_index;
};

static enum nfc_tnep_tag_type tag_type;
static struct t4t_tag t4t;
static struct nfc_ndef_tnep_rec_svc_param services[2];
static u32_t tnep_msg_max_size;
static bool tnep_mode;

/* Text message in English with its language code. */
static const u8_t en_payload[] = {
	'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'
};
static const u8_t en_code[] = {'e', 'n'};

/* Text message in Polish with its language code. */
static const u8_t pl_payload[] = {
	'W', 'i', 't', 'a', 'j', ' ', 0xc5, 0x9a, 'w', 'i', 'e', 'c', 'i',
	'e', '!'
};
static const u8_t pl_code[] = {'P', 'L'};

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

static bool tnep_data_search(const u8_t *ndef_msg_buff, size_t nfc_data_len)
{
	int  err;
	u8_t desc_buf[NFC_NDEF_PARSER_REQIRED_MEMO_SIZE_CALC(MAX_NDEF_RECORDS)];
	size_t desc_buf_len = sizeof(desc_buf);
	u8_t cnt = ARRAY_SIZE(services);

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
		tnep_mode = true;

		return true;
	}

	return false;
}

static void transfer_handler(struct k_work *work)
{
	nfc_tag_detect(false);
}

static int tnep_ndef_read(u8_t *ndef_buff, u16_t ndef_len)
{
	return nfc_t4t_hl_procedure_ndef_read(&NFC_T4T_CC_DESC(t4t_cc),
					      ndef_buff, ndef_len);
}

static int tnep_ndef_update(const u8_t *ndef_buff, u16_t ndef_len)
{
	return nfc_t4t_hl_procedure_ndef_update(&NFC_T4T_CC_DESC(t4t_cc),
						(u8_t *)ndef_buff, ndef_len);
}

static struct nfc_tnep_poller_ndef_api tnep_ndef_api = {
	.ndef_read = tnep_ndef_read,
	.ndef_update = tnep_ndef_update
};

static void nfc_field_on(void)
{
	printk("NFC field on.\n");

	nfc_tag_detect(false);
}

static void nfc_timeout(bool tag_sleep)
{
	if (tag_sleep) {
		printk("Tag sleep or no detected, sending ALL Request.\n");
	} else if (tag_type == NFC_TNEP_TAG_TYPE_T4T) {
		nfc_t4t_isodep_on_timeout();

		return;
	}

	/* Sleep will block processing loop. Accepted as it is short. */
	k_sleep(ALL_REQ_DELAY);

	nfc_tag_detect(true);
}

static void nfc_field_off(void)
{
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

	if (tag_info->type == ST25R3911B_NFCA_TAG_TYPE_T4T) {
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

static void transfer_completed(const u8_t *data, size_t len, int err)
{
	if (err) {
		printk("NFC Transfer error: %d.\n", err);
		return;
	}

	err = nfc_t4t_isodep_data_received(data, len, err);
	if (err) {
		printk("NFC-A T4T read error: %d.\n", err);
	}
}

static void tag_sleep(void)
{
	printk("Tag entered the Sleep state.\n");
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

static void t4t_isodep_data_send(u8_t *data, size_t data_len, u32_t ftd)
{
	int err;

	tx_buf.data = data;
	tx_buf.len = data_len;

	err = st25r3911b_nfca_transfer_with_crc(&tx_buf, &rx_buf, ftd);
	if (err) {
		printk("NFC T4T ISO-DEP transfer error: %d.\n", err);
	}
}

static void t4t_isodep_received(const u8_t *data, size_t data_len)
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

	k_delayed_work_submit(&transmit_work, TRANSMIT_DELAY);
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

			/* Select first service. */
			err = nfc_tnep_poller_svc_select(&tnep_rx_buf,
							 services, tnep_msg_max_size);
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

		k_delayed_work_submit(&transmit_work, TRANSMIT_DELAY);
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

static void t4t_hl_ndef_read(u16_t file_id, const u8_t *data, size_t len)
{
	int err;
	struct nfc_t4t_cc_file *cc;
	struct nfc_t4t_tlv_block *tlv_block;

	printk("NDEF file read, id: 0x%x.\n", file_id);

	if (tnep_mode) {
		err = nfc_tnep_poller_on_ndef_read(data + TYPE_4_TAG_NLEN_FIELD_SIZE,
						   len - TYPE_4_TAG_NLEN_FIELD_SIZE);
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
			if (tnep_data_search(tlv_block[i].value.file.content + TYPE_4_TAG_NLEN_FIELD_SIZE,
					     tlv_block[i].value.file.len - TYPE_4_TAG_NLEN_FIELD_SIZE)) {
				tnep_msg_max_size = tlv_block[i].value.max_file_size;

				/* In case when NFC Tag device contains more
				 * than one NDEF Message, select the
				 * NDEF file which contains it.
				 */
				err = nfc_t4t_hl_procedure_ndef_file_select(tlv_block->value.file_id);
				if (err) {
					printk("TNEP Initial NDEF Message select error: %d.\n", err);
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

static void t4t_hl_ndef_update(u16_t file_id)
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

void tnep_svc_selected(const struct nfc_ndef_tnep_rec_svc_param *param,
		       const struct nfc_tnep_poller_msg *msg,
		       bool timeout)
{
	int err;
	char name[NFC_TNEP_SVC_NAME_MAX_LEN];

	NFC_NDEF_MSG_DEF(update_msg, NFC_TNEP_MAX_RECORD);
	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_en_text_rec,
				      UTF_8,
				      en_code,
				      sizeof(en_code) - 1,
				      en_payload,
				      sizeof(en_payload) - 1);
	NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_pl_text_rec,
				      UTF_8,
				      pl_code,
				      sizeof(pl_code) - 1,
				      pl_payload,
				      sizeof(pl_payload) - 1);

	if (timeout) {
		err = nfc_tnep_poller_svc_deselect();
		if (err) {
			printk("TNEP service deselect error: %d.\n", err);
		}

		return;
	}

	strncpy(name, param->uri, param->uri_length);

	printk("TNEP Service selected. Service URI: %s.\n", name);
	printk("Service status: %d.\n", msg->status);

	if (msg->status != 0) {
		printk("Service status indicates TNEP protocol or service error.\n");
		return;
	}

	if (msg->msg->record_count == 1) {
		printk("Service data contains only status record. Communication is finished.\n");
		printk("Deselecting service.\n");

		err = nfc_tnep_poller_svc_deselect();
		if (err) {
			printk("TNEP service deselect error: %d.\n", err);
		}

		return;
	}

	printk("Service message:\n");

	nfc_ndef_msg_printout(msg->msg);

	/* Add text records to NDEF text message */
	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(update_msg),
				   &NFC_NDEF_TEXT_RECORD_DESC(nfc_en_text_rec));
	if (err) {
		printk("Cannot add first record to TNEP update message!\n");
		return;
	}

	err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(update_msg),
				   &NFC_NDEF_TEXT_RECORD_DESC(nfc_pl_text_rec));
	if (err) {
		printk("Cannot add second record to TNEP update message!\n");
		return;
	}

	/* Update Service data. */
	err = nfc_tnep_poller_svc_write(&NFC_NDEF_MSG(update_msg),
					&tnep_rx_buf);
	if (err) {
		printk("Service Update error: %d.\n", err);
	}
}

void tnep_svc_deselected(void)
{
	int err;

	printk("TNEP Service deselected.\n");

	err = nfc_t4t_isodep_tag_deselect();
	if (err) {
		printk("NFC T4T Deselect error: %d.\n", err);
	}
}

void tnep_svc_sent(const struct nfc_ndef_tnep_rec_svc_param *param,
		   const struct nfc_tnep_poller_msg *rsp_msg,
		   bool timeout)
{
	int err;

	if (timeout) {
		err = nfc_tnep_poller_svc_deselect();
		if (err) {
			printk("TNEP service deselect error: %d.\n", err);
		}

		return;
	}

	printk("TNEP service updated.\n");

	if (rsp_msg->status != 0) {
		printk("TNEP protocol or service error.\n");
		return;
	}

	if (rsp_msg->msg->record_count == 1) {
		printk("Service data contains only status record. Communication is finished. Deselecting service.\n");
	}

	err = nfc_tnep_poller_svc_deselect();
	if (err) {
		printk("TNEP service deselect error: %d.\n", err);
	}
}

void tnep_error(int err)
{
	printk("TNEP error: %d.\n", err);
}

static struct nfc_tnep_poller_cb tnep_cb = {
	.svc_selected = tnep_svc_selected,
	.svc_deselected = tnep_svc_deselected,
	.svc_sent = tnep_svc_sent,
	.error = tnep_error
};

void main(void)
{
	int err;

	printk("NFC reader sample started.\n");
	nfc_t4t_hl_procedure_cb_register(&t4t_hl_procedure_cb);

	k_delayed_work_init(&transmit_work, transfer_handler);

	err = nfc_tnep_poller_init(&tnep_tx_buf, &tnep_cb);
	if (err) {
		printk("NFC TNEP Protocol initialization err: %d\n", err);
		return;
	}

	err = nfc_t4t_isodep_init(tx_data, sizeof(tx_data),
				  t4t.data, sizeof(t4t.data),
				  &t4t_isodep_cb);
	if (err) {
		printk("NFC T4T ISO-DEP Protocol initialization failed err: %d.\n",
		       err);
		return;
	}

	err = st25r3911b_nfca_init(events, ARRAY_SIZE(events), &cb);
	if (err) {
		printk("NFCA initialization failed err: %d.\n", err);
		return;
	}

	err = st25r3911b_nfca_field_on();
	if (err) {
		printk("Field on error %d.", err);
		return;
	}

	while (true) {
		k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		err = st25r3911b_nfca_process();
		if (err) {
			printk("NFC-A process failed, err: %d.\n", err);
			return;
		}
	}
}
