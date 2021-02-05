/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/light_xyl_cli.h>
#include "model_utils.h"

static int status_decode(struct bt_mesh_light_xyl_cli *cli,
			 struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf, uint32_t opcode,
			 struct bt_mesh_light_xyl_status *status)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_MINLEN_STATUS &&
	    buf->len != BT_MESH_LIGHT_XYL_MSG_MAXLEN_STATUS) {
		return -EAGAIN;
	}

	status->params.lightness = net_buf_simple_pull_le16(buf);
	status->params.xy.x = net_buf_simple_pull_le16(buf);
	status->params.xy.y = net_buf_simple_pull_le16(buf);
	status->remaining_time =
		(buf->len == 1) ?
			model_transition_decode(net_buf_simple_pull_u8(buf)) :
			0;

	if (model_ack_match(&cli->ack_ctx, opcode, ctx)) {
		struct bt_mesh_light_xyl_status *rsp =
			(struct bt_mesh_light_xyl_status *)
				cli->ack_ctx.user_data;
		*rsp = *status;
		model_ack_rx(&cli->ack_ctx);
	}
	return 0;
}

static void xyl_status_handle(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_light_xyl_cli *cli = model->user_data;
	struct bt_mesh_light_xyl_status status;

	uint32_t err = status_decode(cli, ctx, buf, BT_MESH_LIGHT_XYL_OP_STATUS,
				     &status);

	if (cli->handlers && cli->handlers->xyl_status && !err) {
		cli->handlers->xyl_status(cli, ctx, &status);
	}
}

static void target_status_handle(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_light_xyl_cli *cli = model->user_data;
	struct bt_mesh_light_xyl_status status;

	uint32_t err = status_decode(
		cli, ctx, buf, BT_MESH_LIGHT_XYL_OP_TARGET_STATUS, &status);

	if (cli->handlers && cli->handlers->target_status && !err) {
		cli->handlers->target_status(cli, ctx, &status);
	}
}

static void default_status_handle(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT) {
		return;
	}

	struct bt_mesh_light_xyl_cli *cli = model->user_data;
	struct bt_mesh_light_xyl status;

	status.lightness = net_buf_simple_pull_le16(buf);
	status.xy.x = net_buf_simple_pull_le16(buf);
	status.xy.y = net_buf_simple_pull_le16(buf);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS,
			    ctx)) {
		struct bt_mesh_light_xyl *rsp =
			(struct bt_mesh_light_xyl *)cli->ack_ctx.user_data;
		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->default_status) {
		cli->handlers->default_status(cli, ctx, &status);
	}
}

static void range_status_handle(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_STATUS) {
		return;
	}

	struct bt_mesh_light_xyl_cli *cli = model->user_data;
	struct bt_mesh_light_xyl_range_status status;

	status.status_code = net_buf_simple_pull_u8(buf);
	status.range.min.x = net_buf_simple_pull_le16(buf);
	status.range.max.x = net_buf_simple_pull_le16(buf);
	status.range.min.y = net_buf_simple_pull_le16(buf);
	status.range.max.y = net_buf_simple_pull_le16(buf);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LIGHT_XYL_OP_RANGE_STATUS,
			    ctx)) {
		struct bt_mesh_light_xyl_range_status *rsp =
			(struct bt_mesh_light_xyl_range_status *)
				cli->ack_ctx.user_data;
		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->range_status) {
		cli->handlers->range_status(cli, ctx, &status);
	}
}

const struct bt_mesh_model_op _bt_mesh_light_xyl_cli_op[] = {
	{ BT_MESH_LIGHT_XYL_OP_STATUS, BT_MESH_LIGHT_XYL_MSG_MINLEN_STATUS,
	  xyl_status_handle },
	{ BT_MESH_LIGHT_XYL_OP_TARGET_STATUS,
	  BT_MESH_LIGHT_XYL_MSG_MINLEN_STATUS, target_status_handle },
	{ BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS,
	  BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT, default_status_handle },
	{ BT_MESH_LIGHT_XYL_OP_RANGE_STATUS,
	  BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_STATUS, range_status_handle },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_light_xyl_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_xyl_cli *cli = model->user_data;

	if (!cli) {
		return -EINVAL;
	}

	cli->model = model;
	net_buf_simple_init_with_data(&cli->pub_msg, cli->buf,
				      sizeof(cli->buf));
	cli->pub.msg = &cli->pub_msg;
	model_ack_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_light_xyl_cli_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_light_xyl_cli *cli = model->user_data;

	net_buf_simple_reset(cli->pub.msg);
	model_ack_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_light_xyl_cli_cb = {
	.init = bt_mesh_light_xyl_cli_init,
	.reset = bt_mesh_light_xyl_cli_reset,
};

static int get_msg(struct bt_mesh_light_xyl_cli *cli,
		   struct bt_mesh_msg_ctx *ctx, void *rsp, uint16_t opcode,
		   uint16_t ret_opcode)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, opcode, BT_MESH_LIGHT_XYL_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, opcode);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL, ret_opcode, rsp);
}

