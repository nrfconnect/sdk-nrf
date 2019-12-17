/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <bluetooth/mesh/gen_onoff_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "model_utils.h"

static void encode_status(struct net_buf_simple *buf,
			  const struct bt_mesh_onoff_status *status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_ONOFF_OP_STATUS);
	net_buf_simple_add_u8(buf, !!status->present_on_off);

	if (status->remaining_time != 0) {
		net_buf_simple_add_u8(buf, status->target_on_off);
		net_buf_simple_add_u8(
			buf, model_transition_encode(status->remaining_time));
	}
}

static void rsp_status(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *rx_ctx,
		       const struct bt_mesh_onoff_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_STATUS,
				 BT_MESH_ONOFF_MSG_MAXLEN_STATUS);
	encode_status(&msg, status);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void handle_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_ONOFF_MSG_LEN_GET) {
		return;
	}

	struct bt_mesh_onoff_srv *srv = model->user_data;
	struct bt_mesh_onoff_status status = { 0 };

	srv->handlers->get(srv, ctx, &status);

	rsp_status(model, ctx, &status);
}

static void onoff_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_ONOFF_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_ONOFF_MSG_MAXLEN_SET) {
		return;
	}

	struct bt_mesh_onoff_srv *srv = model->user_data;
	struct bt_mesh_onoff_status status = { 0 };
	struct bt_mesh_model_transition transition;
	struct bt_mesh_onoff_set set;

	u8_t on_off = net_buf_simple_pull_u8(buf);
	u8_t tid = net_buf_simple_pull_u8(buf);

	if (on_off > 1) {
		return;
	}

	set.on_off = on_off;

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* If this is the same transaction, we don't need to send it
		 * to the app, but we still have to respond with a status.
		 */
		srv->handlers->get(srv, NULL, &status);
		goto respond;
	}

	if (buf->len == 2) {
		model_transition_buf_pull(buf, &transition);
	} else {
		bt_mesh_dtt_srv_transition_get(srv->model, &transition);
	}

	set.transition = &transition;

	srv->handlers->set(srv, ctx, &set, &status);

	(void)bt_mesh_onoff_srv_pub(srv, NULL, &status);

respond:
	if (ack) {
		rsp_status(model, ctx, &status);
	}
}

static void handle_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	onoff_set(model, ctx, buf, true);
}

static void handle_set_unack(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	onoff_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_onoff_srv_op[] = {
	{ BT_MESH_ONOFF_OP_GET, BT_MESH_ONOFF_MSG_LEN_GET, handle_get },
	{ BT_MESH_ONOFF_OP_SET, BT_MESH_ONOFF_MSG_MINLEN_SET, handle_set },
	{ BT_MESH_ONOFF_OP_SET_UNACK, BT_MESH_ONOFF_MSG_MINLEN_SET,
	  handle_set_unack },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_onoff_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_onoff_srv *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(model->pub->msg, 0);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_onoff_srv_cb = {
	.init = bt_mesh_onoff_srv_init
};

int _bt_mesh_onoff_srv_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_onoff_srv *srv = model->user_data;
	struct bt_mesh_onoff_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);
	encode_status(model->pub->msg, &status);

	return 0;
}

s32_t bt_mesh_onoff_srv_pub(struct bt_mesh_onoff_srv *srv,
			    struct bt_mesh_msg_ctx *ctx,
			    const struct bt_mesh_onoff_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_STATUS,
				 BT_MESH_ONOFF_MSG_MAXLEN_STATUS);
	encode_status(&msg, status);
	return model_send(srv->model, ctx, &msg);
}
