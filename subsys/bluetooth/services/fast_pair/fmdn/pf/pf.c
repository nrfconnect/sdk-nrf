/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fmdn_pf, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fhn/pf/pf.h>

#include "fp_activation.h"
#include "fp_fmdn_pf.h"
#include "fp_fmdn_pf_oob_msg.h"

/* The supported version of the message format. */
#define OOB_MSG_VERSION (0x01)

/* Collection of flags to manage the state of the ranging communication channel. */
enum ranging_comm_chan_flag {
	/* If the correct Ranging Capability Request message is received and
	 * the Initiator requires a response.
	 */
	RANGING_COMM_CHAN_FLAG_CAPABILITY_REQ_PENDING,

	/* If the correct Ranging Configuration message is received and
	 * the Initiator requires a response.
	 */
	RANGING_COMM_CHAN_FLAG_CONFIG_REQ_PENDING,

	/* If the correct Stop Ranging message is received and
	 * the Initiator requires a response.
	 */
	RANGING_COMM_CHAN_FLAG_STOP_REQ_PENDING,

	/* If the communication channel for managing ranging is established. */
	RANGING_COMM_CHAN_FLAG_ESTABLISHED,

	/* If the Account Key has been assigned to this communication channel. */
	RANGING_COMM_CHAN_FLAG_ACCOUNT_KEY_PROVIDED,

	/* Total number of flags - must be at the end */
	RANGING_COMM_CHAN_FLAGS,
};

struct ranging_comm_chan {
	/* Bluetooth connection object associated with the communication channel. */
	struct bt_conn *conn;

	/* Flags for SMP state machine */
	ATOMIC_DEFINE(flags, RANGING_COMM_CHAN_FLAGS);

	/* Account Key associated with the communication channel. */
	struct fp_account_key account_key;

	/* Bitmask of active ranging technologies. */
	uint16_t ranging_tech_bm;
};

static struct ranging_comm_chan ranging_comm_chans[CONFIG_BT_MAX_CONN];

static const struct bt_fast_pair_fhn_pf_ranging_mgmt_cb *ranging_mgmt_cb;

static struct ranging_comm_chan *ranging_comm_chan_get(struct bt_conn *conn)
{
	uint8_t index;

	index = bt_conn_index(conn);
	__ASSERT_NO_MSG(index < ARRAY_SIZE(ranging_comm_chans));

	return &ranging_comm_chans[index];
}

static void pf_connected(struct bt_conn *conn, uint8_t err)
{
	struct ranging_comm_chan *chan;

	if (err) {
		return;
	}

	chan = ranging_comm_chan_get(conn);
	__ASSERT_NO_MSG(memcmp(chan, &((struct ranging_comm_chan) {0}), sizeof(*chan)) == 0);
	chan->conn = conn;
}

static void pf_disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct ranging_comm_chan *chan;

	chan = ranging_comm_chan_get(conn);
	if (atomic_test_bit(chan->flags, RANGING_COMM_CHAN_FLAG_ESTABLISHED)) {
		__ASSERT_NO_MSG(ranging_mgmt_cb);

		if (ranging_mgmt_cb->comm_channel_terminated) {
			ranging_mgmt_cb->comm_channel_terminated(chan->conn);
		}
	}

	memset(chan, 0, sizeof(*chan));
}

BT_CONN_CB_DEFINE(pf_conn_callbacks) = {
	.connected = pf_connected,
	.disconnected = pf_disconnected,
};

bool pf_ranging_req_handle(struct ranging_comm_chan *chan,
			   struct fp_fmdn_pf_oob_msg *req,
			   struct net_buf_simple *req_buf,
			   enum fp_fmdn_pf_oob_msg_id msg_id)
{
	int err;

	__ASSERT_NO_MSG(chan);
	__ASSERT_NO_MSG(req_buf);
	__ASSERT_NO_MSG(req);

	/* Decode the incoming message and validate its correctness. */
	err = fp_fmdn_pf_oob_msg_decode(req_buf, req);
	if (err) {
		LOG_WRN("Precision Finding: Ranging Request (0x%02X): "
			"rejecting invalid request", msg_id);
		return false;
	}

