/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include "thingy52_mod.h"
#include "model_utils.h"
#include <bluetooth/mesh/models.h>

static void handle_rgb_set(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_thingy52_mod *srv = model->user_data;
	struct bt_mesh_thingy52_rgb_msg rgb_message;

	uint8_t ttl_speaker = net_buf_simple_pull_u8(buf);

	rgb_message.speaker_on = (ttl_speaker >> 7);
	rgb_message.ttl = (ttl_speaker & 0x7F);
	rgb_message.duration =
		model_transition_decode(net_buf_simple_pull_u8(buf));
	rgb_message.color.red = net_buf_simple_pull_u8(buf);
	rgb_message.color.green = net_buf_simple_pull_u8(buf);
	rgb_message.color.blue = net_buf_simple_pull_u8(buf);

	if (srv->msg_callbacks && srv->msg_callbacks->rgb_set_handler) {
		srv->msg_callbacks->rgb_set_handler(srv, ctx, rgb_message);
	}
}

const struct bt_mesh_model_op _bt_mesh_thingy52_mod_op[] = {
	{ BT_MESH_THINGY52_OP_RGB_SET, BT_MESH_THINGY52_MSG_LEN_RGB_SET,
	  handle_rgb_set },
};

static int bt_mesh_thingy52_mod_init(struct bt_mesh_model *model)
{
	struct bt_mesh_thingy52_mod *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(model->pub->msg, 0);
	return 0;
}

int bt_mesh_thingy52_mod_rgb_set(struct bt_mesh_thingy52_mod *srv,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_thingy52_rgb_msg *rgb)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_THINGY52_OP_RGB_SET,
				 BT_MESH_THINGY52_MSG_LEN_RGB_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_THINGY52_OP_RGB_SET);

	net_buf_simple_add_u8(&msg, rgb->ttl | ((!!rgb->speaker_on) << 7));
	net_buf_simple_add_u8(&msg, model_transition_encode(rgb->duration));
	net_buf_simple_add_u8(&msg, rgb->color.red);
	net_buf_simple_add_u8(&msg, rgb->color.green);
	net_buf_simple_add_u8(&msg, rgb->color.blue);
	return model_send(srv->model, ctx, &msg);
}

const struct bt_mesh_model_cb _bt_mesh_thingy52_mod_cb = {
	.init = bt_mesh_thingy52_mod_init
};
