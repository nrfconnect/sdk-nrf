/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <sys/byteorder.h>
#include <bluetooth/mesh/models.h>
#include "scheduler_internal.h"
#include "model_utils.h"

static void status_op(struct bt_mesh_model *mod,
		      struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	struct bt_mesh_scheduler_cli *cli = mod->user_data;
	uint16_t schedules;

	if (buf->len != BT_MESH_SCHEDULER_MSG_LEN_STATUS) {
		return;
	}

	schedules = net_buf_simple_pull_le16(buf);

	if (model_ack_match(&cli->ack, BT_MESH_SCHEDULER_OP_STATUS, ctx)) {
		uint16_t *rsp = cli->ack.user_data;
		*rsp = schedules;
		model_ack_rx(&cli->ack);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, schedules);
	}
}

static void action_status_op(struct bt_mesh_model *mod,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_scheduler_cli *cli = mod->user_data;
	struct bt_mesh_schedule_entry action = {0};
	uint8_t idx;

	if (buf->len != BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS &&
	    buf->len !=	BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS_REDUCED) {
		return;
	}

	if (buf->len == BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS) {
		scheduler_action_unpack(buf, &idx, &action);
	} else {
		idx = net_buf_simple_pull_u8(buf);
	}

	if (model_ack_match(&cli->ack,
			BT_MESH_SCHEDULER_OP_ACTION_STATUS, ctx) &&
	    cli->ack_idx == idx) {
		struct bt_mesh_schedule_entry *rsp = cli->ack.user_data;
		*rsp = action;
		cli->ack_idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
		model_ack_rx(&cli->ack);
	}

	if (cli->action_status_handler) {
		cli->action_status_handler(cli, ctx, idx,
			buf->len == BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS ?
				&action : NULL);
	}
}

const struct bt_mesh_model_op _bt_mesh_scheduler_cli_op[] = {
	{
		BT_MESH_SCHEDULER_OP_STATUS,
		BT_MESH_SCHEDULER_MSG_LEN_STATUS,
		status_op,
	},
	{
		BT_MESH_SCHEDULER_OP_ACTION_STATUS,
		BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS_REDUCED,
		action_status_op,
	},
	BT_MESH_MODEL_OP_END,
};

static int scheduler_cli_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_scheduler_cli *cli = mod->user_data;

	if (!cli) {
		return -EINVAL;
	}

	cli->mod = mod;

	net_buf_simple_init_with_data(&cli->pub_msg, cli->buf,
				      sizeof(cli->buf));
	cli->pub.msg = &cli->pub_msg;
	cli->ack_idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	model_ack_init(&cli->ack);

	return 0;
}

static void scheduler_cli_reset(struct bt_mesh_model *mod)
{
	struct bt_mesh_scheduler_cli *cli = mod->user_data;

	net_buf_simple_reset(cli->pub.msg);
	model_ack_reset(&cli->ack);
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

	return model_ackd_send(cli->mod, ctx, &buf, rsp ? &cli->ack : NULL,
			BT_MESH_SCHEDULER_OP_STATUS, rsp);
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

	return model_ackd_send(cli->mod, ctx, &buf, rsp ? &cli->ack : NULL,
			BT_MESH_SCHEDULER_OP_ACTION_STATUS, rsp);
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

	if (entry == NULL || idx >= BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		return -EINVAL;
	}

	scheduler_action_pack(&buf, idx, entry);
	cli->ack_idx = rsp ? idx : BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;

	return model_ackd_send(cli->mod, ctx, &buf, rsp ? &cli->ack : NULL,
			BT_MESH_SCHEDULER_OP_ACTION_STATUS, rsp);
}

int bt_mesh_scheduler_cli_action_set_unack(struct bt_mesh_scheduler_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				uint8_t idx,
				const struct bt_mesh_schedule_entry *entry)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK,
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK);

	if (entry == NULL || idx >= BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		return -EINVAL;
	}

	scheduler_action_pack(&buf, idx, entry);

	return model_send(cli->mod, ctx, &buf);
}