	if (req->id != msg_id) {
		LOG_WRN("Precision Finding: Ranging Request (0x%02X): "
			"unexpected message ID: 0x%02X", msg_id, req->id);
		return false;
	}

	if (req->version > OOB_MSG_VERSION) {
		if (msg_id != FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_REQ) {
			LOG_WRN("Precision Finding: Ranging Request (0x%02X): "
				"Initiator uses unsupported version: %u",
				msg_id, req->version);
			return false;
		}

		LOG_WRN("Precision Finding: Ranging Capability Request: "
			"Initiator supports higher version of the Ranging OOB specification");
	}

	/* Check if this action is allowed in the current state and transition to the new state. */
	if (atomic_test_bit(chan->flags, RANGING_COMM_CHAN_FLAG_CAPABILITY_REQ_PENDING) ||
	    atomic_test_bit(chan->flags, RANGING_COMM_CHAN_FLAG_CONFIG_REQ_PENDING) ||
	    atomic_test_bit(chan->flags, RANGING_COMM_CHAN_FLAG_STOP_REQ_PENDING)) {
		LOG_WRN("Precision Finding: Ranging Request (0x%02X): "
			"rejecting request as the previous request is still pending",
			msg_id);
		return false;
	}

	return true;
}

bool fp_fmdn_pf_ranging_capability_req_handle(struct bt_conn *conn,
					      struct net_buf_simple *req_buf)
{
	struct ranging_comm_chan *chan;
	struct fp_fmdn_pf_oob_msg req = {0};
	enum fp_fmdn_pf_oob_msg_id msg_id = FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_REQ;

	__ASSERT_NO_MSG(conn);

	/* Decode the request and perform generic validation. */
	chan = ranging_comm_chan_get(conn);
	if (!pf_ranging_req_handle(chan, &req, req_buf, msg_id)) {
		return false;
	}

	/* Perform request-specific validation and transition to the new state. */
	atomic_set_bit(chan->flags, RANGING_COMM_CHAN_FLAG_CAPABILITY_REQ_PENDING);
	atomic_set_bit(chan->flags, RANGING_COMM_CHAN_FLAG_ESTABLISHED);

	chan->ranging_tech_bm = req.ranging_capability_req.ranging_tech_bm;

	/* Trigger the application callback. */
	__ASSERT_NO_MSG(ranging_mgmt_cb);
	__ASSERT_NO_MSG(ranging_mgmt_cb->ranging_capability_request);
	ranging_mgmt_cb->ranging_capability_request(
		conn, req.ranging_capability_req.ranging_tech_bm);

	return true;
}

bool fp_fmdn_pf_ranging_config_req_handle(struct bt_conn *conn,
					  struct net_buf_simple *req_buf)
{
	struct ranging_comm_chan *chan;
	struct bt_fast_pair_fhn_pf_ranging_tech_payload config_payloads[
		FP_FMDN_PF_OOB_MSG_RANGING_TECH_PAYLOAD_MAX_NUM];
	struct fp_fmdn_pf_oob_msg req = {0};
	enum fp_fmdn_pf_oob_msg_id msg_id = FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_REQ;

	__ASSERT_NO_MSG(conn);

	/* Decode the request and perform generic validation. */
	chan = ranging_comm_chan_get(conn);

	req.ranging_config_req.config_payloads = config_payloads;
	req.ranging_config_req.config_payload_num = ARRAY_SIZE(config_payloads);
	if (!pf_ranging_req_handle(chan, &req, req_buf, msg_id)) {
		return false;
	}

	/* Perform request-specific validation and transition to the new state. */
	if ((chan->ranging_tech_bm != 0) &&
	    (req.ranging_config_req.ranging_tech_bm & ~chan->ranging_tech_bm) > 0) {
		LOG_ERR("Precision Finding: Ranging Configuration: "
			"request cannot refer to unsupported ranging technologies: 0x%04X",
			req.ranging_config_req.ranging_tech_bm & ~chan->ranging_tech_bm);
		return false;
	}

