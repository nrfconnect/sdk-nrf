/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/gen_ponoff_cli.h>
#include "model_utils.h"

static int handle_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct bt_mesh_ponoff_cli *cli = model->user_data;
	enum bt_mesh_on_power_up on_power_up = net_buf_simple_pull_u8(buf);
	enum bt_mesh_on_power_up *rsp;

	if (on_power_up >= BT_MESH_ON_POWER_UP_INVALID) {
		return -EINVAL;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_PONOFF_OP_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = on_power_up;

		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, on_power_up);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_ponoff_cli_op[] = {
	{
		BT_MESH_PONOFF_OP_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_PONOFF_MSG_LEN_STATUS),
		handle_status,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_ponoff_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_ponoff_cli *cli = model->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_ponoff_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_ponoff_cli *cli = model->user_data;

	net_buf_simple_reset(model->pub->msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_ponoff_cli_cb = {
	.init = bt_mesh_ponoff_cli_init,
	.reset = bt_mesh_ponoff_cli_reset,
};

int bt_mesh_ponoff_cli_on_power_up_get(struct bt_mesh_ponoff_cli *cli,
				       struct bt_mesh_msg_ctx *ctx,
				       enum bt_mesh_on_power_up *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PONOFF_OP_GET,
				 BT_MESH_PONOFF_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_PONOFF_OP_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_PONOFF_OP_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
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

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_PONOFF_OP_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
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

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}
