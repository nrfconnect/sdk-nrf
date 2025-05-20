/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/vnd/dm_cli.h>
#include "model_utils.h"
#include <dm.h>

static int handle_cfg_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_dm_cli *cli = model->rt->user_data;
	struct bt_mesh_dm_cli_cfg_status status;
	struct bt_mesh_dm_cli_cfg_status *rsp;
	uint8_t temp;

	status.status = net_buf_simple_pull_u8(buf);
	temp = net_buf_simple_pull_u8(buf);
	status.is_in_progress = temp >> 7;
	status.result_entry_cnt = temp & 0x7F;
	status.def.delay = net_buf_simple_pull_u8(buf);
	status.def.ttl = net_buf_simple_pull_u8(buf);
	status.def.timeout = net_buf_simple_pull_u8(buf);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_DM_CONFIG_STATUS_OP,
				      ctx->addr, (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->cfg_status_handler) {
		cli->handlers->cfg_status_handler(cli, ctx, &status);
	}

	return 0;
}

static bool result_populate(struct bt_mesh_dm_res_entry *entry,
			    struct net_buf_simple *buf)
{
	uint8_t temp = net_buf_simple_pull_u8(buf);

	entry->mode = (temp >> 7) & 0x01;
	entry->quality = (temp >> 4) & 0x07;
	entry->err_occurred = (temp >> 3) & 0x01;
	entry->addr = net_buf_simple_pull_le16(buf);

	if (entry->err_occurred) {
		return true;
	}

	if (entry->mode == DM_RANGING_MODE_RTT && buf->len >= 2) {
		entry->res.rtt = net_buf_simple_pull_le16(buf);
	} else if (entry->mode == DM_RANGING_MODE_MCPD && buf->len >= 8) {
		entry->res.mcpd.best = net_buf_simple_pull_le16(buf);
		entry->res.mcpd.ifft = net_buf_simple_pull_le16(buf);
		entry->res.mcpd.phase_slope = net_buf_simple_pull_le16(buf);
		entry->res.mcpd.rssi_openspace = net_buf_simple_pull_le16(buf);
	} else {
		return false;
	}

	return true;
}

static int handle_result_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_dm_cli *cli = model->rt->user_data;
	struct bt_mesh_dm_cli_results status = {
		.res = cli->res_arr,
		.entry_cnt = 0
	};
	struct bt_mesh_dm_cli_results *rsp;

	status.status = net_buf_simple_pull_u8(buf);

	if (!status.status) {
		memset(cli->res_arr, 0, sizeof(struct bt_mesh_dm_res_entry) * cli->entry_cnt);

		for (int i = 0; i < cli->entry_cnt; i++) {
			if (buf->len < 3) {
				break;
			}

			if (!result_populate(&cli->res_arr[i], buf)) {
				break;
			}
			status.entry_cnt++;
		}
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_DM_RESULT_STATUS_OP,
				      ctx->addr, (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->result_handler) {
		cli->handlers->result_handler(cli, ctx, &status);
	}

	return 0;
}

static int handle_msg(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	uint8_t opcode = net_buf_simple_pull_u8(buf);

	switch (opcode) {
	case BT_MESH_DM_CONFIG_STATUS_OP:
		return handle_cfg_status(model, ctx, buf);
	case BT_MESH_DM_RESULT_STATUS_OP:
		return handle_result_status(model, ctx, buf);
	}

	return -EOPNOTSUPP;
}

const struct bt_mesh_model_op _bt_mesh_dm_cli_op[] = {
	{
		BT_MESH_DM_CLI_OP,
		BT_MESH_LEN_MIN(BT_MESH_DM_RESULT_STATUS_MSG_MIN_LEN),
		handle_msg,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_dm_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_dm_cli *cli = model->rt->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data, sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_dm_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_dm_cli *cli = model->rt->user_data;

	net_buf_simple_reset(cli->pub.msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_dm_cli_cb = {
	.init = bt_mesh_dm_cli_init,
	.reset = bt_mesh_dm_cli_reset,
};

int bt_mesh_dm_cli_config(struct bt_mesh_dm_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_dm_cfg *set,
			  struct bt_mesh_dm_cli_cfg_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DM_SRV_OP,
				 BT_MESH_DM_CONFIG_MSG_LEN_MAX);
	bt_mesh_model_msg_init(&msg, BT_MESH_DM_SRV_OP);

	net_buf_simple_add_u8(&msg, BT_MESH_DM_CONFIG_OP);

	if (set) {
		if ((set->ttl == 1 || set->ttl > 127) || set->timeout < 10) {
			return -EBADMSG;
		}

		net_buf_simple_add_u8(&msg, set->ttl);
		net_buf_simple_add_u8(&msg, set->timeout);
		net_buf_simple_add_u8(&msg, set->delay);
	}

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_DM_CONFIG_STATUS_OP,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_dm_cli_measurement_start(struct bt_mesh_dm_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_dm_cli_start *start,
				     struct bt_mesh_dm_cli_results *rsp)
{
	if (!BT_MESH_ADDR_IS_UNICAST(start->addr)) {
		return -EBADMSG;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DM_SRV_OP, BT_MESH_DM_START_MSG_LEN_MAX);
	bt_mesh_model_msg_init(&msg, BT_MESH_DM_SRV_OP);

	net_buf_simple_add_u8(&msg, BT_MESH_DM_START_OP);
	net_buf_simple_add_le16(&msg, (start->mode << 15) | (start->addr & 0x7FFF));
	net_buf_simple_add_u8(&msg, start->reuse_transaction ? cli->tid : ++cli->tid);
	if (start->cfg) {
		if ((start->cfg->ttl == 1 || start->cfg->ttl > 127) || start->cfg->timeout < 10) {
			return -EBADMSG;
		}

		net_buf_simple_add_u8(&msg, start->cfg->ttl);
		net_buf_simple_add_u8(&msg, start->cfg->timeout);
		net_buf_simple_add_u8(&msg, start->cfg->delay);
	}

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_DM_RESULT_STATUS_OP,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_dm_cli_results_get(struct bt_mesh_dm_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       uint8_t entry_cnt,
			       struct bt_mesh_dm_cli_results *rsp)
{
	if (entry_cnt > cli->entry_cnt) {
		return -EBADMSG;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DM_SRV_OP, BT_MESH_DM_RESULT_GET_MSG_LEN);
	bt_mesh_model_msg_init(&msg, BT_MESH_DM_SRV_OP);

	net_buf_simple_add_u8(&msg, BT_MESH_DM_RESULT_GET_OP);
	net_buf_simple_add_u8(&msg, entry_cnt);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_DM_RESULT_STATUS_OP,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}
