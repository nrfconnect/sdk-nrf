/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <bluetooth/services/ras.h>
#include <bluetooth/gatt_dm.h>
#include <stdint.h>
#include <errno.h>

#include "../ras_internal.h"

LOG_MODULE_REGISTER(ras_rreq, CONFIG_BT_RAS_RREQ_LOG_LEVEL);

enum bt_ras_rreq_cp_state {
	BT_RAS_RREQ_CP_STATE_NONE,
	BT_RAS_RREQ_CP_STATE_GET_RD_WRITTEN,
	BT_RAS_RREQ_CP_STATE_ACK_RD_WRITTEN,
};

struct bt_ras_rreq_cp {
	struct bt_gatt_subscribe_params subscribe_params;
	enum bt_ras_rreq_cp_state state;
};

struct bt_ras_on_demand_rd {
	int error_status;
	struct bt_gatt_subscribe_params subscribe_params;
	struct net_buf_simple *ranging_data_out;
	bt_ras_rreq_ranging_data_get_complete_t cb;
	uint16_t counter_in_progress;
	uint8_t next_expected_segment_counter;
	bool data_get_in_progress;
	bool last_segment_received;
};

struct bt_ras_rd_ready {
	struct bt_gatt_subscribe_params subscribe_params;
	bt_ras_rreq_rd_ready_cb_t cb;
};

struct bt_ras_rd_overwritten {
	struct bt_gatt_subscribe_params subscribe_params;
	bt_ras_rreq_rd_overwritten_cb_t cb;
};

struct bt_ras_features_read {
	struct bt_gatt_read_params read_params;
	bt_ras_rreq_features_read_cb_t cb;
};

static struct bt_ras_rreq {
	struct bt_conn *conn;
	struct bt_ras_rreq_cp cp;
	struct bt_ras_on_demand_rd on_demand_rd;
	struct bt_ras_rd_ready rd_ready;
	struct bt_ras_rd_overwritten rd_overwritten;
	struct bt_ras_features_read features_read;
} rreq_pool[CONFIG_BT_RAS_RREQ_MAX_ACTIVE_CONN];

static struct bt_ras_rreq *ras_rreq_find(struct bt_conn *conn)
{
	if (conn == NULL) {
		return NULL;
	}

	ARRAY_FOR_EACH_PTR(rreq_pool, rreq) {
		if (rreq->conn == conn) {
			return rreq;
		}
	}

	return NULL;
}

static int ras_rreq_alloc(struct bt_conn *conn)
{
	struct bt_ras_rreq *rreq = NULL;

	if (ras_rreq_find(conn) != NULL) {
		return -EALREADY;
	}

	ARRAY_FOR_EACH_PTR(rreq_pool, rreq_iter) {
		if (rreq_iter->conn == NULL) {
			rreq = rreq_iter;
			break;
		}
	}

	if (rreq == NULL) {
		return -ENOMEM;
	}

	LOG_DBG("conn %p new rreq %p", (void *)conn, (void *)rreq);

	memset(rreq, 0, sizeof(struct bt_ras_rreq));
	rreq->conn = bt_conn_ref(conn);

	return 0;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);

	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq) {
		return;
	}

	if (rreq->on_demand_rd.data_get_in_progress) {
		rreq->on_demand_rd.cb(conn, rreq->on_demand_rd.counter_in_progress, -ENOTCONN);
	}

	bt_ras_rreq_free(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
};

