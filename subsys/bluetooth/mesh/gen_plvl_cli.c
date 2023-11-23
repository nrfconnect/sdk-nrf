/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <bluetooth/mesh/gen_plvl_cli.h>
#include "model_utils.h"

static int handle_power_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_PLVL_MSG_MINLEN_LEVEL_STATUS &&
	    buf->len != BT_MESH_PLVL_MSG_MAXLEN_LEVEL_STATUS) {
		return -EMSGSIZE;
	}

	struct bt_mesh_plvl_cli *cli = model->user_data;
	struct bt_mesh_plvl_status status;
	struct bt_mesh_plvl_status *rsp;

	status.current = net_buf_simple_pull_le16(buf);
	if (buf->len == 3) {
		status.target = net_buf_simple_pull_le16(buf);
		status.remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else {
		status.target = status.current;
		status.remaining_time = 0;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_PLVL_OP_LEVEL_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->power_status) {
		cli->handlers->power_status(cli, ctx, &status);
	}

	return 0;
}

static int handle_last_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_plvl_cli *cli = model->user_data;
	uint16_t last = net_buf_simple_pull_le16(buf);
	uint16_t *rsp;

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_PLVL_OP_LAST_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = last;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->last_status) {
		cli->handlers->last_status(cli, ctx, last);
	}

	return 0;
}

static int handle_default_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_plvl_cli *cli = model->user_data;
	uint16_t default_lvl = net_buf_simple_pull_le16(buf);
	uint16_t *rsp;

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_PLVL_OP_DEFAULT_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = default_lvl;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->default_status) {
		cli->handlers->default_status(cli, ctx, default_lvl);
	}

	return 0;
}

static int handle_range_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_plvl_cli *cli = model->user_data;
	struct bt_mesh_plvl_range_status status;
	struct bt_mesh_plvl_range_status *rsp;

	status.status = net_buf_simple_pull_u8(buf);
	status.range.min = net_buf_simple_pull_le16(buf);
	status.range.max = net_buf_simple_pull_le16(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_PLVL_OP_RANGE_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->range_status) {
		cli->handlers->range_status(cli, ctx, &status);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_plvl_cli_op[] = {
	{
		BT_MESH_PLVL_OP_LEVEL_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_PLVL_MSG_MINLEN_LEVEL_STATUS),
		handle_power_status,
	},
	{
		BT_MESH_PLVL_OP_LAST_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_LAST_STATUS),
		handle_last_status,
	},
	{
		BT_MESH_PLVL_OP_DEFAULT_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_DEFAULT_STATUS),
		handle_default_status,
	},
	{
		BT_MESH_PLVL_OP_RANGE_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_RANGE_STATUS),
		handle_range_status,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_lvl_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_cli *cli = model->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_lvl_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_cli *cli = model->user_data;

	net_buf_simple_reset(model->pub->msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_plvl_cli_cb = {
	.init = bt_mesh_lvl_cli_init,
	.reset = bt_mesh_lvl_cli_reset,
};

int bt_mesh_plvl_cli_power_get(struct bt_mesh_plvl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_plvl_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_LEVEL_GET,
				 BT_MESH_PLVL_MSG_LEN_LEVEL_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_LEVEL_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_PLVL_OP_LEVEL_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_plvl_cli_power_set(struct bt_mesh_plvl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_plvl_set *set,
			       struct bt_mesh_plvl_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_LEVEL_SET,
				 BT_MESH_PLVL_MSG_MAXLEN_LEVEL_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_LEVEL_SET);

	net_buf_simple_add_le16(&buf, set->power_lvl);
	net_buf_simple_add_u8(&buf, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&buf, set->transition);
	}

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_PLVL_OP_LEVEL_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_plvl_cli_power_set_unack(struct bt_mesh_plvl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_plvl_set *set)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_LEVEL_SET_UNACK,
				 BT_MESH_PLVL_MSG_MAXLEN_LEVEL_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_LEVEL_SET_UNACK);

	net_buf_simple_add_le16(&buf, set->power_lvl);
	net_buf_simple_add_u8(&buf, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&buf, set->transition);
	}

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_plvl_cli_range_get(struct bt_mesh_plvl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_plvl_range_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_RANGE_GET,
				 BT_MESH_PLVL_MSG_LEN_RANGE_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_RANGE_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_PLVL_OP_RANGE_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_plvl_cli_range_set(struct bt_mesh_plvl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_plvl_range *range,
			       struct bt_mesh_plvl_range_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_RANGE_SET,
				 BT_MESH_PLVL_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_RANGE_SET);
	net_buf_simple_add_le16(&buf, range->min);
	net_buf_simple_add_le16(&buf, range->max);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_PLVL_OP_RANGE_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_plvl_cli_range_set_unack(struct bt_mesh_plvl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_plvl_range *range)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_RANGE_SET_UNACK,
				 BT_MESH_PLVL_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_RANGE_SET_UNACK);
	net_buf_simple_add_le16(&buf, range->min);
	net_buf_simple_add_le16(&buf, range->max);

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_plvl_cli_default_get(struct bt_mesh_plvl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx, uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_DEFAULT_GET,
				 BT_MESH_PLVL_MSG_LEN_DEFAULT_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_DEFAULT_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_PLVL_OP_DEFAULT_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_plvl_cli_default_set(struct bt_mesh_plvl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 uint16_t default_power, uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_DEFAULT_SET,
				 BT_MESH_PLVL_MSG_LEN_DEFAULT_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_DEFAULT_SET);
	net_buf_simple_add_le16(&buf, default_power);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_PLVL_OP_DEFAULT_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_plvl_cli_default_set_unack(struct bt_mesh_plvl_cli *cli,
				       struct bt_mesh_msg_ctx *ctx,
				       uint16_t default_power)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_DEFAULT_SET_UNACK,
				 BT_MESH_PLVL_MSG_LEN_DEFAULT_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_DEFAULT_SET_UNACK);
	net_buf_simple_add_le16(&buf, default_power);

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_plvl_cli_last_get(struct bt_mesh_plvl_cli *cli,
			      struct bt_mesh_msg_ctx *ctx, uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_PLVL_OP_LAST_GET,
				 BT_MESH_PLVL_MSG_LEN_LAST_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_PLVL_OP_LAST_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_PLVL_OP_LAST_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}
