/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/gen_loc_cli.h>
#include "model_utils.h"
#include "gen_loc_internal.h"

static int handle_global_loc(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_loc_cli *cli = model->rt->user_data;
	struct bt_mesh_loc_global loc;
	struct bt_mesh_loc_global *rsp;

	bt_mesh_loc_global_decode(buf, &loc);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LOC_OP_GLOBAL_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = loc;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->global_status) {
		cli->handlers->global_status(cli, ctx, &loc);
	}

	return 0;
}

static int handle_local_loc(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_loc_cli *cli = model->rt->user_data;
	struct bt_mesh_loc_local loc;
	struct bt_mesh_loc_local *rsp;

	bt_mesh_loc_local_decode(buf, &loc);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LOC_OP_LOCAL_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = loc;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->local_status) {
		cli->handlers->local_status(cli, ctx, &loc);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_loc_cli_op[] = {
	{
		BT_MESH_LOC_OP_GLOBAL_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS),
		handle_global_loc,
	},
	{
		BT_MESH_LOC_OP_LOCAL_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_LOC_MSG_LEN_LOCAL_STATUS),
		handle_local_loc,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_loc_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_loc_cli *cli = model->rt->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_loc_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_loc_cli *cli = model->rt->user_data;

	net_buf_simple_reset(model->pub->msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_loc_cli_cb = {
	.init = bt_mesh_loc_init,
	.reset = bt_mesh_loc_reset,
};

int bt_mesh_loc_cli_global_get(struct bt_mesh_loc_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_loc_global *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_GLOBAL_GET,
				 BT_MESH_LOC_MSG_LEN_GLOBAL_GET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_GLOBAL_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LOC_OP_GLOBAL_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_loc_cli_global_set(struct bt_mesh_loc_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_loc_global *loc,
			       struct bt_mesh_loc_global *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_GLOBAL_SET,
				 BT_MESH_LOC_MSG_LEN_GLOBAL_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_GLOBAL_SET);
	bt_mesh_loc_global_encode(&msg, loc);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LOC_OP_GLOBAL_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_loc_cli_global_set_unack(struct bt_mesh_loc_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_loc_global *loc)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_GLOBAL_SET_UNACK,
				 BT_MESH_LOC_MSG_LEN_GLOBAL_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_GLOBAL_SET_UNACK);
	bt_mesh_loc_global_encode(&msg, loc);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_loc_cli_local_get(struct bt_mesh_loc_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_loc_local *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_LOCAL_GET,
				 BT_MESH_LOC_MSG_LEN_LOCAL_GET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_LOCAL_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LOC_OP_LOCAL_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_loc_cli_local_set(struct bt_mesh_loc_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_loc_local *loc,
			      struct bt_mesh_loc_local *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_LOCAL_SET,
				 BT_MESH_LOC_MSG_LEN_LOCAL_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_LOCAL_SET);
	bt_mesh_loc_local_encode(&msg, loc);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LOC_OP_LOCAL_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_loc_cli_local_set_unack(struct bt_mesh_loc_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_loc_local *loc)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_LOCAL_SET_UNACK,
				 BT_MESH_LOC_MSG_LEN_LOCAL_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_LOCAL_SET_UNACK);
	bt_mesh_loc_local_encode(&msg, loc);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}