static uint8_t ranging_data_ready_notify_func(struct bt_conn *conn,
					      struct bt_gatt_subscribe_params *params,
					      const void *data, uint16_t length)
{
	if (data == NULL) {
		LOG_DBG("Ranging data ready unsubscribed");
		return BT_GATT_ITER_STOP;
	}

	if (length != sizeof(uint16_t)) {
		LOG_WRN("Received Ranging Data Ready Indication with invalid size");
		return BT_GATT_ITER_CONTINUE;
	}

	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (rreq == NULL) {
		LOG_WRN("Ranging data ready notification received without associated RREQ context, "
			"unsubscribing");
		return BT_GATT_ITER_STOP;
	}

	struct net_buf_simple rd_ready;

	net_buf_simple_init_with_data(&rd_ready, (uint8_t *)data, length);
	uint16_t ranging_counter = net_buf_simple_pull_le16(&rd_ready);

	if (rreq->rd_ready.cb) {
		rreq->rd_ready.cb(conn, ranging_counter);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void data_receive_finished(struct bt_ras_rreq *rreq)
{
	if (rreq->on_demand_rd.error_status == 0 && !rreq->on_demand_rd.last_segment_received) {
		LOG_WRN("Ranging data completed with missing segments");
		rreq->on_demand_rd.error_status = -ENODATA;
	}

	rreq->on_demand_rd.data_get_in_progress = false;

	rreq->on_demand_rd.cb(rreq->conn, rreq->on_demand_rd.counter_in_progress,
			      rreq->on_demand_rd.error_status);
}

static uint8_t ranging_data_overwritten_notify_func(struct bt_conn *conn,
						    struct bt_gatt_subscribe_params *params,
						    const void *data, uint16_t length)
{
	if (data == NULL) {
		LOG_DBG("Ranging data overwritten unsubscribed");
		return BT_GATT_ITER_STOP;
	}

	if (length != sizeof(uint16_t)) {
		LOG_WRN("Ranging Data Overwritten Indication size error");
		return BT_GATT_ITER_CONTINUE;
	}

	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (rreq == NULL) {
		LOG_WRN("Ranging data overwritten notification received without associated RREQ "
			"context, unsubscribing");
		return BT_GATT_ITER_STOP;
	}

	struct net_buf_simple rd_overwritten;

	net_buf_simple_init_with_data(&rd_overwritten, (uint8_t *)data, length);
	uint16_t ranging_counter = net_buf_simple_pull_le16(&rd_overwritten);

	if (rreq->on_demand_rd.data_get_in_progress &&
	    rreq->on_demand_rd.counter_in_progress == ranging_counter) {
		if (rreq->cp.state != BT_RAS_RREQ_CP_STATE_NONE) {
			LOG_DBG("Overwritten received while writing to RAS-CP, will continue "
				"waiting for RAS-CP response");
			return BT_GATT_ITER_CONTINUE;
		}

		LOG_DBG("Ranging counter %d overwritten whilst receiving", ranging_counter);
		rreq->on_demand_rd.error_status = -EACCES;
		data_receive_finished(rreq);

		return BT_GATT_ITER_CONTINUE;
	}

	if (rreq->rd_overwritten.cb) {
		rreq->rd_overwritten.cb(conn, ranging_counter);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void ack_ranging_data(struct bt_ras_rreq *rreq)
{
	NET_BUF_SIMPLE_DEFINE(ack_buf, RASCP_CMD_OPCODE_LEN + sizeof(uint16_t));
	net_buf_simple_add_u8(&ack_buf, RASCP_OPCODE_ACK_RD);
	net_buf_simple_add_le16(&ack_buf, rreq->on_demand_rd.counter_in_progress);

	int err = bt_gatt_write_without_response(rreq->conn, rreq->cp.subscribe_params.value_handle,
						 ack_buf.data, ack_buf.len, false);

	if (err) {
		LOG_WRN("ACK ranging data write failed, err %d. Ending data receive process "
			"here, peer will just have to wait for timeout.",
			err);
		data_receive_finished(rreq);
	}

	rreq->cp.state = BT_RAS_RREQ_CP_STATE_ACK_RD_WRITTEN;
	LOG_DBG("Ack Ranging data for counter %d", rreq->on_demand_rd.counter_in_progress);
}

static void handle_rsp_code(uint8_t rsp_code, struct bt_ras_rreq *rreq)
{
	switch (rreq->cp.state) {
	case BT_RAS_RREQ_CP_STATE_NONE: {
		if (rreq->on_demand_rd.data_get_in_progress &&
		    rsp_code == RASCP_RESPONSE_PROCEDURE_NOT_COMPLETED) {
			LOG_DBG("Ranging counter %d aborted whilst receiving",
				rreq->on_demand_rd.counter_in_progress);
			rreq->on_demand_rd.error_status = -EACCES;
			data_receive_finished(rreq);
			break;
		}

		LOG_WRN("Unexpected Response code received %d", rsp_code);
		break;
	}
	case BT_RAS_RREQ_CP_STATE_GET_RD_WRITTEN: {
		__ASSERT_NO_MSG(rreq->on_demand_rd.data_get_in_progress);
		rreq->cp.state = BT_RAS_RREQ_CP_STATE_NONE;

		if (rsp_code != RASCP_RESPONSE_SUCCESS) {
			LOG_DBG("Get Ranging Data returned an error %d", rsp_code);
			rreq->on_demand_rd.error_status = -ENOENT;
			data_receive_finished(rreq);
			break;
		}

		LOG_DBG("Get Ranging Data Success");

		break;
	}
	case BT_RAS_RREQ_CP_STATE_ACK_RD_WRITTEN: {
		__ASSERT_NO_MSG(rreq->on_demand_rd.data_get_in_progress);
		rreq->cp.state = BT_RAS_RREQ_CP_STATE_NONE;
		if (rsp_code != RASCP_RESPONSE_SUCCESS) {
			LOG_WRN("ACK Ranging Data returned an error %d. This should have no "
				"impact, so continue.",
				rsp_code);
		}

		data_receive_finished(rreq);
		break;
	}
	default:
		LOG_WRN("Unexpected Response code received %d", rsp_code);
		break;
	}
}

static uint8_t ras_cp_notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				  const void *data, uint16_t length)
{
	if (data == NULL) {
		LOG_DBG("RAS CP unsubscribed");
		return BT_GATT_ITER_STOP;
	}

	struct net_buf_simple rsp;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (rreq == NULL) {
		LOG_WRN("RAS CP notification received without associated RREQ context, "
			"unsubscribing");
		return BT_GATT_ITER_STOP;
	}

	net_buf_simple_init_with_data(&rsp, (uint8_t *)data, length);

	uint8_t opcode = net_buf_simple_pull_u8(&rsp);

	switch (opcode) {
	case RASCP_RSP_OPCODE_COMPLETE_RD_RSP: {
		if (rsp.len != RASCP_RSP_OPCODE_COMPLETE_RD_RSP_LEN) {
			LOG_WRN("Received complete ranging data response with invalid length: %d",
				length);
			break;
		}

		uint16_t ranging_counter = net_buf_simple_pull_le16(&rsp);

		if (!rreq->on_demand_rd.data_get_in_progress ||
		    rreq->on_demand_rd.counter_in_progress != ranging_counter) {
			LOG_WRN("Received complete ranging data response with unexpected ranging "
				"counter %d",
				ranging_counter);
			break;
		}

		ack_ranging_data(rreq);
		break;
	}
	case RASCP_RSP_OPCODE_RSP_CODE: {
		if (rsp.len != RASCP_RSP_OPCODE_RSP_CODE_LEN) {
			LOG_WRN("Received response opcode with invalid length: %d", length);
			break;
		}

		uint8_t rsp_code = net_buf_simple_pull_u8(&rsp);

		handle_rsp_code(rsp_code, rreq);

		break;
	}
	default: {
		LOG_WRN("Received unknown RAS-CP opcode: %d", opcode);
		break;
	}
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t ras_on_demand_ranging_data_notify_func(struct bt_conn *conn,
						      struct bt_gatt_subscribe_params *params,
						      const void *data, uint16_t length)
{
	LOG_DBG("On-demand Ranging Data notification received");

	if (data == NULL) {
		LOG_DBG("On demand ranging data unsubscribed");
		return BT_GATT_ITER_STOP;
	}

	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (rreq == NULL) {
		LOG_WRN("On-demand ranging data notification received without associated RREQ "
			"context, unsubscribing");
		return BT_GATT_ITER_STOP;
	}

	if (!rreq->on_demand_rd.data_get_in_progress) {
		LOG_WRN("Unexpected On-demand Ranging Data notification received");
		return BT_GATT_ITER_CONTINUE;
	}

	if (rreq->on_demand_rd.last_segment_received) {
		LOG_WRN("On-demand Ranging Data notification received after last segment");
		return BT_GATT_ITER_CONTINUE;
	}

	if (rreq->on_demand_rd.error_status) {
		/* Already had an error receiving this ranging counter, so exit here. */
		return BT_GATT_ITER_CONTINUE;
	}

	if (length < 2) {
		LOG_WRN("On-demand Ranging Data notification received invalid length");
		rreq->on_demand_rd.error_status = -EINVAL;
		return BT_GATT_ITER_CONTINUE;
	}

	struct net_buf_simple segment;

	net_buf_simple_init_with_data(&segment, (uint8_t *)data, length);

	uint8_t segmentation_header = net_buf_simple_pull_u8(&segment);

	bool first_segment = segmentation_header & BIT(0);
	bool last_segment = segmentation_header & BIT(1);
	uint8_t rolling_segment_counter = segmentation_header >> 2;

	if (first_segment && rolling_segment_counter != 0) {
		LOG_WRN("On-demand Ranging Data notification received invalid "
			"rolling_segment_counter %d",
			rolling_segment_counter);
		rreq->on_demand_rd.error_status = -EINVAL;
		return BT_GATT_ITER_CONTINUE;
	}

	if (rreq->on_demand_rd.next_expected_segment_counter != rolling_segment_counter) {
		LOG_WRN("No support for receiving segments out of order, expected counter %d, "
			"received counter %d",
			rreq->on_demand_rd.next_expected_segment_counter, rolling_segment_counter);
		rreq->on_demand_rd.error_status = -ENODATA;
		return BT_GATT_ITER_CONTINUE;
	}

	uint16_t ranging_data_segment_length = segment.len;

	if (net_buf_simple_tailroom(rreq->on_demand_rd.ranging_data_out) <
	    ranging_data_segment_length) {
		LOG_WRN("Ranging data out buffer not large enough for next segment");
		rreq->on_demand_rd.error_status = -ENOMEM;
		return BT_GATT_ITER_CONTINUE;
	}

	uint8_t *ranging_data_segment =
		net_buf_simple_pull_mem(&segment, ranging_data_segment_length);
	net_buf_simple_add_mem(rreq->on_demand_rd.ranging_data_out, ranging_data_segment,
			       ranging_data_segment_length);

	if (last_segment) {
		rreq->on_demand_rd.last_segment_received = true;
	}

	/* Segment counter is between 0-63. */
	rreq->on_demand_rd.next_expected_segment_counter =
		(rolling_segment_counter + 1) & BIT_MASK(6);

	return BT_GATT_ITER_CONTINUE;
}

static void subscribed_func(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_subscribe_params *params)
{
	if (err) {
		LOG_ERR("Subscribe to ccc_handle %d failed, err %d", params->ccc_handle, err);
	}
}

static int ras_cp_subscribe_params_populate(struct bt_gatt_dm *dm, struct bt_ras_rreq *rreq)
{
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	/* RAS-CP characteristic (mandatory) */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_RAS_CP);
	if (!gatt_chrc) {
		LOG_WRN("Could not locate mandatory RAS CP characteristic");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_RAS_CP);
	if (!gatt_desc) {
		LOG_WRN("Could not locate mandatory RAS CP descriptor");
		return -EINVAL;
	}
	rreq->cp.subscribe_params.value_handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_WRN("Could not locate mandatory RAS CP CCC");
		return -EINVAL;
	}
	rreq->cp.subscribe_params.ccc_handle = gatt_desc->handle;

	rreq->cp.subscribe_params.notify = ras_cp_notify_func;
	rreq->cp.subscribe_params.value = BT_GATT_CCC_INDICATE;
	rreq->cp.subscribe_params.subscribe = subscribed_func;

	return 0;
}

static uint8_t feature_read_cb(struct bt_conn *conn, uint8_t err,
			       struct bt_gatt_read_params *params, const void *data,
			       uint16_t length)
{
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (rreq == NULL) {
		return BT_GATT_ITER_STOP;
	}

	if (!rreq->features_read.cb) {
		LOG_ERR("No read callback present");
		return BT_GATT_ITER_STOP;
	}

	if (err) {
		LOG_ERR("Read value error: %d", err);
		rreq->features_read.cb(conn, 0, err);
		return BT_GATT_ITER_STOP;
	}

	if (!data || length != sizeof(uint32_t)) {
		rreq->features_read.cb(conn, 0, -EMSGSIZE);
		return BT_GATT_ITER_STOP;
	}

	struct net_buf_simple feature_buf;

	net_buf_simple_init_with_data(&feature_buf, (uint8_t *)data, length);
	uint16_t feature_bits = net_buf_simple_pull_le32(&feature_buf);

	rreq->features_read.cb(conn, feature_bits, err);

	rreq->features_read.cb = NULL;

	return BT_GATT_ITER_STOP;
}

static int ras_read_features_params_populate(struct bt_gatt_dm *dm, struct bt_ras_rreq *rreq)
{
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	/* RAS Features (Mandatory) */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_RAS_FEATURES);
	if (!gatt_chrc) {
		LOG_WRN("Could not locate mandatory RAS Features characteristic");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_RAS_FEATURES);
	if (!gatt_desc) {
		LOG_WRN("Could not locate mandatory RAS Features descriptor");
		return -EINVAL;
	}
	rreq->features_read.read_params.single.handle = gatt_desc->handle;
	rreq->features_read.read_params.func = feature_read_cb;
	rreq->features_read.read_params.handle_count = 1;
	rreq->features_read.read_params.single.offset = 0;

	return 0;
}

int bt_ras_rreq_cp_subscribe(struct bt_conn *conn)
{
	int err;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq) {
		return -EINVAL;
	}

	err = bt_gatt_subscribe(conn, &rreq->cp.subscribe_params);
	if (err && err != -EALREADY) {
		LOG_DBG("RAS-CP subscribe failed (err %d)", err);
		return err;
	}

	return 0;
}

int bt_ras_rreq_cp_unsubscribe(struct bt_conn *conn)
{
	int err;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq) {
		return -EINVAL;
	}

	err = bt_gatt_unsubscribe(conn, &rreq->cp.subscribe_params);
	if (err && err != -EINVAL) {
		LOG_DBG("RAS-CP unsubscribe failed (err %d)", err);
		return err;
	}

	return 0;
}

