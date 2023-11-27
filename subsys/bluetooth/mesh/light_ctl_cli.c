/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/light_ctl_cli.h>
#include "model_utils.h"

static int handle_ctl_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_MINLEN_STATUS &&
	    buf->len != BT_MESH_LIGHT_CTL_MSG_MAXLEN_STATUS) {
		return -EMSGSIZE;
	}

	struct bt_mesh_light_ctl_cli *cli = model->rt->user_data;
	struct bt_mesh_light_ctl_status status;
	struct bt_mesh_light_ctl_status *rsp;

	status.current_light = net_buf_simple_pull_le16(buf);
	status.current_temp = net_buf_simple_pull_le16(buf);
	if ((status.current_temp < BT_MESH_LIGHT_TEMP_MIN) ||
	    (status.current_temp > BT_MESH_LIGHT_TEMP_MAX)) {
		return -EINVAL;
	}

	if (buf->len == 5) {
		status.target_light = net_buf_simple_pull_le16(buf);
		status.target_temp = net_buf_simple_pull_le16(buf);
		if ((status.target_temp < BT_MESH_LIGHT_TEMP_MIN) ||
		    (status.target_temp > BT_MESH_LIGHT_TEMP_MAX)) {
			return -EINVAL;
		}

		status.remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else {
		status.target_light = status.current_light;
		status.target_temp = status.current_temp;
		status.remaining_time = 0;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHT_CTL_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->ctl_status) {
		cli->handlers->ctl_status(cli, ctx, &status);
	}

	return 0;
}

static int handle_temp_range_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctl_cli *cli = model->rt->user_data;
	struct bt_mesh_light_temp_range_status status;
	struct bt_mesh_light_temp_range_status *rsp;

	status.status = net_buf_simple_pull_u8(buf);
	status.range.min = net_buf_simple_pull_le16(buf);
	status.range.max = net_buf_simple_pull_le16(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHT_TEMP_RANGE_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->temp_range_status) {
		cli->handlers->temp_range_status(cli, ctx, &status);
	}

	return 0;
}

static int handle_temp_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_STATUS &&
	    buf->len != BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_STATUS) {
		return -EMSGSIZE;
	}

	struct bt_mesh_light_ctl_cli *cli = model->rt->user_data;
	struct bt_mesh_light_temp_status status;
	struct bt_mesh_light_temp_status *rsp;

	status.current.temp = net_buf_simple_pull_le16(buf);
	if ((status.current.temp < BT_MESH_LIGHT_TEMP_MIN) ||
	    (status.current.temp > BT_MESH_LIGHT_TEMP_MAX)) {
		return -EINVAL;
	}

	status.current.delta_uv = net_buf_simple_pull_le16(buf);

	if (buf->len == 5) {
		status.target.temp = net_buf_simple_pull_le16(buf);
		if ((status.target.temp < BT_MESH_LIGHT_TEMP_MIN) ||
		    (status.target.temp > BT_MESH_LIGHT_TEMP_MAX)) {
			return -EINVAL;
		}

		status.target.delta_uv = net_buf_simple_pull_le16(buf);
		status.remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else {
		status.target.temp = status.current.temp;
		status.target.delta_uv = status.current.delta_uv;
		status.remaining_time = 0;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHT_TEMP_STATUS,
			    ctx->addr, (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->temp_status) {
		cli->handlers->temp_status(cli, ctx, &status);
	}

	return 0;
}

static int handle_default_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctl_cli *cli = model->rt->user_data;
	struct bt_mesh_light_ctl status;
	struct bt_mesh_light_ctl *rsp;

	status.light = net_buf_simple_pull_le16(buf);
	status.temp = net_buf_simple_pull_le16(buf);
	if ((status.temp < BT_MESH_LIGHT_TEMP_MIN) ||
	    (status.temp > BT_MESH_LIGHT_TEMP_MAX)) {
		return -EINVAL;
	}

	status.delta_uv = net_buf_simple_pull_le16(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHT_CTL_DEFAULT_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->default_status) {
		cli->handlers->default_status(cli, ctx, &status);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_light_ctl_cli_op[] = {
	{
		BT_MESH_LIGHT_CTL_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTL_MSG_MINLEN_STATUS),
		handle_ctl_status,
	},
	{
		BT_MESH_LIGHT_TEMP_RANGE_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_STATUS),
		handle_temp_range_status,
	},
	{
		BT_MESH_LIGHT_TEMP_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_STATUS),
		handle_temp_status,
	},
	{
		BT_MESH_LIGHT_CTL_DEFAULT_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG),
		handle_default_status,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_light_ctl_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctl_cli *cli = model->rt->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_light_ctl_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctl_cli *cli = model->rt->user_data;

	net_buf_simple_reset(cli->pub.msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_light_ctl_cli_cb = {
	.init = bt_mesh_light_ctl_cli_init,
	.reset = bt_mesh_light_ctl_cli_reset,
};

static int get_msg(struct bt_mesh_light_ctl_cli *cli,
		   struct bt_mesh_msg_ctx *ctx, void *rsp, uint16_t opcode,
		   uint16_t ret_opcode)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, opcode, BT_MESH_LIGHT_CTL_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, opcode);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = ret_opcode,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_light_ctl_get(struct bt_mesh_light_ctl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_ctl_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_CTL_GET,
		       BT_MESH_LIGHT_CTL_STATUS);
}

