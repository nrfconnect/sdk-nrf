/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <bluetooth/mesh/gen_lvl_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "model_utils.h"

static void encode_status(const struct bt_mesh_lvl_status *status,
			  struct net_buf_simple *buf)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LVL_OP_STATUS);
	net_buf_simple_add_le16(buf, status->current);

	if (status->remaining_time == 0) {
		return;
	}

	net_buf_simple_add_le16(buf, status->target);
	net_buf_simple_add_u8(buf,
			      model_transition_encode(status->remaining_time));
}

static void transition_get(struct bt_mesh_lvl_srv *srv,
			   struct bt_mesh_model_transition *transition,
			   struct net_buf_simple *buf)
{
	if (buf->len == 2) {
		model_transition_buf_pull(buf, transition);
	} else {
		bt_mesh_dtt_srv_transition_get(srv->model, transition);
	}
}

static void rsp_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       const struct bt_mesh_lvl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LVL_OP_STATUS,
				 BT_MESH_LVL_MSG_MAXLEN_STATUS);
	encode_status(status, &rsp);

	bt_mesh_model_send(model, ctx, &rsp, NULL, 0);
}

static void handle_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LVL_MSG_LEN_GET) {
		return;
	}

	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };

	srv->handlers->get(srv, ctx, &status);

	rsp_status(model, ctx, &status);
}

static void set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LVL_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LVL_MSG_MAXLEN_SET) {
		return;
	}

	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };
	struct bt_mesh_model_transition transition;
	struct bt_mesh_lvl_set set;

	set.lvl = net_buf_simple_pull_le16(buf);
	set.new_transaction = !tid_check_and_update(
		&srv->tid, net_buf_simple_pull_u8(buf), ctx);
	transition_get(srv, &transition, buf);
	set.transition = &transition;

	srv->handlers->set(srv, ctx, &set, &status);

	if (ack) {
		rsp_status(model, ctx, &status);
	}

	(void)bt_mesh_lvl_srv_pub(srv, NULL, &status);
}

static void delta_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LVL_MSG_MINLEN_DELTA_SET &&
	    buf->len != BT_MESH_LVL_MSG_MAXLEN_DELTA_SET) {
		return;
	}

	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };
	struct bt_mesh_lvl_delta_set delta_set;
	struct bt_mesh_model_transition transition;

	delta_set.delta = net_buf_simple_pull_le32(buf);
	delta_set.new_transaction = !tid_check_and_update(
		&srv->tid, net_buf_simple_pull_u8(buf), ctx);
	transition_get(srv, &transition, buf);
	delta_set.transition = &transition;

	srv->handlers->delta_set(srv, ctx, &delta_set, &status);

	if (ack) {
		rsp_status(model, ctx, &status);
	}

	(void)bt_mesh_lvl_srv_pub(srv, NULL, &status);
}

static void move_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LVL_MSG_MINLEN_MOVE_SET &&
	    buf->len != BT_MESH_LVL_MSG_MAXLEN_MOVE_SET) {
		return;
	}

	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };
	struct bt_mesh_model_transition transition;
	struct bt_mesh_lvl_move_set move_set;

	move_set.delta = net_buf_simple_pull_le16(buf);
	move_set.new_transaction = !tid_check_and_update(
		&srv->tid, net_buf_simple_pull_u8(buf), ctx);
	transition_get(srv, &transition, buf);
	move_set.transition = &transition;

	/* If transition.time is 0, we shouldn't move. Align these two
	 * parameters to simplify application logic for this case:
	 */
	if (move_set.transition->time == 0 || move_set.delta == 0) {
		move_set.delta = 0;
		transition.time = 0;
	}

	srv->handlers->move_set(srv, ctx, &move_set, &status);

	if (ack) {
		rsp_status(model, ctx, &status);
	}

	(void)bt_mesh_lvl_srv_pub(srv, ctx, &status);
}

/* Message handlers */

static void handle_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	set(model, ctx, buf, true);
}

static void handle_set_unack(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	set(model, ctx, buf, false);
}

static void handle_delta_set(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	delta_set(model, ctx, buf, true);
}

static void handle_delta_set_unack(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	delta_set(model, ctx, buf, false);
}

static void handle_move_set(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	move_set(model, ctx, buf, true);
}

static void handle_move_set_unack(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	move_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_lvl_srv_op[] = {
	{ BT_MESH_LVL_OP_GET, BT_MESH_LVL_MSG_LEN_GET, handle_get },
	{ BT_MESH_LVL_OP_SET, BT_MESH_LVL_MSG_MINLEN_SET, handle_set },
	{ BT_MESH_LVL_OP_SET_UNACK, BT_MESH_LVL_MSG_MINLEN_SET,
	  handle_set_unack },
	{ BT_MESH_LVL_OP_DELTA_SET, BT_MESH_LVL_MSG_MINLEN_DELTA_SET,
	  handle_delta_set },
	{ BT_MESH_LVL_OP_DELTA_SET_UNACK, BT_MESH_LVL_MSG_MINLEN_DELTA_SET,
	  handle_delta_set_unack },
	{ BT_MESH_LVL_OP_MOVE_SET, BT_MESH_LVL_MSG_MINLEN_MOVE_SET,
	  handle_move_set },
	{ BT_MESH_LVL_OP_MOVE_SET_UNACK, BT_MESH_LVL_MSG_MINLEN_MOVE_SET,
	  handle_move_set_unack },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_lvl_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_lvl_srv *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(model->pub->msg, 0);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_lvl_srv_cb = {
	.init = bt_mesh_lvl_srv_init,
};

int _bt_mesh_lvl_srv_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);

	encode_status(&status, model->pub->msg);
	return 0;
}

int bt_mesh_lvl_srv_pub(struct bt_mesh_lvl_srv *srv,
			struct bt_mesh_msg_ctx *ctx,
			const struct bt_mesh_lvl_status *status)
{
	if (!srv->pub.addr) {
		return 0;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LVL_OP_STATUS,
				 BT_MESH_LVL_MSG_MAXLEN_STATUS);
	encode_status(status, &msg);

	return model_send(srv->model, ctx, &msg);
}
