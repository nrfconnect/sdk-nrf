/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <bluetooth/mesh/gen_battery_cli.h>
#include "model_utils.h"

static void decode_status(struct net_buf_simple *buf,
			  struct bt_mesh_battery_status *status)
{
	status->battery_lvl = net_buf_simple_pull_le16(buf);

	u8_t *discharge_minutes = net_buf_simple_pull_mem(buf, 3);

	status->discharge_minutes =
		((discharge_minutes[0] >> 16) | (discharge_minutes[1] >> 8) |
		 (discharge_minutes[2]));

	u8_t *charge_minutes = net_buf_simple_pull_mem(buf, 3);

	status->charge_minutes =
		((charge_minutes[0] >> 16) | (charge_minutes[1] >> 8) |
		 (charge_minutes[2]));

	u8_t flags = net_buf_simple_pull_u8(buf);

	status->presence = flags & BIT_MASK(2);
	status->indicator = (flags >> 2) & BIT_MASK(2);
	status->charging = (flags >> 4) & BIT_MASK(2);
	status->service = (flags >> 6) & BIT_MASK(2);
}

static void handle_status(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_battery_cli *cli = model->user_data;
	struct bt_mesh_battery_status status;

	decode_status(buf, &status);

	if (model_ack_match(&cli->ack_ctx, BT_MESH_BATTERY_OP_STATUS, ctx)) {
		struct bt_mesh_battery_status *rsp =
			(struct bt_mesh_battery_status *)cli->ack_ctx.user_data;

		*rsp = status;
		model_ack_rx(&cli->ack_ctx);
	}

	if (cli->status_handler) {
		cli->status_handler(cli, ctx, &status);
	}
}

const struct bt_mesh_model_op _bt_mesh_battery_cli_op[] = {
	{ BT_MESH_BATTERY_OP_STATUS, BT_MESH_BATTERY_MSG_LEN_STATUS,
	  handle_status },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_battery_cli_init(struct bt_mesh_model *model)
{
	struct bt_mesh_battery_cli *cli = model->user_data;

	cli->model = model;
	net_buf_simple_init(model->pub->msg, 0);
	model_ack_init(&cli->ack_ctx);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_battery_cli_cb = {
	.init = bt_mesh_battery_cli_init,
};

int bt_mesh_battery_cli_status_get(struct bt_mesh_battery_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_battery_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_BATTERY_OP_GET,
				 BT_MESH_BATTERY_MSG_LEN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_BATTERY_OP_GET);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_BATTERY_OP_STATUS, rsp);
}