static int ondemand_rd_subscribe_params_populate(struct bt_gatt_dm *dm, struct bt_ras_rreq *rreq)
{
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	/* RAS On-demand ranging data characteristic (mandatory) */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_RAS_ONDEMAND_RD);
	if (!gatt_chrc) {
		LOG_WRN("Could not locate mandatory On-demand ranging data characteristic");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_RAS_ONDEMAND_RD);
	if (!gatt_desc) {
		LOG_WRN("Could not locate mandatory On-demand ranging data descriptor");
		return -EINVAL;
	}
	rreq->on_demand_rd.subscribe_params.value_handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_WRN("Could not locate mandatory On-demand ranging data CCC");
		return -EINVAL;
	}
	rreq->on_demand_rd.subscribe_params.ccc_handle = gatt_desc->handle;
	rreq->on_demand_rd.subscribe_params.notify = ras_on_demand_ranging_data_notify_func;
	rreq->on_demand_rd.subscribe_params.value = BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE;
	rreq->on_demand_rd.subscribe_params.subscribe = subscribed_func;

	return 0;
}

static int rd_ready_subscribe_params_populate(struct bt_gatt_dm *dm, struct bt_ras_rreq *rreq)
{
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	/* RAS Ranging Data Ready characteristic (mandatory) */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_RAS_RD_READY);
	if (!gatt_chrc) {
		LOG_WRN("Could not locate mandatory ranging data ready characteristic");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_RAS_RD_READY);
	if (!gatt_desc) {
		LOG_WRN("Could not locate mandatory ranging data ready descriptor");
		return -EINVAL;
	}
	rreq->rd_ready.subscribe_params.value_handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_WRN("Could not locate mandatory ranging data ready CCC");
		return -EINVAL;
	}
	rreq->rd_ready.subscribe_params.ccc_handle = gatt_desc->handle;
	rreq->rd_ready.subscribe_params.notify = ranging_data_ready_notify_func;
	rreq->rd_ready.subscribe_params.value = BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE;
	rreq->rd_ready.subscribe_params.subscribe = subscribed_func;

	return 0;
}

