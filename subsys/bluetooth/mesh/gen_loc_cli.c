/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/mesh/gen_loc_cli.h>
#include "model_utils.h"
#include "gen_loc_internal.h"

static void handle_global_loc(struct bt_mesh_model *mod,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS) {
		return;
	}

	struct bt_mesh_loc_cli *cli = mod->user_data;
	struct bt_mesh_loc_global loc;

	bt_mesh_loc_global_decode(buf, &loc);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LOC_OP_GLOBAL_STATUS, ctx)) {
		struct bt_mesh_loc_global *rsp =
			(struct bt_mesh_loc_global *)cli->ack_ctx.user_data;

		*rsp = loc;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->global_status) {
		cli->handlers->global_status(cli, ctx, &loc);
	}
}

static void handle_local_loc(struct bt_mesh_model *mod,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LOC_MSG_LEN_LOCAL_STATUS) {
		return;
	}

	struct bt_mesh_loc_cli *cli = mod->user_data;
	struct bt_mesh_loc_local loc;

	bt_mesh_loc_local_decode(buf, &loc);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LOC_OP_LOCAL_STATUS, ctx)) {
		struct bt_mesh_loc_local *rsp =
			(struct bt_mesh_loc_local *)cli->ack_ctx.user_data;

		*rsp = loc;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->local_status) {
		cli->handlers->local_status(cli, ctx, &loc);
	}
}

const struct bt_mesh_model_op _bt_mesh_loc_cli_op[] = {
	{ BT_MESH_LOC_OP_GLOBAL_STATUS, BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS,
	  handle_global_loc },
	{ BT_MESH_LOC_OP_LOCAL_STATUS, BT_MESH_LOC_MSG_LEN_LOCAL_STATUS,
	  handle_local_loc },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_loc_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_loc_cli *cli = mod->user_data;

	cli->model = mod;
	net_buf_simple_init(mod->pub->msg, 0);
	model_ack_init(&cli->ack_ctx);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_loc_cli_cb = {
	.init = bt_mesh_loc_init,
};

int bt_mesh_loc_cli_global_get(struct bt_mesh_loc_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_loc_global *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_GLOBAL_GET,
				 BT_MESH_LOC_MSG_LEN_GLOBAL_GET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_GLOBAL_GET);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LOC_OP_GLOBAL_STATUS, rsp);
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

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LOC_OP_GLOBAL_STATUS, rsp);
}

int bt_mesh_loc_cli_global_set_unack(struct bt_mesh_loc_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_loc_global *loc)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_GLOBAL_SET_UNACK,
				 BT_MESH_LOC_MSG_LEN_GLOBAL_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_GLOBAL_SET_UNACK);
	bt_mesh_loc_global_encode(&msg, loc);

	return model_send(cli->model, ctx, &msg);
}

int bt_mesh_loc_cli_local_get(struct bt_mesh_loc_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_loc_local *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_LOCAL_GET,
				 BT_MESH_LOC_MSG_LEN_LOCAL_GET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_LOCAL_GET);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LOC_OP_LOCAL_STATUS, rsp);
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

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LOC_OP_LOCAL_STATUS, rsp);
}

int bt_mesh_loc_cli_local_set_unack(struct bt_mesh_loc_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_loc_local *loc)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_LOCAL_SET_UNACK,
				 BT_MESH_LOC_MSG_LEN_LOCAL_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_LOCAL_SET_UNACK);
	bt_mesh_loc_local_encode(&msg, loc);

	return model_send(cli->model, ctx, &msg);
}
