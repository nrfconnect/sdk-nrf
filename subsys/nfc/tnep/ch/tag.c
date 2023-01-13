/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <nfc/tnep/ch.h>

#include <nfc/ndef/ch_msg.h>
#include <nfc/ndef/ch_rec_parser.h>
#include <nfc/ndef/msg_parser.h>

#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_DECLARE(nfc_tnep_ch);

enum nfc_tnep_ch_role {
	NFC_TNEP_CH_ROLE_UNKNOWN,
	NFC_TNEP_CH_ROLE_SELECTOR,
	NFC_TNEP_CH_ROLE_REQUESTER
};

struct ch_tnep_control {
	struct nfc_tnep_ch_cb *cb;
	enum nfc_tnep_ch_role role;
	bool mediation_req;
	bool initial_req;
	bool carrier_inactive;
};

static struct ch_tnep_control ch_tnep_ctrl;

static bool hc_rec_check(const struct nfc_tnep_ch_record *ch_rec,
			 const struct nfc_ndef_record_desc *rec)
{
	return ((ch_rec->count == 1) &&
		(ch_rec->ac->carrier_data_ref.length == rec->id_length) &&
		(memcmp(ch_rec->ac->carrier_data_ref.data, rec->id, rec->id_length) == 0) &&
		nfc_ndef_ch_hc_rec_check(rec));
}

static bool mediation_req_check(const struct nfc_ndef_ch_hc_rec *hc_rec)
{
	return ((hc_rec->ctf == TNF_WELL_KNOWN) &&
		(hc_rec->carrier.type_len == sizeof(nfc_ndef_ch_hm_rec_type_field)) &&
		(memcmp(hc_rec->carrier.type, nfc_ndef_ch_hm_rec_type_field,
		sizeof(nfc_ndef_ch_hm_rec_type_field)) == 0));
}

static int select_msg_handle(const struct nfc_ndef_msg_desc *msg,
			     struct net_buf_simple *buf)
{
	int err;
	struct nfc_tnep_ch_record ch_select;

	err = nfc_tnep_ch_select_msg_parse(msg, &ch_select, buf);
	if (err) {
		LOG_ERR("Parsing Connection Handover Select message failed: %d",
			err);
		return err;
	}

	ch_tnep_ctrl.carrier_inactive =
			!nfc_tnep_ch_active_carrier_check(&ch_select);

	if (ch_tnep_ctrl.cb->select_msg_recv) {
		err = ch_tnep_ctrl.cb->select_msg_recv(&ch_select,
						       ch_tnep_ctrl.carrier_inactive);
		return err;
	}

	return 0;
}

static int request_msg_handle(const struct nfc_ndef_msg_desc *msg,
			      struct net_buf_simple *buf)
{
	int err;
	struct nfc_tnep_ch_request ch_req;
	const struct nfc_ndef_record_desc *rec;

	err = nfc_tnep_ch_request_msg_parse(msg, &ch_req, buf);
	if (err) {
		LOG_ERR("Parsing Connection Handover Request message failed: %d",
			err);
		return err;
	}

	rec = *ch_req.ch_record.carrier;

	/* Check if Request is the Mediation message. */
	if (hc_rec_check(&ch_req.ch_record, rec)) {
		const struct nfc_ndef_ch_hc_rec *hc_rec;

		err = nfc_tnep_ch_hc_rec_parse(&hc_rec, rec, buf);
		if (err) {
			return err;
		}

		if (mediation_req_check(hc_rec)) {
			/* Mediation Request Received */
			ch_tnep_ctrl.mediation_req = true;

			if (ch_tnep_ctrl.cb->mediation_req_recv) {
				err = ch_tnep_ctrl.cb->mediation_req_recv();
				return err;
			}
		}
	}

	if (ch_tnep_ctrl.cb->request_msg_recv) {
		err = ch_tnep_ctrl.cb->request_msg_recv(&ch_req);
		return err;
	}

	return 0;
}

static int initiate_msg_handle(const struct nfc_ndef_msg_desc *msg,
			       struct net_buf_simple *buf)
{
	int err;
	struct nfc_tnep_ch_record ch_init;

	err = nfc_tnep_ch_initiate_msg_parse(msg, &ch_init, buf);
	if (err) {
		LOG_ERR("Parsing Connection Handover Initiate message failed: %d",
			err);
		return err;
	}

	if (ch_tnep_ctrl.cb->initial_msg_recv) {
		err = ch_tnep_ctrl.cb->initial_msg_recv(&ch_init);
		return err;
	}

	return 0;
}

static void error_handler(int err)
{
	if (ch_tnep_ctrl.cb->error) {
		ch_tnep_ctrl.cb->error(err);
	}
}

static void ch_svc_selected(void)
{
	int err;

	LOG_INF("TNEP Handover Service selected");

	if (ch_tnep_ctrl.role == NFC_TNEP_CH_ROLE_REQUESTER) {
		if (ch_tnep_ctrl.cb->request_msg_prepare) {
			ch_tnep_ctrl.cb->request_msg_prepare();
		}
	} else {
		err = nfc_tnep_tag_tx_msg_no_app_data();
		if (err) {
			error_handler(err);
		}
	}
}

static void ch_svc_deselected(void)
{
	LOG_INF("TNEP Connection Handover Service deselected");

	ch_tnep_ctrl.initial_req = false;
	ch_tnep_ctrl.mediation_req = false;
}

