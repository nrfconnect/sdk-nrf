/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/time_cli.h>
#include "time_internal.h"
#include "model_utils.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_time_cli);

static int handle_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_time_cli *cli = model->user_data;
	struct bt_mesh_time_status status;
	struct bt_mesh_time_status *rsp;

	bt_mesh_time_decode_time_params(buf, &status);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_TIME_OP_TIME_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->time_status) {
		cli->handlers->time_status(cli, ctx, &status);
	}

	return 0;
}

static int handle_time_role_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	struct bt_mesh_time_cli *cli = model->user_data;
	enum bt_mesh_time_role status;
	uint8_t *rsp;

	status = net_buf_simple_pull_u8(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_TIME_OP_TIME_ROLE_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->time_role_status) {
		cli->handlers->time_role_status(cli, ctx, status);
	}

	return 0;
}

static int handle_time_zone_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	struct bt_mesh_time_cli *cli = model->user_data;
	struct bt_mesh_time_zone_status status;
	struct bt_mesh_time_zone_status *rsp;

	status.current_offset =
		net_buf_simple_pull_u8(buf) - ZONE_CHANGE_ZERO_POINT;
	status.time_zone_change.new_offset =
		net_buf_simple_pull_u8(buf) - ZONE_CHANGE_ZERO_POINT;
	status.time_zone_change.timestamp = bt_mesh_time_buf_pull_tai_sec(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_TIME_OP_TIME_ZONE_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->time_zone_status) {
		cli->handlers->time_zone_status(cli, ctx, &status);
	}

	return 0;
}

static int handle_tai_utc_delta_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	struct bt_mesh_time_cli *cli = model->user_data;
	struct bt_mesh_time_tai_utc_delta_status status;
	struct bt_mesh_time_tai_utc_delta_status *rsp;

	status.delta_current =
		net_buf_simple_pull_le16(buf) - UTC_CHANGE_ZERO_POINT;
	status.tai_utc_change.delta_new =
		net_buf_simple_pull_le16(buf) - UTC_CHANGE_ZERO_POINT;
	status.tai_utc_change.timestamp = bt_mesh_time_buf_pull_tai_sec(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_TIME_OP_TAI_UTC_DELTA_STATUS,
				      ctx->addr, (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->tai_utc_delta_status) {
		cli->handlers->tai_utc_delta_status(cli, ctx, &status);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_time_cli_op[] = {
	{
		BT_MESH_TIME_OP_TIME_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_TIME_MSG_MINLEN_TIME_STATUS),
		handle_status,
	},
	{
		BT_MESH_TIME_OP_TIME_ROLE_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_TIME_ROLE_STATUS),
		handle_time_role_status,
	},
	{
		BT_MESH_TIME_OP_TIME_ZONE_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_TIME_ZONE_STATUS),
		handle_time_zone_status,
	},
	{
		BT_MESH_TIME_OP_TAI_UTC_DELTA_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_TAI_UTC_DELTA_STATUS),
		handle_tai_utc_delta_status,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_time_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_time_cli *cli = model->user_data;

	cli->model = model;
	net_buf_simple_init(cli->pub.msg, 0);
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	if (bt_mesh_model_find(bt_mesh_model_elem(model), BT_MESH_MODEL_ID_TIME_SRV)) {
		LOG_ERR("Time server and time client can not share the same element");
		return -EINVAL;
	}

	return 0;
}

static void bt_mesh_time_cli_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_time_cli *cli = model->user_data;

	net_buf_simple_reset(cli->pub.msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_time_cli_cb = {
	.init = bt_mesh_time_cli_init,
	.reset = bt_mesh_time_cli_reset,
};

static int get_msg(struct bt_mesh_time_cli *cli, struct bt_mesh_msg_ctx *ctx,
		   void *rsp, uint16_t opcode, uint16_t ret_opcode)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, opcode, BT_MESH_TIME_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, opcode);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = ret_opcode,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_time_cli_time_get(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_time_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_TIME_OP_TIME_GET,
		       BT_MESH_TIME_OP_TIME_STATUS);
}

int bt_mesh_time_cli_time_set(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_time_status *set,
			      struct bt_mesh_time_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_SET,
				 BT_MESH_TIME_MSG_LEN_TIME_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TIME_SET);
	bt_mesh_time_encode_time_params(&msg, set);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_TIME_OP_TIME_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_time_cli_zone_get(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_time_zone_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_TIME_OP_TIME_ZONE_GET,
		       BT_MESH_TIME_OP_TIME_ZONE_STATUS);
}

int bt_mesh_time_cli_zone_set(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_time_zone_change *set,
			      struct bt_mesh_time_zone_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_ZONE_SET,
				 BT_MESH_TIME_MSG_LEN_TIME_ZONE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TIME_ZONE_SET);
	net_buf_simple_add_u8(&msg,
			      (uint8_t)(set->new_offset + ZONE_CHANGE_ZERO_POINT));
	bt_mesh_time_buf_put_tai_sec(&msg, set->timestamp);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_TIME_OP_TIME_ZONE_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_time_cli_tai_utc_delta_get(
	struct bt_mesh_time_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_time_tai_utc_delta_status *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_TIME_OP_TAI_UTC_DELTA_GET,
		       BT_MESH_TIME_OP_TAI_UTC_DELTA_STATUS);
}

int bt_mesh_time_cli_tai_utc_delta_set(
	struct bt_mesh_time_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_time_tai_utc_change *set,
	struct bt_mesh_time_tai_utc_delta_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TAI_UTC_DELTA_SET,
				 BT_MESH_TIME_MSG_LEN_TAI_UTC_DELTA_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TAI_UTC_DELTA_SET);

	net_buf_simple_add_le16(
		&msg, (uint16_t)(set->delta_new + UTC_CHANGE_ZERO_POINT));
	bt_mesh_time_buf_put_tai_sec(&msg, set->timestamp);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_TIME_OP_TAI_UTC_DELTA_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_time_cli_role_get(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx, uint8_t *rsp)
{
	return get_msg(cli, ctx, rsp, BT_MESH_TIME_OP_TIME_ROLE_GET,
		       BT_MESH_TIME_OP_TIME_ROLE_STATUS);
}

int bt_mesh_time_cli_role_set(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx, const uint8_t *set,
			      uint8_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_ROLE_SET,
				 BT_MESH_TIME_MSG_LEN_TIME_ROLE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TIME_ROLE_SET);
	net_buf_simple_add_u8(&msg, *set);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_TIME_OP_TIME_ROLE_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}
