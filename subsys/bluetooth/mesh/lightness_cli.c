/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <bluetooth/mesh/lightness_cli.h>
#include "model_utils.h"
#include "lightness_internal.h"

static int light_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf, enum light_repr repr)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_MINLEN_STATUS &&
	    buf->len != BT_MESH_LIGHTNESS_MSG_MAXLEN_STATUS) {
		return -EMSGSIZE;
	}

	struct bt_mesh_lightness_cli *cli = model->rt->user_data;
	struct bt_mesh_lightness_status status;
	struct bt_mesh_lightness_status *rsp;

	status.current = repr_to_light(net_buf_simple_pull_le16(buf), repr);
	if (buf->len == 3) {
		status.target =
			repr_to_light(net_buf_simple_pull_le16(buf), repr);
		status.remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else {
		status.target = status.current;
		status.remaining_time = 0;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, op_get(LIGHTNESS_OP_TYPE_STATUS, repr),
				      ctx->addr, (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->light_status) {
		cli->handlers->light_status(cli, ctx, &status);
	}

	return 0;
}

static int handle_light_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	return light_status(model, ctx, buf, ACTUAL);
}

static int handle_light_linear_status(const struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	return light_status(model, ctx, buf, LINEAR);
}

static int handle_last_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_lightness_cli *cli = model->rt->user_data;
	uint16_t last = from_actual(net_buf_simple_pull_le16(buf));
	uint16_t *rsp;

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHTNESS_OP_LAST_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = last;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->last_light_status) {
		cli->handlers->last_light_status(cli, ctx, last);
	}

	return 0;
}

static int handle_default_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_lightness_cli *cli = model->rt->user_data;
	uint16_t default_lvl = from_actual(net_buf_simple_pull_le16(buf));
	uint16_t *rsp;

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS, ctx->addr,
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
	struct bt_mesh_lightness_cli *cli = model->rt->user_data;
	struct bt_mesh_lightness_range_status status;
	struct bt_mesh_lightness_range_status *rsp;

	status.status = net_buf_simple_pull_u8(buf);
	status.range.min = from_actual(net_buf_simple_pull_le16(buf));
	status.range.max = from_actual(net_buf_simple_pull_le16(buf));

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHTNESS_OP_RANGE_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->range_status) {
		cli->handlers->range_status(cli, ctx, &status);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_lightness_cli_op[] = {
	{
		BT_MESH_LIGHTNESS_OP_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_LIGHTNESS_MSG_MINLEN_STATUS),
		handle_light_status,
	},
	{
		BT_MESH_LIGHTNESS_OP_LINEAR_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_LIGHTNESS_MSG_MINLEN_STATUS),
		handle_light_linear_status,
	},
	{
		BT_MESH_LIGHTNESS_OP_LAST_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_LAST_STATUS),
		handle_last_status,
	},
	{
		BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_STATUS),
		handle_default_status,
	},
	{
		BT_MESH_LIGHTNESS_OP_RANGE_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_RANGE_STATUS),
		handle_range_status,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_lvl_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_lightness_cli *cli = model->rt->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data, sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_lvl_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_lightness_cli *cli = model->rt->user_data;

	net_buf_simple_reset(model->pub->msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_lightness_cli_cb = {
	.init = bt_mesh_lvl_cli_init,
	.reset = bt_mesh_lvl_cli_reset,
};

int lightness_cli_light_get(struct bt_mesh_lightness_cli *cli, struct bt_mesh_msg_ctx *ctx,
			    enum light_repr repr, struct bt_mesh_lightness_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_GET, BT_MESH_LIGHTNESS_MSG_LEN_GET);
	bt_mesh_model_msg_init(&buf, op_get(LIGHTNESS_OP_TYPE_GET, repr));

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = op_get(LIGHTNESS_OP_TYPE_STATUS, repr),
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int lightness_cli_light_set(struct bt_mesh_lightness_cli *cli, struct bt_mesh_msg_ctx *ctx,
			    enum light_repr repr, const struct bt_mesh_lightness_set *set,
			    struct bt_mesh_lightness_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_SET, BT_MESH_LIGHTNESS_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&buf, op_get(LIGHTNESS_OP_TYPE_SET, repr));

	net_buf_simple_add_le16(&buf, set->lvl);
	net_buf_simple_add_u8(&buf, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&buf, set->transition);
	}

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = op_get(LIGHTNESS_OP_TYPE_STATUS, repr),
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int lightness_cli_light_set_unack(struct bt_mesh_lightness_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  enum light_repr repr,
				  const struct bt_mesh_lightness_set *set)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_SET_UNACK,
				 BT_MESH_LIGHTNESS_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&buf, op_get(LIGHTNESS_OP_TYPE_SET_UNACK, repr));

	net_buf_simple_add_le16(&buf, set->lvl);
	net_buf_simple_add_u8(&buf, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&buf, set->transition);
	}

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_lightness_cli_light_get(struct bt_mesh_lightness_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_lightness_status *rsp)
{
	return lightness_cli_light_get(cli, ctx, LIGHT_USER_REPR, rsp);
}

int bt_mesh_lightness_cli_light_set(struct bt_mesh_lightness_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_lightness_set *set,
				    struct bt_mesh_lightness_status *rsp)
{
	return lightness_cli_light_set(cli, ctx, LIGHT_USER_REPR, set, rsp);
}

int bt_mesh_lightness_cli_light_set_unack(
	struct bt_mesh_lightness_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_lightness_set *set)
{
	return lightness_cli_light_set_unack(cli, ctx, LIGHT_USER_REPR, set);
}

int bt_mesh_lightness_cli_range_get(struct bt_mesh_lightness_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_lightness_range_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_RANGE_GET,
				 BT_MESH_LIGHTNESS_MSG_LEN_RANGE_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_RANGE_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHTNESS_OP_RANGE_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_lightness_cli_range_set(struct bt_mesh_lightness_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_lightness_range *range,
				    struct bt_mesh_lightness_range_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_RANGE_SET,
				 BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_RANGE_SET);
	net_buf_simple_add_le16(&buf, to_actual(range->min));
	net_buf_simple_add_le16(&buf, to_actual(range->max));

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHTNESS_OP_RANGE_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_lightness_cli_range_set_unack(
	struct bt_mesh_lightness_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_lightness_range *range)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_RANGE_SET_UNACK,
				 BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_RANGE_SET_UNACK);
	net_buf_simple_add_le16(&buf, to_actual(range->min));
	net_buf_simple_add_le16(&buf, to_actual(range->max));

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_lightness_cli_default_get(struct bt_mesh_lightness_cli *cli,
				      struct bt_mesh_msg_ctx *ctx, uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_DEFAULT_GET,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_DEFAULT_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_lightness_cli_default_set(struct bt_mesh_lightness_cli *cli,
				      struct bt_mesh_msg_ctx *ctx,
				      uint16_t default_light, uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_DEFAULT_SET,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_DEFAULT_SET);
	net_buf_simple_add_le16(&buf, to_actual(default_light));

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_lightness_cli_default_set_unack(struct bt_mesh_lightness_cli *cli,
					    struct bt_mesh_msg_ctx *ctx,
					    uint16_t default_light)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_DEFAULT_SET_UNACK,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_DEFAULT_SET_UNACK);
	net_buf_simple_add_le16(&buf, to_actual(default_light));

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_lightness_cli_last_get(struct bt_mesh_lightness_cli *cli,
				   struct bt_mesh_msg_ctx *ctx, uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_LAST_GET,
				 BT_MESH_LIGHTNESS_MSG_LEN_LAST_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_LAST_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHTNESS_OP_LAST_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}