	atomic_set_bit(chan->flags, RANGING_COMM_CHAN_FLAG_CONFIG_REQ_PENDING);
	atomic_set_bit(chan->flags, RANGING_COMM_CHAN_FLAG_ESTABLISHED);

	chan->ranging_tech_bm = req.ranging_config_req.ranging_tech_bm;

	/* Trigger the application callback. */
	__ASSERT_NO_MSG(ranging_mgmt_cb);
	__ASSERT_NO_MSG(ranging_mgmt_cb->ranging_config_request);
	ranging_mgmt_cb->ranging_config_request(conn,
						req.ranging_config_req.ranging_tech_bm,
						req.ranging_config_req.config_payloads,
						req.ranging_config_req.config_payload_num);

	return true;
}

bool fp_fmdn_pf_stop_ranging_req_handle(struct bt_conn *conn,
					struct net_buf_simple *req_buf)
{
	struct ranging_comm_chan *chan;
	struct fp_fmdn_pf_oob_msg req = {0};
	enum fp_fmdn_pf_oob_msg_id msg_id = FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_REQ;

	__ASSERT_NO_MSG(conn);

	/* Decode the request and perform generic validation. */
	chan = ranging_comm_chan_get(conn);
	if (!pf_ranging_req_handle(chan, &req, req_buf, msg_id)) {
		return false;
	}

	/* Perform request-specific validation and transition to the new state. */
	if (chan->ranging_tech_bm == 0) {
		LOG_WRN("Precision Finding: Stop Ranging: "
			"rejecting request as ranging has not been configured");
		return false;
	}

	if ((req.stop_ranging_req.ranging_tech_bm & ~chan->ranging_tech_bm) > 0) {
		LOG_ERR("Precision Finding: Stop Ranging: "
			"request cannot refer to unconfigured ranging technologies: 0x%04X",
			req.stop_ranging_req.ranging_tech_bm & ~chan->ranging_tech_bm);
		return false;
	}

	atomic_set_bit(chan->flags, RANGING_COMM_CHAN_FLAG_STOP_REQ_PENDING);

	/* Trigger the application callback. */
	__ASSERT_NO_MSG(ranging_mgmt_cb);
	__ASSERT_NO_MSG(ranging_mgmt_cb->stop_ranging_request);
	ranging_mgmt_cb->stop_ranging_request(
		conn, req.stop_ranging_req.ranging_tech_bm);

	return true;
}

static int pf_ranging_rsp_prepare(struct bt_conn *conn,
				  struct fp_fmdn_pf_oob_msg *rsp,
				  struct net_buf_simple *rsp_buf)
{
	int err;
	uint16_t ranging_tech_bm;
	enum ranging_comm_chan_flag req_flag;
	struct ranging_comm_chan *chan;

	__ASSERT_NO_MSG(rsp);

	/* Map the message ID to the request pending flag and
	 * initialize ranging technology bitmask.
	 */
	switch (rsp->id) {
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_RSP:
		req_flag = RANGING_COMM_CHAN_FLAG_CAPABILITY_REQ_PENDING;
		ranging_tech_bm = rsp->ranging_capability_rsp.ranging_tech_bm;
		break;
	case FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_RSP:
		req_flag = RANGING_COMM_CHAN_FLAG_CONFIG_REQ_PENDING;
		ranging_tech_bm = rsp->ranging_config_rsp.ranging_tech_bm;
		break;
	case FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_RSP:
		req_flag = RANGING_COMM_CHAN_FLAG_STOP_REQ_PENDING;
		ranging_tech_bm = rsp->stop_ranging_rsp.ranging_tech_bm;
		break;
	default:
		__ASSERT_NO_MSG(0);
		return -EINVAL;
	}

	/* Check if this action is allowed in the current state and transition to the new state. */
	chan = ranging_comm_chan_get(conn);
	if (!atomic_test_bit(chan->flags, req_flag)) {
		LOG_ERR("Precision Finding: Ranging Response (0x%02X): "
			"cannot send response without the proceeding request", rsp->id);
		return -ESRCH;
	}

