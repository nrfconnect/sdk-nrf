/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <bluetooth/mesh/gen_onoff_cli.h>
#include "model_utils.h"

static void decode_status(struct net_buf_simple *buf,
			  struct bt_mesh_onoff_status *status)
{
	status->present_on_off = net_buf_simple_pull_u8(buf);

	if (buf->len == 2) {
		status->target_on_off = net_buf_simple_pull_u8(buf);
		status->remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else {
		status->target_on_off = status->present_on_off;
		status->remaining_time = 0;
	}
}

static void handle_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_ONOFF_MSG_MINLEN_STATUS &&
	    buf->len != BT_MESH_ONOFF_MSG_MAXLEN_STATUS) {
		return;
	}

	struct bt_mesh_onoff_cli *cli = model->user_data;
	struct bt_mesh_onoff_status status;

	decode_status(buf, &status);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_ONOFF_OP_STATUS, ctx)) {
		struct bt_mesh_onoff_status *rsp =
			(struct bt_mesh_onoff_status *)cli->ack_ctx.user_data;

		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, &status);
	}
}

const struct bt_mesh_model_op _bt_mesh_onoff_cli_op[] = {
	{ BT_MESH_ONOFF_OP_STATUS, BT_MESH_ONOFF_MSG_MINLEN_STATUS,
	  handle_status },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_onoff_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_onoff_cli *cli = model->user_data;

	cli->model = model;
	net_buf_simple_init(cli->pub.msg, 0);
	model_ack_init(&cli->ack_ctx);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_onoff_cli_cb = {
	.init = bt_mesh_onoff_cli_init,
};

int bt_mesh_onoff_cli_status_get(struct bt_mesh_onoff_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_onoff_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_GET,
				 BT_MESH_ONOFF_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_ONOFF_OP_GET);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_ONOFF_OP_STATUS, rsp);
}

int bt_mesh_onoff_cli_set(struct bt_mesh_onoff_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_onoff_set *set,
			  struct bt_mesh_onoff_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_SET,
				 BT_MESH_ONOFF_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_ONOFF_OP_SET);

	net_buf_simple_add_u8(&msg, set->on_off);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_ONOFF_OP_STATUS, rsp);
}

int bt_mesh_onoff_cli_set_unack(struct bt_mesh_onoff_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_onoff_set *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_SET_UNACK,
				 BT_MESH_ONOFF_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_ONOFF_OP_SET_UNACK);

	net_buf_simple_add_u8(&msg, set->on_off);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_send(cli->model, ctx, &msg);
}
