/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/mesh/gen_ponoff_cli.h>
#include "model_utils.h"

static void handle_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_PONOFF_MSG_LEN_STATUS) {
		return;
	}

	struct bt_mesh_ponoff_cli *cli = model->user_data;
	enum bt_mesh_on_power_up on_power_up = net_buf_simple_pull_u8(buf);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_PONOFF_OP_STATUS, ctx)) {
		enum bt_mesh_on_power_up *rsp =
			(enum bt_mesh_on_power_up *)cli->ack_ctx.user_data;
		*rsp = on_power_up;

		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, on_power_up);
	}
}

const struct bt_mesh_model_op _bt_mesh_ponoff_cli_op[] = {
	{ BT_MESH_PONOFF_OP_STATUS, BT_MESH_PONOFF_MSG_LEN_STATUS,
	  handle_status },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_ponoff_cli_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_ponoff_cli *cli = mod->user_data;

	cli->model = mod;
	net_buf_simple_init(mod->pub->msg, 0);
	model_ack_init(&cli->ack_ctx);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_ponoff_cli_cb = {
	.init = bt_mesh_ponoff_cli_init,
};

int bt_mesh_ponoff_cli_on_power_up_get(struct bt_mesh_ponoff_cli *cli,
				       struct bt_mesh_msg_ctx *ctx,
				       enum bt_mesh_on_power_up *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PONOFF_OP_GET,
				 BT_MESH_PONOFF_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_PONOFF_OP_GET);

	return model_ackd_send(cli->model, ctx, &msg,
			       (rsp ? &cli->ack_ctx : NULL),
			       BT_MESH_PONOFF_OP_STATUS, rsp);
}

int bt_mesh_ponoff_cli_on_power_up_set(struct bt_mesh_ponoff_cli *cli,
				       struct bt_mesh_msg_ctx *ctx,
				       enum bt_mesh_on_power_up on_power_up,
				       enum bt_mesh_on_power_up *rsp)
{
	if (on_power_up >= BT_MESH_ON_POWER_UP_INVALID) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PONOFF_OP_SET,
				 BT_MESH_PONOFF_MSG_LEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_PONOFF_OP_SET);
	net_buf_simple_add_u8(&msg, on_power_up);

	return model_ackd_send(cli->model, ctx, &msg,
			       (rsp ? &cli->ack_ctx : NULL),
			       BT_MESH_PONOFF_OP_STATUS, rsp);
}

int bt_mesh_ponoff_cli_on_power_up_set_unack(
	struct bt_mesh_ponoff_cli *cli, struct bt_mesh_msg_ctx *ctx,
	enum bt_mesh_on_power_up on_power_up)
{
	if (on_power_up >= BT_MESH_ON_POWER_UP_INVALID) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PONOFF_OP_SET_UNACK,
				 BT_MESH_PONOFF_MSG_LEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_PONOFF_OP_SET_UNACK);
	net_buf_simple_add_u8(&msg, on_power_up);

	return model_send(cli->model, ctx, &msg);
}