static int rd_overwritten_subscribe_params_populate(struct bt_gatt_dm *dm, struct bt_ras_rreq *rreq)
{
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	/* RAS Ranging Data Overwritten characteristic (mandatory) */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_RAS_RD_OVERWRITTEN);
	if (!gatt_chrc) {
		LOG_WRN("Could not locate mandatory ranging data overwritten characteristic");
		return -EINVAL;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_RAS_RD_OVERWRITTEN);
	if (!gatt_desc) {
		LOG_WRN("Could not locate mandatory ranging data overwritten descriptor");
		return -EINVAL;
	}
	rreq->rd_overwritten.subscribe_params.value_handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc) {
		LOG_WRN("Could not locate mandatory ranging data overwritten CCC");
		return -EINVAL;
	}
	rreq->rd_overwritten.subscribe_params.ccc_handle = gatt_desc->handle;
	rreq->rd_overwritten.subscribe_params.notify = ranging_data_overwritten_notify_func;
	rreq->rd_overwritten.subscribe_params.value = BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE;
	rreq->rd_overwritten.subscribe_params.subscribe = subscribed_func;

	return 0;
}

int bt_ras_rreq_on_demand_rd_subscribe(struct bt_conn *conn)
{
	int err;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq) {
		return -EINVAL;
	}

	err = bt_gatt_subscribe(conn, &rreq->on_demand_rd.subscribe_params);
	if (err && err != -EALREADY) {
		LOG_DBG("On-demand ranging data subscribe failed (err %d)", err);
		return err;
	}

	LOG_DBG("On-demand ranging data subscribed");

	return 0;
}