static void ch_svc_msg_received(const uint8_t *data, size_t len)
{
	int err;
	struct nfc_ndef_msg_desc *ch_msg;
	uint8_t desc_buf[NFC_NDEF_PARSER_REQUIRED_MEM(CONFIG_NFC_TNEP_CH_MAX_RECORD_COUNT)];
	size_t desc_buf_len = sizeof(desc_buf);

	NET_BUF_SIMPLE_DEFINE(ch_buf,
			      CONFIG_NFC_TNEP_CH_PARSER_BUFFER_SIZE);

	LOG_INF("Received new TNEP Connection Handover message");

	err = nfc_ndef_msg_parse(desc_buf, &desc_buf_len, data, &len);
	if (err) {
		LOG_ERR("Received Message parse error: %d", err);
		error_handler(err);
	}

	ch_msg = (struct nfc_ndef_msg_desc *)desc_buf;

	if (!ch_msg || !ch_msg->record_count) {
		LOG_ERR("NDEF Message does not contains any NDEF record");
		error_handler(0);
	}

	/* Check Connection Handover Record */
	switch (ch_tnep_ctrl.role) {
	case NFC_TNEP_CH_ROLE_SELECTOR:
		if (nfc_ndef_ch_rec_check(*ch_msg->record,
				   NFC_NDEF_CH_REC_TYPE_HANDOVER_INITIATE)) {
			LOG_DBG("Handover Initiate message received");

			err = initiate_msg_handle(ch_msg, &ch_buf);
		} else if (nfc_ndef_ch_rec_check(*ch_msg->record,
				   NFC_NDEF_CH_REC_TYPE_HANDOVER_REQUEST)) {
			LOG_DBG("Handover Request message received");

			err = request_msg_handle(ch_msg, &ch_buf);
		} else {
			LOG_ERR("Received not expected NDEF message");
			err = -EFAULT;
		}

		break;

	case NFC_TNEP_CH_ROLE_REQUESTER:
		if (nfc_ndef_ch_rec_check(*ch_msg->record,
				   NFC_NDEF_CH_REC_TYPE_HANDOVER_SELECT)) {
			LOG_DBG("Handover Select message received");
			err = select_msg_handle(ch_msg, &ch_buf);
			if (!err) {
				err = nfc_tnep_tag_tx_msg_no_app_data();
			}
		} else {
			LOG_ERR("Received not expected NDEF message");
			err = -EFAULT;
		}

		break;

	default:
		LOG_ERR("Unknown Connection Handover role");
		err = -ENOTSUP;
		break;
	}

	if (err) {
		error_handler(err);
	}
}

static void ch_svc_error(int err_code)
{
	LOG_ERR("TNEP Connection Handover Service error: %d", err_code);
	error_handler(err_code);
}

NFC_TNEP_TAG_SERVICE_DEF(ch_tnep, nfc_tnep_ch_svc_uri,
			 sizeof(nfc_tnep_ch_svc_uri),
			 NFC_TNEP_COMM_MODE_SINGLE_RESPONSE,
			 CONFIG_NFC_TNEP_CH_MIN_WAIT_TIME,
			 CONFIG_NFC_TNEP_CH_MAX_TIME_EXTENSION,
			 CONFIG_NFC_TNEP_CH_MAX_NDEF_SIZE,
			 ch_svc_selected, ch_svc_deselected,
			 ch_svc_msg_received, ch_svc_error);

int nfc_tnep_ch_carrier_set(struct nfc_ndef_record_desc *ac_rec,
			    struct nfc_ndef_record_desc *carrier_rec,
			    size_t count)
{
	struct nfc_ndef_ch_msg_records ch_rec;

	if (!ac_rec || !carrier_rec || (count < 1)) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	ch_rec.ac = ac_rec;
	ch_rec.carrier = carrier_rec;
	ch_rec.cnt = count;

	switch (ch_tnep_ctrl.role) {
	case NFC_TNEP_CH_ROLE_SELECTOR:
		if (ch_tnep_ctrl.mediation_req) {
			ch_tnep_ctrl.mediation_req = false;

			LOG_DBG("Answering with Connection Handover Mediation message");

			return nfc_tnep_ch_tnep_mediation_msg_prepare(&ch_rec);
		}

		if (ch_tnep_ctrl.initial_req) {
			ch_tnep_ctrl.initial_req = false;

			LOG_DBG("Answering with Connection Handover Initiate message");

			return nfc_tnep_ch_initiate_msg_prepare(&ch_rec);
		}

		LOG_DBG("Answering with Connection Handover Select message");

		return nfc_tnep_ch_select_msg_prepare(&ch_rec);

	case NFC_TNEP_CH_ROLE_REQUESTER:
		LOG_DBG("Sending Connection Handover Request message");

		return nfc_tnep_ch_request_msg_prepare(&ch_rec);

	default:
		LOG_ERR("Unknown Connection Handover role");
		return -ENOTSUP;
	}

	return 0;
}

int nfc_tnep_ch_service_init(struct nfc_tnep_ch_cb *cb)
{
	if (!cb) {
		LOG_ERR("NULL callback structure");
		return -EINVAL;
	}

	if (cb->request_msg_prepare && cb->select_msg_recv) {
		ch_tnep_ctrl.role = NFC_TNEP_CH_ROLE_REQUESTER;
		LOG_INF("Connection Handover Requester role");
	} else if (cb->request_msg_recv) {
		ch_tnep_ctrl.role = NFC_TNEP_CH_ROLE_SELECTOR;
		LOG_INF("Connection Handover Selector role");
	} else {
		ch_tnep_ctrl.role = NFC_TNEP_CH_ROLE_UNKNOWN;
		LOG_ERR("Invalid callback set");
		return -ENOTSUP;
	}

	ch_tnep_ctrl.cb = cb;

	return 0;
}

int nfc_tnep_ch_msg_write(struct nfc_ndef_msg_desc *msg)
{
	return nfc_tnep_tag_tx_msg_app_data(msg,
					    NFC_TNEP_STATUS_SUCCESS);
}
