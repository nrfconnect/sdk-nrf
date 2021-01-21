/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/mesh/light_hsl_cli.h>
#include "model_utils.h"
#include "light_hsl_internal.h"

static int status_decode(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf, uint32_t opcode,
			  struct bt_mesh_light_hsl_status *status)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_MINLEN_STATUS &&
	    buf->len != BT_MESH_LIGHT_HSL_MSG_MAXLEN_STATUS) {
		return -EINVAL;
	}

	light_hsl_buf_pull(buf, &status->params);
	status->remaining_time =
		(buf->len == 1) ?
			model_transition_decode(net_buf_simple_pull_u8(buf)) :
			0;

	if (model_ack_match(&cli->ack_ctx, opcode, ctx)) {
		struct bt_mesh_light_hsl_status *rsp =
			(struct bt_mesh_light_hsl_status *)
				cli->ack_ctx.user_data;
		*rsp = *status;
		model_ack_rx(&cli->ack_ctx);
	}

	return 0;
}

static void hsl_status_handle(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_cli *cli = model->user_data;
	struct bt_mesh_light_hsl_status status;
	int err;

	err = status_decode(cli, ctx, buf, BT_MESH_LIGHT_HSL_OP_STATUS,
			    &status);
	if (err) {
		return;
	}

	if (cli->handlers->status) {
		cli->handlers->status(cli, ctx, &status);
	}
}

static void hsl_target_status_handle(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_cli *cli = model->user_data;
	struct bt_mesh_light_hsl_status status;
	int err;

	err = status_decode(cli, ctx, buf, BT_MESH_LIGHT_HSL_OP_TARGET_STATUS,
			    &status);
	if (err) {
		return;
	}

	if (cli->handlers->target_status) {
		cli->handlers->target_status(cli, ctx, &status);
	}
}

static void default_status_handle(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS) {
		return;
	}

	struct bt_mesh_light_hsl_cli *cli = model->user_data;
	struct bt_mesh_light_hsl status;

	light_hsl_buf_pull(buf, &status);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS,
			    ctx)) {
		struct bt_mesh_light_hsl *rsp =
			(struct bt_mesh_light_hsl *)cli->ack_ctx.user_data;
		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers->default_status) {
		cli->handlers->default_status(cli, ctx, &status);
	}
}

static void range_status_handle(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_STATUS) {
		return;
	}

	struct bt_mesh_light_hsl_cli *cli = model->user_data;
	struct bt_mesh_light_hsl_range_status status;

	status.status_code = net_buf_simple_pull_u8(buf);
	light_hue_sat_range_buf_pull(buf, &status.range);

	if (model_ack_match(&cli->ack_ctx,
			    BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_STATUS, ctx)) {
		struct bt_mesh_light_hsl_range_status *rsp =
			(struct bt_mesh_light_hsl_range_status *)
				cli->ack_ctx.user_data;
		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers->range_status) {
		cli->handlers->range_status(cli, ctx, &status);
	}
}

static void hue_status_handle(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_cli *cli = model->user_data;
	struct bt_mesh_light_hue_status status;

	if (buf->len != BT_MESH_LIGHT_HSL_MSG_MINLEN_HUE_STATUS &&
	    buf->len != BT_MESH_LIGHT_HSL_MSG_MAXLEN_HUE_STATUS) {
		return;
	}

	status.current = net_buf_simple_pull_le16(buf);
	status.target = net_buf_simple_pull_le16(buf);
	if (buf->len) {
		status.remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else {
		status.remaining_time = 0;
	}

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LIGHT_HUE_OP_STATUS, ctx)) {
		struct bt_mesh_light_hue_status *rsp = cli->ack_ctx.user_data;

		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers->hue_status) {
		cli->handlers->hue_status(cli, ctx, &status);
	}
}

static void saturation_status_handle(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_cli *cli = model->user_data;
	struct bt_mesh_light_sat_status status;

	if (buf->len != BT_MESH_LIGHT_HSL_MSG_MINLEN_SAT_STATUS &&
	    buf->len != BT_MESH_LIGHT_HSL_MSG_MAXLEN_SAT_STATUS) {
		return;
	}

	status.current = net_buf_simple_pull_le16(buf);
	status.target = net_buf_simple_pull_le16(buf);
	if (buf->len) {
		status.remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else {
		status.remaining_time = 0;
	}

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LIGHT_SAT_OP_STATUS, ctx)) {
		struct bt_mesh_light_sat_status *rsp = cli->ack_ctx.user_data;

		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers->saturation_status) {
		cli->handlers->saturation_status(cli, ctx, &status);
	}
}

const struct bt_mesh_model_op _bt_mesh_light_hsl_cli_op[] = {
	{
		BT_MESH_LIGHT_HSL_OP_STATUS,
		BT_MESH_LIGHT_HSL_MSG_MINLEN_STATUS,
		hsl_status_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_TARGET_STATUS,
		BT_MESH_LIGHT_HSL_MSG_MINLEN_STATUS,
		hsl_target_status_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS,
		BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT,
		default_status_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_RANGE_STATUS,
		BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_STATUS,
		range_status_handle,
	},
	{
		BT_MESH_LIGHT_HUE_OP_STATUS,
		BT_MESH_LIGHT_HSL_MSG_MINLEN_HUE_STATUS,
		hue_status_handle,
	},
	{
		BT_MESH_LIGHT_SAT_OP_STATUS,
		BT_MESH_LIGHT_HSL_MSG_MINLEN_HUE_STATUS,
		saturation_status_handle,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_light_hsl_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_hsl_cli *cli = model->user_data;

	cli->model = model;
	cli->pub.msg = &cli->buf;
	net_buf_simple_init_with_data(&cli->buf, cli->pub_data,
				      ARRAY_SIZE(cli->pub_data));
	model_ack_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_light_hsl_cli_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_light_hsl_cli *cli = model->user_data;

	net_buf_simple_reset(cli->pub.msg);
	model_ack_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_light_hsl_cli_cb = {
	.init = bt_mesh_light_hsl_cli_init,
	.reset = bt_mesh_light_hsl_cli_reset,
};

static int get_msg(struct bt_mesh_light_hsl_cli *cli,
		   struct bt_mesh_msg_ctx *ctx, void *rsp, uint16_t opcode,
		   uint16_t ret_opcode)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, opcode, BT_MESH_LIGHT_HSL_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, opcode);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL, ret_opcode, rsp);
}