int bt_ras_rreq_on_demand_rd_unsubscribe(struct bt_conn *conn)
{
	int err;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq) {
		return -EINVAL;
	}

	err = bt_gatt_unsubscribe(conn, &rreq->on_demand_rd.subscribe_params);
	if (err && err != -EINVAL) {
		LOG_DBG("On-demand ranging data unsubscribe failed (err %d)", err);
		return err;
	}

	LOG_DBG("On-demand ranging data unsubscribed");

	return 0;
}

int bt_ras_rreq_rd_ready_subscribe(struct bt_conn *conn, bt_ras_rreq_rd_ready_cb_t cb)
{
	int err;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq || !cb) {
		return -EINVAL;
	}

	err = bt_gatt_subscribe(conn, &rreq->rd_ready.subscribe_params);
	if (err && err != -EALREADY) {
		LOG_DBG("Ranging data ready subscribe failed (err %d)", err);
		return err;
	}

	LOG_DBG("Ranging data ready subscribed");
	rreq->rd_ready.cb = cb;

	return 0;
}

int bt_ras_rreq_rd_ready_unsubscribe(struct bt_conn *conn)
{
	int err;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq) {
		return -EINVAL;
	}

	err = bt_gatt_unsubscribe(conn, &rreq->rd_ready.subscribe_params);
	if (err && err != -EINVAL) {
		LOG_DBG("Ranging data ready unsubscribe failed (err %d)", err);
		return err;
	}

	LOG_DBG("Ranging data ready unsubscribed");
	rreq->rd_ready.cb = NULL;

	return 0;
}

