/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <nfc/tnep/ch.h>
#include <nfc/tnep/poller.h>

#include <nfc/ndef/tnep_rec.h>
#include <nfc/ndef/ch_rec_parser.h>
#include <nfc/ndef/ch_msg.h>

#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_DECLARE(nfc_tnep_ch);

struct ch_tnep_control {
	struct nfc_tnep_ch_cb *cb;
	bool svc_selected;
	bool request_received;
	bool carrier_inactive;
};

static struct ch_tnep_control ch_tnep_ctrl;
static uint8_t tnep_data[CONFIG_NFC_TNEP_CH_POLLER_RX_BUF_SIZE];
static struct nfc_tnep_buf tnep_buf = {
	.data = tnep_data,
	.size = sizeof(tnep_data)
};

static void error_handler(int err)
{
	ch_tnep_ctrl.svc_selected = false;
	ch_tnep_ctrl.request_received = false;
	ch_tnep_ctrl.carrier_inactive = false;

	if (ch_tnep_ctrl.cb->error) {
		ch_tnep_ctrl.cb->error(err);
	}
}

static int request_msg_handle(const struct nfc_ndef_msg_desc *msg,
			      struct net_buf_simple *buf)
{
	int err;
	struct nfc_tnep_ch_request ch_req;

	err = nfc_tnep_ch_request_msg_parse(msg, &ch_req, buf);
	if (err) {
		LOG_ERR("Parsing Connection Handover Request message failed: %d",
			err);
		return err;
	}

	ch_tnep_ctrl.request_received = true;

	if (ch_tnep_ctrl.cb->request_msg_recv) {
		err = ch_tnep_ctrl.cb->request_msg_recv(&ch_req);
		return err;
	}

	return 0;
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


static bool check_tnep_svc(const struct nfc_ndef_tnep_rec_svc_param *param)
{
	return ((param->uri_length == sizeof(nfc_tnep_ch_svc_uri)) &&
		(memcmp(param->uri, nfc_tnep_ch_svc_uri,
			sizeof(nfc_tnep_ch_svc_uri)) == 0));

}

static void tnep_svc_selected(const struct nfc_ndef_tnep_rec_svc_param *param,
			      const struct nfc_tnep_poller_msg *msg,
			      bool timeout)
{
	int err;

	NET_BUF_SIMPLE_DEFINE(ch_buf,
		      CONFIG_NFC_TNEP_CH_PARSER_BUFFER_SIZE);

	if (!check_tnep_svc(param)) {
		return;
	}

	LOG_INF("TNEP Connection Handover Service selected");

	ch_tnep_ctrl.svc_selected = true;

	if (timeout) {
		err = nfc_tnep_poller_svc_deselect();
		if (err) {
			LOG_ERR("Deselecting Connection Handover service failed %d",
				err);
			error_handler(err);
		}
	}

	if ((msg->msg->record_count == 1) &&
	    (msg->status == NFC_TNEP_STATUS_SUCCESS)) {
		if (ch_tnep_ctrl.cb->request_msg_prepare) {
			err = ch_tnep_ctrl.cb->request_msg_prepare();
			if (err) {
				LOG_ERR("Handover Request prepare error: %d",
					err);
				error_handler(err);
			}
		}

		return;
	}

	if ((nfc_ndef_ch_rec_check(*msg->msg->record,
				   NFC_NDEF_CH_REC_TYPE_HANDOVER_REQUEST))) {
		err = request_msg_handle(msg->msg, &ch_buf);
		if (err) {
			error_handler(err);
		}
	}
}

static void tnep_svc_sent(const struct nfc_ndef_tnep_rec_svc_param *param,
			  const struct nfc_tnep_poller_msg *rsp_msg,
			  bool timeout)
{
	int err;

	NET_BUF_SIMPLE_DEFINE(ch_buf,
		      CONFIG_NFC_TNEP_CH_PARSER_BUFFER_SIZE);

	if ((rsp_msg->msg->record_count == 1) &&
	    (rsp_msg->status == NFC_TNEP_STATUS_SUCCESS)) {
		LOG_DBG("Tag respond only with Status Record. Ending communication");
		err = nfc_tnep_poller_svc_deselect();
		if (err) {
			error_handler(err);
		}

		return;
	}

	if (!check_tnep_svc(param) &&
	   (!ch_tnep_ctrl.svc_selected)) {
		return;
	}

	if (timeout) {
		LOG_DBG("Service sent timeout");

		err = nfc_tnep_poller_svc_deselect();
		if (err) {
			error_handler(err);
		}

		return;
	}

	LOG_INF("Received new Connection Handover service data");

	if ((nfc_ndef_ch_rec_check(*rsp_msg->msg->record,
				   NFC_NDEF_CH_REC_TYPE_HANDOVER_SELECT))) {
		LOG_INF("Connection Handover Select message received");

		err = select_msg_handle(rsp_msg->msg, &ch_buf);
		if (!err) {
			err = nfc_tnep_poller_svc_deselect();
		}

		if (err) {
			error_handler(err);
		}
	}
}

static void tnep_svc_deselected(void)
{
	if (!ch_tnep_ctrl.svc_selected) {
		return;
	}

	ch_tnep_ctrl.svc_selected = false;
	ch_tnep_ctrl.request_received = false;
	ch_tnep_ctrl.carrier_inactive = false;

	LOG_INF("Connection Handover service deselected");
}

static struct nfc_tnep_poller_cb tnep_cb = {
	.svc_selected = tnep_svc_selected,
	.svc_sent = tnep_svc_sent,
	.svc_deselected = tnep_svc_deselected,
};

int nfc_tnep_ch_service_init(struct nfc_tnep_ch_cb *cb)
{
	if (!cb) {
		LOG_ERR("NULL callback structure");
		return -EINVAL;
	}

	if ((!cb->request_msg_prepare) ||
	    (!cb->request_msg_recv) ||
	    (!cb->select_msg_recv)) {
		LOG_ERR("Invalid callback set");
		return -EINVAL;
	}

	ch_tnep_ctrl.cb = cb;

	nfc_tnep_poller_cb_register(&tnep_cb);

	return 0;
}

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

	if (ch_tnep_ctrl.request_received) {
		LOG_DBG("Answering with Connection Handover Select message");
		return nfc_tnep_ch_select_msg_prepare(&ch_rec);
	}

	LOG_DBG("Sending Connection Handover Request message");

	return  nfc_tnep_ch_request_msg_prepare(&ch_rec);
}

const struct nfc_ndef_tnep_rec_svc_param *nfc_tnep_ch_svc_search(
	const struct nfc_ndef_tnep_rec_svc_param *services,
	size_t cnt)
{
	if (!services || !cnt) {
		LOG_ERR("NULL parameters");
		return NULL;
	}

	for (size_t i = 0; i < cnt; i++) {
		const struct nfc_ndef_tnep_rec_svc_param *ch_svc =
				&services[i];
		if ((ch_svc->uri_length == sizeof(nfc_tnep_ch_svc_uri)) &&
		    (memcmp(ch_svc->uri, nfc_tnep_ch_svc_uri, sizeof(nfc_tnep_ch_svc_uri)) == 0) &&
		    (ch_svc->communication_mode == NFC_TNEP_COMM_MODE_SINGLE_RESPONSE)) {
			return ch_svc;
		}
	}

	return NULL;
}

int nfc_tnep_ch_msg_write(struct nfc_ndef_msg_desc *msg)
{
	return nfc_tnep_poller_svc_write(msg, &tnep_buf);
}
