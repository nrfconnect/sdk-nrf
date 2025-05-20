/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <bluetooth/mesh/gen_battery_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "gen_battery_internal.h"
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

void bt_mesh_gen_bat_encode_status(struct net_buf_simple *buf,
				   const struct bt_mesh_battery_status *status)
{
	net_buf_simple_add_u8(buf, status->battery_lvl);

	uint8_t *discharge = net_buf_simple_add(buf, 3);

	discharge[0] = status->discharge_minutes >> 16;
	discharge[1] = status->discharge_minutes >> 8;
	discharge[2] = status->discharge_minutes;

	uint8_t *charge = net_buf_simple_add(buf, 3);

	charge[0] = status->charge_minutes >> 16;
	charge[1] = status->charge_minutes >> 8;
	charge[2] = status->charge_minutes;

	uint8_t flags = ((status->presence & BIT_MASK(2)) |
		      ((status->indicator & BIT_MASK(2)) << 2) |
		      ((status->charging & BIT_MASK(2)) << 4) |
		      ((status->service & BIT_MASK(2)) << 6));

	net_buf_simple_add_u8(buf, flags);
}

static void rsp_status(const struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *rx_ctx,
		       const struct bt_mesh_battery_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_BATTERY_OP_STATUS,
				 BT_MESH_BATTERY_MSG_LEN_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_BATTERY_OP_STATUS);
	bt_mesh_gen_bat_encode_status(&msg, status);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static int handle_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	struct bt_mesh_battery_srv *srv = model->rt->user_data;
	struct bt_mesh_battery_status status = BATTERY_STATUS_DEFAULT;

	srv->get(srv, ctx, &status);

	rsp_status(model, ctx, &status);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_battery_srv_op[] = {
	{
		BT_MESH_BATTERY_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_BATTERY_MSG_LEN_GET),
		handle_get,
	},
	BT_MESH_MODEL_OP_END,
};

static int update_handler(const struct bt_mesh_model *model)
{
	struct bt_mesh_battery_srv *srv = model->rt->user_data;
	struct bt_mesh_battery_status status = BATTERY_STATUS_DEFAULT;

	srv->get(srv, NULL, &status);

	bt_mesh_model_msg_init(model->pub->msg, BT_MESH_BATTERY_OP_STATUS);
	bt_mesh_gen_bat_encode_status(model->pub->msg, &status);

	return 0;
}

static int bt_mesh_battery_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_battery_srv *srv = model->rt->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	return 0;
}

static void bt_mesh_battery_srv_reset(const struct bt_mesh_model *model)
{
	net_buf_simple_reset(model->pub->msg);
}

const struct bt_mesh_model_cb _bt_mesh_battery_srv_cb = {
	.init = bt_mesh_battery_srv_init,
	.reset = bt_mesh_battery_srv_reset,
};

int bt_mesh_battery_srv_pub(struct bt_mesh_battery_srv *srv,
			    struct bt_mesh_msg_ctx *ctx,
			    const struct bt_mesh_battery_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_BATTERY_OP_STATUS,
				 BT_MESH_BATTERY_MSG_LEN_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_BATTERY_OP_STATUS);
	bt_mesh_gen_bat_encode_status(&msg, status);
	return bt_mesh_msg_send(srv->model, ctx, &msg);
}