int bt_ras_rreq_rd_overwritten_subscribe(struct bt_conn *conn, bt_ras_rreq_rd_overwritten_cb_t cb)
{
	int err;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq || !cb) {
		return -EINVAL;
	}

	err = bt_gatt_subscribe(conn, &rreq->rd_overwritten.subscribe_params);
	if (err && err != -EALREADY) {
		LOG_DBG("Ranging data overwritten subscribe failed (err %d)", err);
		return err;
	}

	LOG_DBG("Ranging data overwritten subscribed");
	rreq->rd_overwritten.cb = cb;

	return 0;
}

int bt_ras_rreq_rd_overwritten_unsubscribe(struct bt_conn *conn)
{
	int err;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq) {
		return -EINVAL;
	}

	err = bt_gatt_unsubscribe(conn, &rreq->rd_overwritten.subscribe_params);
	if (err && err != -EINVAL) {
		LOG_DBG("Ranging data overwritten unsubscribe failed (err %d)", err);
		return err;
	}

	LOG_DBG("Ranging data overwritten unsubscribed");
	rreq->rd_overwritten.cb = NULL;

	return 0;
}

void bt_ras_rreq_free(struct bt_conn *conn)
{
	int err = 0;
	struct bt_conn_info info;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (!rreq) {
		return;
	}

	LOG_DBG("Free rreq %p for conn %p", (void *)conn, (void *)rreq);

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		bt_conn_unref(rreq->conn);
		rreq->conn = NULL;
		return;
	}

	if (info.state == BT_CONN_STATE_CONNECTED) {
		/* -EINVAL means already not subscribed */
		err = bt_gatt_unsubscribe(conn, &rreq->cp.subscribe_params);
		if (err != 0 && err != -EINVAL) {
			LOG_WRN("Failed to unsubscribe to RAS-CP: %d", err);
		}

		err = bt_gatt_unsubscribe(conn, &rreq->on_demand_rd.subscribe_params);
		if (err != 0 && err != -EINVAL) {
			LOG_WRN("Failed to unsubscribe to ondemand ranging data: %d", err);
		}

		err = bt_gatt_unsubscribe(conn, &rreq->rd_ready.subscribe_params);
		if (err != 0 && err != -EINVAL) {
			LOG_WRN("Failed to unsubscribe to ranging data ready: %d", err);
		}

		err = bt_gatt_unsubscribe(conn, &rreq->rd_overwritten.subscribe_params);
		if (err != 0 && err != -EINVAL) {
			LOG_WRN("Failed to unsubscribe to ranging data overwritten: %d", err);
		}
	}

	bt_conn_unref(rreq->conn);
	rreq->conn = NULL;
}

