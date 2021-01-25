/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <bluetooth/mesh/lightness_cli.h>
#include "model_utils.h"
#include "lightness_internal.h"

static void light_status(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf, enum light_repr repr)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_MINLEN_STATUS &&
	    buf->len != BT_MESH_LIGHTNESS_MSG_MAXLEN_STATUS) {
		return;
	}

	struct bt_mesh_lightness_cli *cli = mod->user_data;
	struct bt_mesh_lightness_status status;

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

	if (model_ack_match(&cli->ack_ctx,
			    op_get(LIGHTNESS_OP_TYPE_STATUS, repr), ctx)) {
		struct bt_mesh_lightness_status *rsp = cli->ack_ctx.user_data;
		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->light_status) {
		cli->handlers->light_status(cli, ctx, &status);
	}
}

static void handle_light_status(struct bt_mesh_model *mod,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	light_status(mod, ctx, buf, ACTUAL);
}

static void handle_light_linear_status(struct bt_mesh_model *mod,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	light_status(mod, ctx, buf, LINEAR);
}

static void handle_last_status(struct bt_mesh_model *mod,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_LEN_LAST_STATUS) {
		return;
	}

	struct bt_mesh_lightness_cli *cli = mod->user_data;
	uint16_t last = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LIGHTNESS_OP_LAST_STATUS, ctx)) {
		uint16_t *rsp = cli->ack_ctx.user_data;
		*rsp = last;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->last_light_status) {
		cli->handlers->last_light_status(cli, ctx, last);
	}
}

static void handle_default_status(struct bt_mesh_model *mod,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_STATUS) {
		return;
	}

	struct bt_mesh_lightness_cli *cli = mod->user_data;
	uint16_t default_lvl =
		repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS, ctx)) {
		uint16_t *rsp = cli->ack_ctx.user_data;
		*rsp = default_lvl;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->default_status) {
		cli->handlers->default_status(cli, ctx, default_lvl);
	}
}

static void handle_range_status(struct bt_mesh_model *mod,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_LEN_RANGE_STATUS) {
		return;
	}

	struct bt_mesh_lightness_cli *cli = mod->user_data;
	struct bt_mesh_lightness_range_status status;

	status.status = net_buf_simple_pull_u8(buf);
	status.range.min = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);
	status.range.max = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_LIGHTNESS_OP_RANGE_STATUS, ctx)) {
		struct bt_mesh_lightness_range_status *rsp =
			cli->ack_ctx.user_data;
		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->range_status) {
		cli->handlers->range_status(cli, ctx, &status);
	}
}

const struct bt_mesh_model_op _bt_mesh_lightness_cli_op[] = {
	{ BT_MESH_LIGHTNESS_OP_STATUS, BT_MESH_LIGHTNESS_MSG_MINLEN_STATUS,
	  handle_light_status },
	{ BT_MESH_LIGHTNESS_OP_LINEAR_STATUS, BT_MESH_LIGHTNESS_MSG_MINLEN_STATUS,
	  handle_light_linear_status },
	{ BT_MESH_LIGHTNESS_OP_LAST_STATUS,
	  BT_MESH_LIGHTNESS_MSG_LEN_LAST_STATUS, handle_last_status },
	{ BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS,
	  BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_STATUS, handle_default_status },
	{ BT_MESH_LIGHTNESS_OP_RANGE_STATUS,
	  BT_MESH_LIGHTNESS_MSG_LEN_RANGE_STATUS, handle_range_status },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_lvl_cli_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_lightness_cli *cli = mod->user_data;

	cli->model = mod;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	model_ack_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_lvl_cli_reset(struct bt_mesh_model *mod)
{
	struct bt_mesh_lightness_cli *cli = mod->user_data;

	net_buf_simple_reset(mod->pub->msg);
	model_ack_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_lightness_cli_cb = {
	.init = bt_mesh_lvl_cli_init,
	.reset = bt_mesh_lvl_cli_reset,
};

int lightness_cli_light_get(struct bt_mesh_lightness_cli *cli,
			    struct bt_mesh_msg_ctx *ctx, enum light_repr repr,
			    struct bt_mesh_lightness_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_GET,
				 BT_MESH_LIGHTNESS_MSG_LEN_GET);
	bt_mesh_model_msg_init(&buf, op_get(LIGHTNESS_OP_TYPE_GET, repr));

	return model_ackd_send(cli->model, ctx, &buf,
			       rsp ? &cli->ack_ctx : NULL,
			       op_get(LIGHTNESS_OP_TYPE_STATUS, repr), rsp);
}

int lightness_cli_light_set(struct bt_mesh_lightness_cli *cli,
			    struct bt_mesh_msg_ctx *ctx, enum light_repr repr,
			    const struct bt_mesh_lightness_set *set,
			    struct bt_mesh_lightness_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_SET,
				 BT_MESH_LIGHTNESS_MSG_MAXLEN_SET);
	bt_mesh_model_msg_init(&buf, op_get(LIGHTNESS_OP_TYPE_SET, repr));

	net_buf_simple_add_le16(&buf, set->lvl);
	net_buf_simple_add_u8(&buf, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&buf, set->transition);
	}

	return model_ackd_send(cli->model, ctx, &buf,
			       rsp ? &cli->ack_ctx : NULL,
			       op_get(LIGHTNESS_OP_TYPE_STATUS, repr), rsp);
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

	return model_send(cli->model, ctx, &buf);
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

	return model_ackd_send(cli->model, ctx, &buf,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHTNESS_OP_RANGE_STATUS, rsp);
}