int bt_mesh_light_hsl_get(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_hsl_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_HSL_OP_GET,
		       BT_MESH_LIGHT_HSL_OP_STATUS);
}

int bt_mesh_light_hsl_set(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_hsl_params *set,
			  struct bt_mesh_light_hsl_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_SET,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HSL_OP_SET);
	light_hsl_buf_push(&msg,  &set->params);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHT_HSL_OP_STATUS, rsp);
}

int bt_mesh_light_hsl_set_unack(struct bt_mesh_light_hsl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_hsl_params *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_SET_UNACK,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HSL_OP_SET_UNACK);
	light_hsl_buf_push(&msg,  &set->params);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_send(cli->model, ctx, &msg);
}

int bt_mesh_light_hsl_target_get(struct bt_mesh_light_hsl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_light_hsl_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_HSL_OP_TARGET_GET,
		       BT_MESH_LIGHT_HSL_OP_TARGET_STATUS);
}

int bt_mesh_light_hsl_default_get(struct bt_mesh_light_hsl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  struct bt_mesh_light_hsl *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_HSL_OP_DEFAULT_GET,
		       BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS);
}

int bt_mesh_light_hsl_default_set(struct bt_mesh_light_hsl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_light_hsl *set,
				  struct bt_mesh_light_hsl *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_DEFAULT_SET,
				 BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HSL_OP_DEFAULT_SET);
	light_hsl_buf_push(&msg,  set);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS, rsp);
}

int bt_mesh_light_hsl_default_set_unack(struct bt_mesh_light_hsl_cli *cli,
					struct bt_mesh_msg_ctx *ctx,
					const struct bt_mesh_light_hsl *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_DEFAULT_SET_UNACK,
				 BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HSL_OP_DEFAULT_SET_UNACK);
	light_hsl_buf_push(&msg,  set);

	return model_send(cli->model, ctx, &msg);
}

int bt_mesh_light_hsl_range_get(struct bt_mesh_light_hsl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_light_hsl_range_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_HSL_OP_RANGE_GET,
		       BT_MESH_LIGHT_HSL_OP_RANGE_STATUS);
}

int bt_mesh_light_hsl_range_set(struct bt_mesh_light_hsl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_hue_sat_range *set,
				struct bt_mesh_light_hsl_range_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_RANGE_SET,
				 BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HSL_OP_RANGE_SET);
	light_hue_sat_range_buf_push(&msg, set);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHT_HSL_OP_RANGE_STATUS, rsp);
}

int bt_mesh_light_hsl_range_set_unack(
	struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_hue_sat_range *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_RANGE_SET_UNACK,
				 BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HSL_OP_RANGE_SET_UNACK);
	light_hue_sat_range_buf_push(&msg, set);

	return model_send(cli->model, ctx, &msg);
}

int bt_mesh_light_hue_get(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_hue_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_HUE_OP_GET,
		       BT_MESH_LIGHT_HUE_OP_STATUS);
}

int bt_mesh_light_hue_set(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_hue *set,
			  struct bt_mesh_light_hue_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HUE_OP_SET,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_HUE);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HUE_OP_SET);
	net_buf_simple_add_le16(&msg, set->lvl);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHT_HUE_OP_STATUS, rsp);
}

int bt_mesh_light_hue_set_unack(struct bt_mesh_light_hsl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_hue *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HUE_OP_SET_UNACK,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_HUE);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HUE_OP_SET_UNACK);
	net_buf_simple_add_le16(&msg, set->lvl);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_send(cli->model, ctx, &msg);
}

int bt_mesh_light_saturation_get(struct bt_mesh_light_hsl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_light_sat_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_LIGHT_SAT_OP_GET,
		       BT_MESH_LIGHT_SAT_OP_STATUS);
}

int bt_mesh_light_saturation_set(struct bt_mesh_light_hsl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 const struct bt_mesh_light_sat *set,
				 struct bt_mesh_light_sat_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_SAT_OP_SET,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_HUE);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_SAT_OP_SET);
	net_buf_simple_add_le16(&msg, set->lvl);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHT_SAT_OP_STATUS, rsp);
}

int bt_mesh_light_saturation_set_unack(struct bt_mesh_light_hsl_cli *cli,
				       struct bt_mesh_msg_ctx *ctx,
				       const struct bt_mesh_light_sat *set)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_SAT_OP_SET_UNACK,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_HUE);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_SAT_OP_SET_UNACK);
	net_buf_simple_add_le16(&msg, set->lvl);
	net_buf_simple_add_u8(&msg, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&msg, set->transition);
	}

	return model_send(cli->model, ctx, &msg);
}