int bt_ras_rreq_alloc_and_assign_handles(struct bt_gatt_dm *dm, struct bt_conn *conn)
{
	int err;
	struct bt_ras_rreq *rreq = NULL;

	if (dm == NULL || conn == NULL) {
		return -EINVAL;
	}

	err = ras_rreq_alloc(conn);
	if (err) {
		return err;
	}

	rreq = ras_rreq_find(conn);
	__ASSERT_NO_MSG(rreq);

	err = ondemand_rd_subscribe_params_populate(dm, rreq);
	if (err) {
		bt_ras_rreq_free(conn);
		return err;
	}

	err = rd_ready_subscribe_params_populate(dm, rreq);
	if (err) {
		bt_ras_rreq_free(conn);
		return err;
	}

	err = rd_overwritten_subscribe_params_populate(dm, rreq);
	if (err) {
		bt_ras_rreq_free(conn);
		return err;
	}

	err = ras_cp_subscribe_params_populate(dm, rreq);
	if (err) {
		bt_ras_rreq_free(conn);
		return err;
	}

	err = ras_read_features_params_populate(dm, rreq);
	if (err) {
		bt_ras_rreq_free(conn);
		return err;
	}

	return 0;
}

int bt_ras_rreq_read_features(struct bt_conn *conn, bt_ras_rreq_features_read_cb_t cb)
{
	int err;

	if (cb == NULL || conn == NULL) {
		return -EINVAL;
	}

	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (rreq == NULL) {
		return -EINVAL;
	}

	if (rreq->features_read.cb) {
		return -EBUSY;
	}

	rreq->features_read.cb = cb;

	err = bt_gatt_read(conn, &rreq->features_read.read_params);
	if (err) {
		rreq->features_read.cb = NULL;
		return err;
	}

	return 0;
}

int bt_ras_rreq_cp_get_ranging_data(struct bt_conn *conn, struct net_buf_simple *ranging_data_out,
				    uint16_t ranging_counter,
				    bt_ras_rreq_ranging_data_get_complete_t cb)
{
	int err;
	struct bt_ras_rreq *rreq = ras_rreq_find(conn);

	if (rreq == NULL || ranging_data_out == NULL || cb == NULL) {
		return -EINVAL;
	}

	if (rreq->cp.state != BT_RAS_RREQ_CP_STATE_NONE ||
	    rreq->on_demand_rd.data_get_in_progress) {
		return -EBUSY;
	}

	rreq->on_demand_rd.data_get_in_progress = true;
	rreq->on_demand_rd.ranging_data_out = ranging_data_out;
	rreq->on_demand_rd.counter_in_progress = ranging_counter;
	rreq->on_demand_rd.cb = cb;
	rreq->on_demand_rd.next_expected_segment_counter = 0;
	rreq->on_demand_rd.last_segment_received = false;
	rreq->on_demand_rd.error_status = 0;

	NET_BUF_SIMPLE_DEFINE(get_ranging_data, RASCP_CMD_OPCODE_LEN + sizeof(uint16_t));
	net_buf_simple_add_u8(&get_ranging_data, RASCP_OPCODE_GET_RD);
	net_buf_simple_add_le16(&get_ranging_data, rreq->on_demand_rd.counter_in_progress);

	err = bt_gatt_write_without_response(conn, rreq->cp.subscribe_params.value_handle,
					     get_ranging_data.data, get_ranging_data.len, false);
	if (err) {
		LOG_DBG("CP Get ranging data written failed, err %d", err);
		return err;
	}

	rreq->cp.state = BT_RAS_RREQ_CP_STATE_GET_RD_WRITTEN;

	return 0;
}

