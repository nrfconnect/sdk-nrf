/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/gen_dtt_cli.h>
#include "model_utils.h"

static void handle_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_DTT_MSG_LEN_STATUS) {
		return;
	}

	struct bt_mesh_dtt_cli *cli = model->user_data;
	int32_t transition_time =
		model_transition_decode(net_buf_simple_pull_u8(buf));

	if (model_ack_match(&cli->ack_ctx, BT_MESH_DTT_OP_STATUS, ctx)) {
		int32_t *rsp = (int32_t *)cli->ack_ctx.user_data;
		*rsp = transition_time;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, transition_time);
	}
}

const struct bt_mesh_model_op _bt_mesh_dtt_cli_op[] = {
	{ BT_MESH_DTT_OP_STATUS, BT_MESH_DTT_MSG_LEN_STATUS, handle_status },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_dtt_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_dtt_cli *cli = mod->user_data;

	cli->model = mod;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	model_ack_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_dtt_reset(struct bt_mesh_model *mod)
{
	struct bt_mesh_dtt_cli *cli = mod->user_data;

	net_buf_simple_reset(mod->pub->msg);
	model_ack_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_dtt_cli_cb = {
	.init = bt_mesh_dtt_init,
	.reset = bt_mesh_dtt_reset,
};

int bt_mesh_dtt_get(struct bt_mesh_dtt_cli *cli, struct bt_mesh_msg_ctx *ctx,
		    int32_t *rsp_transition_time)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DTT_OP_GET,
				 BT_MESH_DTT_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_DTT_OP_GET);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp_transition_time ? &cli->ack_ctx : NULL,
			       BT_MESH_DTT_OP_STATUS, rsp_transition_time);
}

int bt_mesh_dtt_set(struct bt_mesh_dtt_cli *cli, struct bt_mesh_msg_ctx *ctx,
		    uint32_t transition_time, int32_t *rsp_transition_time)
{
	if (transition_time > BT_MESH_MODEL_TRANSITION_TIME_MAX_MS) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DTT_OP_SET,
				 BT_MESH_DTT_MSG_LEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_DTT_OP_SET);
	net_buf_simple_add_u8(&msg,
			      model_transition_encode((int32_t)transition_time));

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp_transition_time ? &cli->ack_ctx : NULL,
			       BT_MESH_DTT_OP_STATUS, rsp_transition_time);
}

int bt_mesh_dtt_set_unack(struct bt_mesh_dtt_cli *cli,
			  struct bt_mesh_msg_ctx *ctx, uint32_t transition_time)
{
	if (transition_time > BT_MESH_MODEL_TRANSITION_TIME_MAX_MS) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DTT_OP_SET_UNACK,
				 BT_MESH_DTT_MSG_LEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_DTT_OP_SET_UNACK);
	net_buf_simple_add_u8(&msg,
			      model_transition_encode((int32_t)transition_time));

	return model_send(cli->model, ctx, &msg);
}
