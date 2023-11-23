/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <bluetooth/mesh/gen_onoff_cli.h>
#include "model_utils.h"

static int decode_status(struct net_buf_simple *buf,
			  struct bt_mesh_onoff_status *status)
{
	uint8_t on_off;

	on_off = net_buf_simple_pull_u8(buf);
	if (on_off > 1) {
		return -EINVAL;
	}
	status->present_on_off = on_off;

	if (buf->len == 2) {
		on_off = net_buf_simple_pull_u8(buf);
		if (on_off > 1) {
			return -EINVAL;
		}
		status->target_on_off = on_off;
		status->remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else {
		status->target_on_off = status->present_on_off;
		status->remaining_time = 0;
	}

	return 0;
}

static int handle_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_ONOFF_MSG_MINLEN_STATUS &&
	    buf->len != BT_MESH_ONOFF_MSG_MAXLEN_STATUS) {
		return -EMSGSIZE;
	}

	struct bt_mesh_onoff_cli *cli = model->user_data;
	struct bt_mesh_onoff_status status;
	struct bt_mesh_onoff_status *rsp;
	int err;

	err = decode_status(buf, &status);
	if (err) {
		return err;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_ONOFF_OP_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, &status);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_onoff_cli_op[] = {
	{
		BT_MESH_ONOFF_OP_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_ONOFF_MSG_MINLEN_STATUS),
		handle_status,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_onoff_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_onoff_cli *cli = model->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_onoff_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_onoff_cli *cli = model->user_data;

	net_buf_simple_reset(cli->pub.msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_onoff_cli_cb = {
	.init = bt_mesh_onoff_cli_init,
	.reset = bt_mesh_onoff_cli_reset,
};

int bt_mesh_onoff_cli_get(struct bt_mesh_onoff_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_onoff_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_GET,
				 BT_MESH_ONOFF_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_ONOFF_OP_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_ONOFF_OP_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_onoff_cli_set(struct bt_mesh_onoff_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_onoff_set *set,
			  struct bt_mesh_onoff_status *rsp)
{
	if (!set->reuse_transaction) {
		cli->tid++;
	}
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_SET,
				 BT_MESH_ONOFF_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_ONOFF_OP_SET);

	net_buf_simple_add_u8(&msg, set->on_off);
	net_buf_simple_add_u8(&msg, cli->tid);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_ONOFF_OP_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_onoff_cli_set_unack(struct bt_mesh_onoff_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_onoff_set *set)
{
	if (!set->reuse_transaction) {
		cli->tid++;
	}
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_SET_UNACK,
				 BT_MESH_ONOFF_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_ONOFF_OP_SET_UNACK);

	net_buf_simple_add_u8(&msg, set->on_off);
	net_buf_simple_add_u8(&msg, cli->tid);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}