	if ((ranging_tech_bm & ~chan->ranging_tech_bm) > 0) {
		LOG_ERR("Precision Finding: Ranging Response (0x%02X): "
			"response cannot refer to unrequested ranging technologies: 0x%04X",
			rsp->id, ranging_tech_bm & ~chan->ranging_tech_bm);
		return -EINVAL;
	}

	err = fp_fmdn_pf_oob_msg_encode(rsp, rsp_buf);
	if (err) {
		LOG_ERR("Precision Finding: Ranging Capability Response (0x%02X): "
			"cannot encode the response: %d", rsp->id, err);
		return err;
	}

	return 0;
}

int fp_fmdn_pf_ranging_capability_rsp_prepare(
	struct bt_conn *conn,
	uint16_t ranging_tech_bm,
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *capability_payloads,
	uint8_t capability_payload_num,
	struct net_buf_simple *rsp_buf)
{
	struct fp_fmdn_pf_oob_msg rsp = {
		.id = FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_RSP,
		.version = OOB_MSG_VERSION,
		.ranging_capability_rsp = {
			.ranging_tech_bm = ranging_tech_bm,
			.capability_payloads = capability_payloads,
			.capability_payload_num = capability_payload_num,
		},
	};

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(rsp_buf);

	return pf_ranging_rsp_prepare(conn, &rsp, rsp_buf);
}

int fp_fmdn_pf_ranging_config_rsp_prepare(struct bt_conn *conn,
					  uint16_t ranging_tech_bm,
					  struct net_buf_simple *rsp_buf)
{
	struct fp_fmdn_pf_oob_msg rsp = {
		.id = FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_RSP,
		.version = OOB_MSG_VERSION,
		.ranging_config_rsp = {
			.ranging_tech_bm = ranging_tech_bm,
		},
	};

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(rsp_buf);

	return pf_ranging_rsp_prepare(conn, &rsp, rsp_buf);
}

int fp_fmdn_pf_stop_ranging_rsp_prepare(struct bt_conn *conn,
					uint16_t ranging_tech_bm,
					struct net_buf_simple *rsp_buf)
{
	struct fp_fmdn_pf_oob_msg rsp = {
		.id = FP_FMDN_PF_OOB_MSG_ID_STOP_RANGING_RSP,
		.version = OOB_MSG_VERSION,
		.stop_ranging_rsp = {
			.ranging_tech_bm = ranging_tech_bm,
		},
	};

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(rsp_buf);

	return pf_ranging_rsp_prepare(conn, &rsp, rsp_buf);
}

void fp_fmdn_pf_ranging_rsp_sent_notify(struct bt_conn *conn, uint16_t ranging_tech_bm)
{
	bool is_stop_rsp = false;
	uint8_t active_req_flag_cnt = 0;
	struct ranging_comm_chan *chan = ranging_comm_chan_get(conn);
	enum ranging_comm_chan_flag req_flags[] = {
		RANGING_COMM_CHAN_FLAG_CAPABILITY_REQ_PENDING,
		RANGING_COMM_CHAN_FLAG_CONFIG_REQ_PENDING,
		RANGING_COMM_CHAN_FLAG_STOP_REQ_PENDING,
	};

	for (uint8_t i = 0; i < ARRAY_SIZE(req_flags); i++) {
		if (atomic_test_bit(chan->flags, req_flags[i])) {
			atomic_clear_bit(chan->flags, req_flags[i]);
			active_req_flag_cnt++;

			if (atomic_test_bit(
				chan->flags, RANGING_COMM_CHAN_FLAG_STOP_REQ_PENDING)) {
				is_stop_rsp = true;
			}
		}
	}

	__ASSERT_NO_MSG(active_req_flag_cnt == 1);

	chan->ranging_tech_bm = is_stop_rsp ?
		(chan->ranging_tech_bm & ~ranging_tech_bm) : ranging_tech_bm;
}

