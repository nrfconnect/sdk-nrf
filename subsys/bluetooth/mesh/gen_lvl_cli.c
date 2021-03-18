/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/gen_lvl_cli.h>
#include "model_utils.h"

static void handle_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LVL_MSG_MINLEN_STATUS &&
	    buf->len != BT_MESH_LVL_MSG_MAXLEN_STATUS) {
		return;
	}

	struct bt_mesh_lvl_cli *cli = model->user_data;
	struct bt_mesh_lvl_status status;

	status.current = net_buf_simple_pull_le16(buf);
	if (buf->len == 3) {
		status.target = net_buf_simple_pull_le16(buf);
		status.remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else {
		status.target = status.current;
		status.remaining_time = 0;
	}

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LVL_OP_STATUS, ctx)) {
		struct bt_mesh_lvl_status *rsp =
			(struct bt_mesh_lvl_status *)cli->ack_ctx.user_data;

		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, &status);
	}
}

const struct bt_mesh_model_op _bt_mesh_lvl_cli_op[] = {
	{ BT_MESH_LVL_OP_STATUS, BT_MESH_LVL_MSG_MINLEN_STATUS, handle_status },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_lvl_init(struct bt_mesh_model *model)
{
	struct bt_mesh_lvl_cli *cli = model->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));

	model_ack_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_lvl_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_lvl_cli *cli = model->user_data;

	net_buf_simple_reset(model->pub->msg);
	model_ack_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_lvl_cli_cb = {
	.init = bt_mesh_lvl_init,
	.reset = bt_mesh_lvl_reset,
};

int bt_mesh_lvl_cli_get(struct bt_mesh_lvl_cli *cli,
			struct bt_mesh_msg_ctx *ctx,
			struct bt_mesh_lvl_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LVL_OP_GET,
				 BT_MESH_LVL_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LVL_OP_GET);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LVL_OP_STATUS, rsp);
}

int bt_mesh_lvl_cli_set(struct bt_mesh_lvl_cli *cli,
			struct bt_mesh_msg_ctx *ctx,
			const struct bt_mesh_lvl_set *set,
			struct bt_mesh_lvl_status *rsp)
{
	if (set->new_transaction) {
		cli->tid++;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LVL_OP_SET,
				 BT_MESH_LVL_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LVL_OP_SET);

	net_buf_simple_add_le16(&msg, set->lvl);
	net_buf_simple_add_u8(&msg, cli->tid);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LVL_OP_STATUS, rsp);
}

int bt_mesh_lvl_cli_set_unack(struct bt_mesh_lvl_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_lvl_set *set)
{
	if (set->new_transaction) {
		cli->tid++;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LVL_OP_SET_UNACK,
				 BT_MESH_LVL_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LVL_OP_SET_UNACK);

	net_buf_simple_add_le16(&msg, set->lvl);
	net_buf_simple_add_u8(&msg, cli->tid);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_send(cli->model, ctx, &msg);
}

int bt_mesh_lvl_cli_delta_set(struct bt_mesh_lvl_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_lvl_delta_set *delta_set,
			      struct bt_mesh_lvl_status *rsp)
{
	if (delta_set->new_transaction) {
		cli->tid++;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LVL_OP_DELTA_SET,
				 BT_MESH_LVL_MSG_MAXLEN_DELTA_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LVL_OP_DELTA_SET);

	net_buf_simple_add_le32(&msg, delta_set->delta);
	net_buf_simple_add_u8(&msg, cli->tid);
	if (delta_set->transition) {
		model_transition_buf_add(&msg, delta_set->transition);
	}

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LVL_OP_STATUS, rsp);
}

int bt_mesh_lvl_cli_delta_set_unack(
	struct bt_mesh_lvl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_lvl_delta_set *delta_set)
{
	if (delta_set->new_transaction) {
		cli->tid++;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LVL_OP_DELTA_SET_UNACK,
				 BT_MESH_LVL_MSG_MAXLEN_DELTA_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LVL_OP_DELTA_SET_UNACK);

	net_buf_simple_add_le32(&msg, delta_set->delta);
	net_buf_simple_add_u8(&msg, cli->tid);
	if (delta_set->transition) {
		model_transition_buf_add(&msg, delta_set->transition);
	}

	return model_send(cli->model, ctx, &msg);
}

int bt_mesh_lvl_cli_move_set(struct bt_mesh_lvl_cli *cli,
			     struct bt_mesh_msg_ctx *ctx,
			     const struct bt_mesh_lvl_move_set *move_set,
			     struct bt_mesh_lvl_status *rsp)
{
	if (move_set->new_transaction) {
		cli->tid++;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LVL_OP_MOVE_SET,
				 BT_MESH_LVL_MSG_MAXLEN_MOVE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LVL_OP_MOVE_SET);

	net_buf_simple_add_le16(&msg, move_set->delta);
	net_buf_simple_add_u8(&msg, cli->tid);

	if (move_set->transition) {
		model_transition_buf_add(&msg, move_set->transition);
	}

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LVL_OP_STATUS, rsp);
}

int bt_mesh_lvl_cli_move_set_unack(struct bt_mesh_lvl_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_lvl_move_set *move_set)
{
	if (move_set->new_transaction) {
		cli->tid++;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LVL_OP_MOVE_SET_UNACK,
				 BT_MESH_LVL_MSG_MAXLEN_MOVE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LVL_OP_MOVE_SET_UNACK);

	net_buf_simple_add_le16(&msg, move_set->delta);
	net_buf_simple_add_u8(&msg, cli->tid);

	if (move_set->transition) {
		model_transition_buf_add(&msg, move_set->transition);
	}

	return model_send(cli->model, ctx, &msg);
}