int bt_mesh_light_xyl_get(struct bt_mesh_light_xyl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_xyl_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_XYL_OP_GET,
		       BT_MESH_LIGHT_XYL_OP_STATUS);
}

int bt_mesh_light_xyl_set(struct bt_mesh_light_xyl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_xyl_set_params *set,
			  struct bt_mesh_light_xyl_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_SET,
				 BT_MESH_LIGHT_XYL_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_XYL_OP_SET);
	net_buf_simple_add_le16(&msg, set->params.lightness);
	net_buf_simple_add_le16(&msg, set->params.xy.x);
	net_buf_simple_add_le16(&msg, set->params.xy.y);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHT_XYL_OP_STATUS, rsp);
}

int bt_mesh_light_xyl_set_unack(struct bt_mesh_light_xyl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_xyl_set_params *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_SET_UNACK,
				 BT_MESH_LIGHT_XYL_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_XYL_OP_SET_UNACK);
	net_buf_simple_add_le16(&msg, set->params.lightness);
	net_buf_simple_add_le16(&msg, set->params.xy.x);
	net_buf_simple_add_le16(&msg, set->params.xy.y);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_send(cli->model, ctx, &msg);
}

int bt_mesh_light_xyl_target_get(struct bt_mesh_light_xyl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_light_xyl_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_XYL_OP_TARGET_GET,
		       BT_MESH_LIGHT_XYL_OP_TARGET_STATUS);
}

int bt_mesh_light_xyl_default_get(struct bt_mesh_light_xyl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  struct bt_mesh_light_xyl *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_XYL_OP_DEFAULT_GET,
		       BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS);
}

int bt_mesh_light_xyl_default_set(struct bt_mesh_light_xyl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_light_xyl *set,
				  struct bt_mesh_light_xyl *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_DEFAULT_SET,
				 BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_XYL_OP_DEFAULT_SET);
	net_buf_simple_add_le16(&msg, set->lightness);
	net_buf_simple_add_le16(&msg, set->xy.x);
	net_buf_simple_add_le16(&msg, set->xy.y);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS, rsp);
}

int bt_mesh_light_xyl_default_set_unack(struct bt_mesh_light_xyl_cli *cli,
					struct bt_mesh_msg_ctx *ctx,
					const struct bt_mesh_light_xyl *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_DEFAULT_SET_UNACK,
				 BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_XYL_OP_DEFAULT_SET_UNACK);
	net_buf_simple_add_le16(&msg, set->lightness);
	net_buf_simple_add_le16(&msg, set->xy.x);
	net_buf_simple_add_le16(&msg, set->xy.y);

	return model_send(cli->model, ctx, &msg);
}

int bt_mesh_light_xyl_range_get(struct bt_mesh_light_xyl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_light_xyl_range_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_XYL_OP_RANGE_GET,
		       BT_MESH_LIGHT_XYL_OP_RANGE_STATUS);
}

int bt_mesh_light_xyl_range_set(struct bt_mesh_light_xyl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_xy_range *set,
				struct bt_mesh_light_xyl_range_status *rsp)
{
	if ((set->min.x > set->max.x) || (set->min.y > set->max.y)) {
		return -EFAULT;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_RANGE_SET,
				 BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_XYL_OP_RANGE_SET);
	net_buf_simple_add_le16(&msg, set->min.x);
	net_buf_simple_add_le16(&msg, set->max.x);
	net_buf_simple_add_le16(&msg, set->min.y);
	net_buf_simple_add_le16(&msg, set->max.y);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHT_XYL_OP_RANGE_STATUS, rsp);
}

int bt_mesh_light_xyl_range_set_unack(struct bt_mesh_light_xyl_cli *cli,
				      struct bt_mesh_msg_ctx *ctx,
				      const struct bt_mesh_light_xy_range *set)
{
	if ((set->min.x > set->max.x) || (set->min.y > set->max.y)) {
		return -EFAULT;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_RANGE_SET_UNACK,
				 BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_XYL_OP_RANGE_SET_UNACK);
	net_buf_simple_add_le16(&msg, set->min.x);
	net_buf_simple_add_le16(&msg, set->max.x);
	net_buf_simple_add_le16(&msg, set->min.y);
	net_buf_simple_add_le16(&msg, set->max.y);

	return model_send(cli->model, ctx, &msg);
}