void fp_fmdn_pf_account_key_set(struct bt_conn *conn, struct fp_account_key *account_key)
{
	struct ranging_comm_chan *chan;

	chan = ranging_comm_chan_get(conn);
	memcpy(&chan->account_key, account_key, sizeof(chan->account_key));
	atomic_set_bit(chan->flags, RANGING_COMM_CHAN_FLAG_ACCOUNT_KEY_PROVIDED);
}

struct fp_account_key *fp_fmdn_pf_account_key_get(struct bt_conn *conn)
{
	struct ranging_comm_chan *chan;

	chan = ranging_comm_chan_get(conn);
	if (!atomic_test_bit(chan->flags, RANGING_COMM_CHAN_FLAG_ACCOUNT_KEY_PROVIDED)) {
		return NULL;
	}

	return &chan->account_key;
}

bool bt_fast_pair_fhn_pf_ranging_tech_bm_check(uint16_t bm,
					       enum bt_fast_pair_fhn_pf_ranging_tech_id id)
{
	__ASSERT_NO_MSG(id <= BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI);

	return ((bm & BIT(id)) > 0);
}

void bt_fast_pair_fhn_pf_ranging_tech_bm_write(uint16_t *bm,
					       enum bt_fast_pair_fhn_pf_ranging_tech_id id,
					       bool value)
{
	__ASSERT_NO_MSG(bm);
	__ASSERT_NO_MSG(id <= BT_FAST_PAIR_FHN_PF_RANGING_TECH_ID_BLE_RSSI);

	WRITE_BIT(*bm, id, value);
}

int bt_fast_pair_fhn_pf_ranging_mgmt_cb_register(
	const struct bt_fast_pair_fhn_pf_ranging_mgmt_cb *cb)
{
	if (bt_fast_pair_is_ready()) {
		return -EOPNOTSUPP;
	}

	if (!cb ||
	    !cb->ranging_capability_request ||
	    !cb->ranging_config_request ||
	    !cb->stop_ranging_request) {
		return -EINVAL;
	}

	ranging_mgmt_cb = cb;

	return 0;
}

static void pf_oob_msg_len_validate(void)
{
	uint8_t max_len;

	if (!IS_ENABLED(CONFIG_ASSERT)) {
		return;
	}

	max_len = fp_fmdn_pf_oob_msg_max_len_get(FP_FMDN_PF_OOB_MSG_ID_RANGING_CAPABILITY_RSP);
	__ASSERT_NO_MSG(max_len == FP_FMDN_PF_RANGING_CAPABILITY_RSP_MAX_LEN);

	max_len = fp_fmdn_pf_oob_msg_max_len_get(FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_RSP);
	__ASSERT_NO_MSG(max_len == FP_FMDN_PF_RANGING_CONFIG_RSP_LEN);

	max_len = fp_fmdn_pf_oob_msg_max_len_get(FP_FMDN_PF_OOB_MSG_ID_RANGING_CONFIG_RSP);
	__ASSERT_NO_MSG(max_len == FP_FMDN_PF_STOP_RANGING_RSP_LEN);
}

static int init(void)
{
	pf_oob_msg_len_validate();

	if (!ranging_mgmt_cb) {
		LOG_ERR("Precision Finding: ranging management callbacks not registered");
		return -EACCES;
	}

	return 0;
}

static int uninit(void)
{
	/* Trigger the termination callback for active connections. */
	for (uint8_t i = 0; i < ARRAY_SIZE(ranging_comm_chans); i++) {
		struct ranging_comm_chan *chan;

		chan = &ranging_comm_chans[i];
		if (atomic_test_bit(chan->flags, RANGING_COMM_CHAN_FLAG_ESTABLISHED)) {
			__ASSERT_NO_MSG(ranging_mgmt_cb);

			if (ranging_mgmt_cb->comm_channel_terminated) {
				ranging_mgmt_cb->comm_channel_terminated(chan->conn);
			}
		}
	}

	/* Clear the connection context. */
	memset(ranging_comm_chans, 0, sizeof(ranging_comm_chans));

	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_fmdn_pf,
			      FP_ACTIVATION_INIT_PRIORITY_DEFAULT,
			      init,
			      uninit);