int bt_mesh_lightness_cli_range_set(struct bt_mesh_lightness_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_lightness_range *range,
				    struct bt_mesh_lightness_range_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_RANGE_SET,
				 BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_RANGE_SET);
	net_buf_simple_add_le16(&buf, light_to_repr(range->min, ACTUAL));
	net_buf_simple_add_le16(&buf, light_to_repr(range->max, ACTUAL));

	return model_ackd_send(cli->model, ctx, &buf,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHTNESS_OP_RANGE_STATUS, rsp);
}

int bt_mesh_lightness_cli_range_set_unack(
	struct bt_mesh_lightness_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_lightness_range *range)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_RANGE_SET_UNACK,
				 BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_RANGE_SET_UNACK);
	net_buf_simple_add_le16(&buf, light_to_repr(range->min, ACTUAL));
	net_buf_simple_add_le16(&buf, light_to_repr(range->max, ACTUAL));

	return model_send(cli->model, ctx, &buf);
}

int bt_mesh_lightness_cli_default_get(struct bt_mesh_lightness_cli *cli,
				      struct bt_mesh_msg_ctx *ctx, uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_DEFAULT_GET,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_DEFAULT_GET);

	return model_ackd_send(cli->model, ctx, &buf,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS, rsp);
}

int bt_mesh_lightness_cli_default_set(struct bt_mesh_lightness_cli *cli,
				      struct bt_mesh_msg_ctx *ctx,
				      uint16_t default_light, uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_DEFAULT_SET,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_DEFAULT_SET);
	net_buf_simple_add_le16(&buf, light_to_repr(default_light, ACTUAL));

	return model_ackd_send(cli->model, ctx, &buf,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS, rsp);
}

int bt_mesh_lightness_cli_default_set_unack(struct bt_mesh_lightness_cli *cli,
					    struct bt_mesh_msg_ctx *ctx,
					    uint16_t default_light)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_DEFAULT_SET_UNACK,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_DEFAULT_SET_UNACK);
	net_buf_simple_add_le16(&buf, light_to_repr(default_light, ACTUAL));

	return model_send(cli->model, ctx, &buf);
}

int bt_mesh_lightness_cli_last_get(struct bt_mesh_lightness_cli *cli,
				   struct bt_mesh_msg_ctx *ctx, uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHTNESS_OP_LAST_GET,
				 BT_MESH_LIGHTNESS_MSG_LEN_LAST_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHTNESS_OP_LAST_GET);

	return model_ackd_send(cli->model, ctx, &buf,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_LIGHTNESS_OP_LAST_STATUS, rsp);
}
