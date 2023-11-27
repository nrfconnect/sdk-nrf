/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/sys/byteorder.h>
#include <bluetooth/mesh/models.h>
#include "scheduler_internal.h"
#include "model_utils.h"

static int handle_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct bt_mesh_scheduler_cli *cli = model->rt->user_data;
	uint16_t schedules;
	uint16_t *rsp;

	schedules = net_buf_simple_pull_le16(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_SCHEDULER_OP_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = schedules;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, schedules);
	}

	return 0;
}

static int handle_action_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_scheduler_cli *cli = model->rt->user_data;
	struct bt_mesh_schedule_entry action_data = {0};
	struct bt_mesh_schedule_entry *action = NULL;
	struct bt_mesh_schedule_entry *rsp;
	uint8_t idx;

	if (buf->len != BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS &&
	    buf->len !=	BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS_REDUCED) {
		return -EMSGSIZE;
	}

	if (buf->len == BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS) {
		scheduler_action_unpack(buf, &idx, &action_data);
		action = &action_data;
	} else {
		idx = net_buf_simple_pull_u8(buf);
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_SCHEDULER_OP_ACTION_STATUS,
				      ctx->addr, (void **)&rsp) &&
	    cli->ack_idx == idx) {
		*rsp = action_data;
		cli->ack_idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->action_status_handler) {
		cli->action_status_handler(cli, ctx, idx, action);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_scheduler_cli_op[] = {
	{
		BT_MESH_SCHEDULER_OP_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_SCHEDULER_MSG_LEN_STATUS),
		handle_status,
	},
	{
		BT_MESH_SCHEDULER_OP_ACTION_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS_REDUCED),
		handle_action_status,
	},
	BT_MESH_MODEL_OP_END,
};

static int scheduler_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_scheduler_cli *cli = model->rt->user_data;

	if (!cli) {
		return -EINVAL;
	}

	cli->model = model;

	net_buf_simple_init_with_data(&cli->pub_msg, cli->buf,
				      sizeof(cli->buf));
	cli->pub.msg = &cli->pub_msg;
	cli->ack_idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void scheduler_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_scheduler_cli *cli = model->rt->user_data;

	net_buf_simple_reset(cli->pub.msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_scheduler_cli_cb = {
	.init = scheduler_cli_init,
	.reset = scheduler_cli_reset,
};

int bt_mesh_scheduler_cli_get(struct bt_mesh_scheduler_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      uint16_t *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_GET,
			BT_MESH_SCHEDULER_MSG_LEN_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCHEDULER_OP_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_SCHEDULER_OP_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_scheduler_cli_action_get(struct bt_mesh_scheduler_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				uint8_t idx,
				struct bt_mesh_schedule_entry *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_GET,
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCHEDULER_OP_ACTION_GET);

	if (idx >= BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		return -EINVAL;
	}

	net_buf_simple_add_u8(&buf, idx);
	cli->ack_idx = rsp ? idx : BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_SCHEDULER_OP_ACTION_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_scheduler_cli_action_set(struct bt_mesh_scheduler_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				uint8_t idx,
				const struct bt_mesh_schedule_entry *entry,
				struct bt_mesh_schedule_entry *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_SET,
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCHEDULER_OP_ACTION_SET);

	if (!scheduler_action_valid(entry, idx)) {
		return -EINVAL;
	}

	scheduler_action_pack(&buf, idx, entry);
	cli->ack_idx = rsp ? idx : BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_SCHEDULER_OP_ACTION_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_scheduler_cli_action_set_unack(struct bt_mesh_scheduler_cli *cli,
					   struct bt_mesh_msg_ctx *ctx, uint8_t idx,
					   const struct bt_mesh_schedule_entry *entry)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK,
				 BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK);

	if (!scheduler_action_valid(entry, idx)) {
		return -EINVAL;
	}

	scheduler_action_pack(&buf, idx, entry);

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}