void bt_ras_rreq_rd_subevent_data_parse(struct net_buf_simple *peer_ranging_data_buf,
					struct net_buf_simple *local_step_data_buf,
					enum bt_conn_le_cs_role cs_role,
					bt_ras_rreq_ranging_header_cb_t ranging_header_cb,
					bt_ras_rreq_subevent_header_cb_t subevent_header_cb,
					bt_ras_rreq_step_data_cb_t step_data_cb, void *user_data)
{
	if (!peer_ranging_data_buf
		|| !local_step_data_buf
		|| peer_ranging_data_buf->len == 0
		|| local_step_data_buf->len == 0) {
		LOG_ERR("Tried to parse empty step data.");
		return;
	}

	/* Remove ranging data header. */
	struct ras_ranging_header *peer_ranging_header =
		net_buf_simple_pull_mem(peer_ranging_data_buf, sizeof(struct ras_ranging_header));

	if (ranging_header_cb && !ranging_header_cb(peer_ranging_header, user_data)) {
		return;
	}

	while (peer_ranging_data_buf->len >= sizeof(struct ras_subevent_header)) {
		struct ras_subevent_header *peer_subevent_header_data = net_buf_simple_pull_mem(
			peer_ranging_data_buf, sizeof(struct ras_subevent_header));

		if (subevent_header_cb &&
		    !subevent_header_cb(peer_subevent_header_data, user_data)) {
			return;
		}

		if (peer_subevent_header_data->num_steps_reported == 0) {
			LOG_DBG("Skipping subevent with no steps.");
			continue;
		}

		if (peer_ranging_data_buf->len == 0) {
			LOG_WRN("Empty peer step data buf where steps were expected.");
			return;
		}

		for (uint8_t i = 0; i < peer_subevent_header_data->num_steps_reported; i++) {
			struct bt_le_cs_subevent_step local_step;
			struct bt_le_cs_subevent_step peer_step;

			if (local_step_data_buf->len < 3) {
				LOG_WRN("Local step data appears malformed.");
				return;
			}

			if (peer_ranging_data_buf->len < 1) {
				LOG_WRN("Peer step data appears malformed.");
				return;
			}

			local_step.mode = net_buf_simple_pull_u8(local_step_data_buf);
			local_step.channel = net_buf_simple_pull_u8(local_step_data_buf);
			local_step.data_len = net_buf_simple_pull_u8(local_step_data_buf);

			peer_step.mode = net_buf_simple_pull_u8(peer_ranging_data_buf);
			peer_step.channel = local_step.channel;

			if (peer_step.mode != local_step.mode) {
				LOG_WRN("Mismatch of local and peer step mode %d != %d",
					peer_step.mode, local_step.mode);
				return;
			}

			if (local_step.data_len == 0) {
				LOG_WRN("Encountered zero-length step data.");
				return;
			}

			peer_step.data = peer_ranging_data_buf->data;
			local_step.data = local_step_data_buf->data;

			peer_step.data_len = local_step.data_len;

			if (peer_step.mode & BIT(7)) {
				/* From RAS spec:
				 * Bit 7: 1 means Aborted, 0 means Success
				 * If the Step is aborted and bit 7 is set to 1, then bits 0-6 do
				 * not contain any valid data
				 */
				LOG_INF("Peer step aborted");
				return;
			}

			if (peer_step.mode == 0) {
				/* Only occasion where peer step mode length is not equal to local
				 * step mode length is mode 0 steps.
				 */
				peer_step.data_len =
					(cs_role == BT_CONN_LE_CS_ROLE_INITIATOR)
						? sizeof(struct
							 bt_hci_le_cs_step_data_mode_0_reflector)
						: sizeof(struct
							 bt_hci_le_cs_step_data_mode_0_initiator);
			}

			if (local_step.data_len > local_step_data_buf->len) {
				LOG_WRN("Local step data appears malformed.");
				return;
			}

			if (peer_step.data_len > peer_ranging_data_buf->len) {
				LOG_WRN("Peer step data appears malformed.");
				return;
			}

			if (step_data_cb && !step_data_cb(&local_step, &peer_step, user_data)) {
				return;
			}

			net_buf_simple_pull(peer_ranging_data_buf, peer_step.data_len);
			net_buf_simple_pull(local_step_data_buf, local_step.data_len);
		}
	}

	if (local_step_data_buf->len != 0 || peer_ranging_data_buf->len != 0) {
		LOG_WRN("Peer or local buffers not fully drained at the end of parsing.");
	}
}
