/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <bluetooth/mesh/gen_battery_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "model_utils.h"

#define BATTERY_STATUS_DEFAULT                                                 \
	{                                                                      \
		.battery_lvl = BT_MESH_BATTERY_LVL_UNKNOWN,                    \
		.discharge_minutes = BT_MESH_BATTERY_TIME_UNKNOWN,             \
		.charge_minutes = BT_MESH_BATTERY_TIME_UNKNOWN,                \
		.presence = BT_MESH_BATTERY_PRESENCE_UNKNOWN,                  \
		.indicator = BT_MESH_BATTERY_INDICATOR_UNKNOWN,                \
		.charging = BT_MESH_BATTERY_CHARGING_UNKNOWN,                  \
		.service = BT_MESH_BATTERY_SERVICE_UNKNOWN,                    \
	}

static void encode_status(struct net_buf_simple *buf,
			  const struct bt_mesh_battery_status *status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_BATTERY_OP_STATUS);
	net_buf_simple_add_u8(buf, status->battery_lvl);

	u8_t *discharge = net_buf_simple_add(buf, 3);

	discharge[0] = status->discharge_minutes;
	discharge[1] = status->discharge_minutes >> 8;
	discharge[2] = status->discharge_minutes >> 16;

	u8_t *charge = net_buf_simple_add(buf, 3);

	charge[0] = status->charge_minutes;
	charge[1] = status->charge_minutes >> 8;
	charge[2] = status->charge_minutes >> 16;

	u8_t flags = ((status->presence & BIT_MASK(2)) |
		      ((status->indicator & BIT_MASK(2)) << 2) |
		      ((status->charging & BIT_MASK(2)) << 4) |
		      ((status->service & BIT_MASK(2)) << 6));

	net_buf_simple_add_u8(buf, flags);
}

static void rsp_status(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *rx_ctx,
		       const struct bt_mesh_battery_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_BATTERY_OP_STATUS,
				 BT_MESH_BATTERY_MSG_LEN_STATUS);
	encode_status(&msg, status);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void handle_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	struct bt_mesh_battery_srv *srv = model->user_data;
	struct bt_mesh_battery_status status = BATTERY_STATUS_DEFAULT;

	srv->get(srv, ctx, &status);

	rsp_status(model, ctx, &status);
}

const struct bt_mesh_model_op _bt_mesh_battery_srv_op[] = {
	{ BT_MESH_BATTERY_OP_GET, BT_MESH_BATTERY_MSG_LEN_GET, handle_get },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_battery_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_battery_srv *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(model->pub->msg, 0);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_battery_srv_cb = {
	.init = bt_mesh_battery_srv_init
};

int _bt_mesh_battery_srv_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_battery_srv *srv = model->user_data;
	struct bt_mesh_battery_status status = BATTERY_STATUS_DEFAULT;

	srv->get(srv, NULL, &status);

	encode_status(model->pub->msg, &status);

	return 0;
}

s32_t bt_mesh_battery_srv_pub(struct bt_mesh_battery_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_battery_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_BATTERY_OP_STATUS,
				 BT_MESH_BATTERY_MSG_LEN_STATUS);
	encode_status(&msg, status);
	return model_send(srv->model, ctx, &msg);
}
