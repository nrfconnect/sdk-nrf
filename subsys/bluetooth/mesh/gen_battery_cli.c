/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <bluetooth/mesh/gen_battery_cli.h>
#include "gen_battery_internal.h"
#include "model_utils.h"

int bt_mesh_gen_bat_decode_status(struct net_buf_simple *buf,
				  struct bt_mesh_battery_status *status)
{
	uint8_t battery_lvl = net_buf_simple_pull_u8(buf);

	if ((battery_lvl > 100) &&
	    (battery_lvl != BT_MESH_BATTERY_LVL_UNKNOWN)) {
		return -EINVAL;
	}

	status->battery_lvl = battery_lvl;

	uint8_t *discharge_minutes = net_buf_simple_pull_mem(buf, 3);

	status->discharge_minutes =
		((discharge_minutes[2] << 16) | (discharge_minutes[1] << 8) |
		 (discharge_minutes[0]));

	uint8_t *charge_minutes = net_buf_simple_pull_mem(buf, 3);

	status->charge_minutes =
		((charge_minutes[2] << 16) | (charge_minutes[1] << 8) |
		 (charge_minutes[0]));

	uint8_t flags = net_buf_simple_pull_u8(buf);

	status->presence = flags & BIT_MASK(2);
	status->indicator = (flags >> 2) & BIT_MASK(2);
	status->charging = (flags >> 4) & BIT_MASK(2);
	status->service = (flags >> 6) & BIT_MASK(2);

	return 0;
}

static int handle_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct bt_mesh_battery_cli *cli = model->user_data;
	struct bt_mesh_battery_status status;
	struct bt_mesh_battery_status *rsp;
	int err;

	err = bt_mesh_gen_bat_decode_status(buf, &status);
	if (err) {
		return err;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_BATTERY_OP_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, &status);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_battery_cli_op[] = {
	{
		BT_MESH_BATTERY_OP_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_BATTERY_MSG_LEN_STATUS),
		handle_status,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_battery_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_battery_cli *cli = model->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_battery_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_battery_cli *cli = model->user_data;

	net_buf_simple_reset(model->pub->msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_battery_cli_cb = {
	.init = bt_mesh_battery_cli_init,
	.reset = bt_mesh_battery_cli_reset,
};

int bt_mesh_battery_cli_get(struct bt_mesh_battery_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_battery_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_BATTERY_OP_GET,
				 BT_MESH_BATTERY_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_BATTERY_OP_GET);
	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_BATTERY_OP_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}