int bt_mesh_light_ctl_set(struct bt_mesh_light_ctl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_ctl_set *set,
			  struct bt_mesh_light_ctl_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_SET,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_CTL_SET);
	net_buf_simple_add_le16(&msg, set->params.light);
	net_buf_simple_add_le16(&msg, set->params.temp);
	net_buf_simple_add_le16(&msg, set->params.delta_uv);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTL_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_light_ctl_set_unack(struct bt_mesh_light_ctl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_ctl_set *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_SET_UNACK,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_CTL_SET_UNACK);
	net_buf_simple_add_le16(&msg, set->params.light);
	net_buf_simple_add_le16(&msg, set->params.temp);
	net_buf_simple_add_le16(&msg, set->params.delta_uv);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_light_temp_get(struct bt_mesh_light_ctl_cli *cli,
			   struct bt_mesh_msg_ctx *ctx,
			   struct bt_mesh_light_temp_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_TEMP_GET,
		       BT_MESH_LIGHT_TEMP_STATUS);
}

int bt_mesh_light_temp_set(struct bt_mesh_light_ctl_cli *cli,
			   struct bt_mesh_msg_ctx *ctx,
			   const struct bt_mesh_light_temp_set *set,
			   struct bt_mesh_light_temp_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_SET,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_TEMP_SET);
	net_buf_simple_add_le16(&msg, set->params.temp);
	net_buf_simple_add_le16(&msg, set->params.delta_uv);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_TEMP_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_light_temp_set_unack(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_temp_set *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_SET_UNACK,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_TEMP_SET_UNACK);
	net_buf_simple_add_le16(&msg, set->params.temp);
	net_buf_simple_add_le16(&msg, set->params.delta_uv);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_light_ctl_default_get(struct bt_mesh_light_ctl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  struct bt_mesh_light_ctl *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_CTL_DEFAULT_GET,
		       BT_MESH_LIGHT_CTL_DEFAULT_STATUS);
}

int bt_mesh_light_ctl_default_set(struct bt_mesh_light_ctl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_light_ctl *set,
				  struct bt_mesh_light_ctl *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_DEFAULT_SET,
				 BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_CTL_DEFAULT_SET);
	net_buf_simple_add_le16(&msg, set->light);
	net_buf_simple_add_le16(&msg, set->temp);
	net_buf_simple_add_le16(&msg, set->delta_uv);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTL_DEFAULT_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_light_ctl_default_set_unack(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_ctl *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_DEFAULT_SET_UNACK,
				 BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_CTL_DEFAULT_SET_UNACK);
	net_buf_simple_add_le16(&msg, set->light);
	net_buf_simple_add_le16(&msg, set->temp);
	net_buf_simple_add_le16(&msg, set->delta_uv);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_light_temp_range_get(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_light_temp_range_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_TEMP_RANGE_GET,
		       BT_MESH_LIGHT_TEMP_RANGE_STATUS);
}

int bt_mesh_light_temp_range_set(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_temp_range *set,
	struct bt_mesh_light_temp_range_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_RANGE_SET,
				 BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_TEMP_RANGE_SET);
	net_buf_simple_add_le16(&msg, set->min);
	net_buf_simple_add_le16(&msg, set->max);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_TEMP_RANGE_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_light_temp_range_set_unack(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_temp_range *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_RANGE_SET_UNACK,
				 BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_TEMP_RANGE_SET_UNACK);
	net_buf_simple_add_le16(&msg, set->min);
	net_buf_simple_add_le16(&msg, set->max);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}
