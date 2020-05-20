/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "model_utils.h"

static void encode_status(struct net_buf_simple *buf, u32_t transition_time)
{
	bt_mesh_model_msg_init(buf, BT_MESH_DTT_OP_STATUS);
	net_buf_simple_add_u8(buf, model_transition_encode(transition_time));
}

static void rsp_status(struct bt_mesh_dtt_srv *srv,
		       struct bt_mesh_msg_ctx *rx_ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DTT_OP_STATUS,
				 BT_MESH_DTT_MSG_LEN_STATUS);
	encode_status(&msg, srv->transition_time);

	(void)bt_mesh_model_send(srv->model, rx_ctx, &msg, NULL, NULL);
}

static void handle_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_DTT_MSG_LEN_GET) {
		return;
	}

	struct bt_mesh_dtt_srv *srv = model->user_data;

	rsp_status(srv, ctx);
}

static void set_dtt(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_DTT_MSG_LEN_SET) {
		return;
	}

	struct bt_mesh_dtt_srv *srv = model->user_data;
	u32_t old_time = srv->transition_time;
	u32_t new_time = model_transition_decode(net_buf_simple_pull_u8(buf));

	if (new_time == SYS_FOREVER_MS) {
		/* Invalid parameter */
		return;
	}

	srv->transition_time = new_time;

	if (old_time != new_time && srv->update) {
		srv->update(srv, ctx, old_time, srv->transition_time);
	}

	if (ack) {
		rsp_status(srv, ctx);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_DTT_SRV_PERSISTENT)) {
		(void)bt_mesh_model_data_store(model, false,
					       &srv->transition_time,
					       sizeof(srv->transition_time));
	}

	(void)bt_mesh_dtt_srv_pub(srv, NULL);
}

static void handle_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	set_dtt(model, ctx, buf, true);
}

static void handle_set_unack(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	set_dtt(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_dtt_srv_op[] = {
	{ BT_MESH_DTT_OP_GET, BT_MESH_DTT_MSG_LEN_GET, handle_get },
	{ BT_MESH_DTT_OP_SET, BT_MESH_DTT_MSG_LEN_SET, handle_set },
	{ BT_MESH_DTT_OP_SET_UNACK, BT_MESH_DTT_MSG_LEN_SET, handle_set_unack },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_dtt_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_dtt_srv *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(model->pub->msg, 0);

	return 0;
}

#ifdef CONFIG_BT_MESH_DTT_SRV_PERSISTENT
static int bt_mesh_dtt_srv_settings_set(struct bt_mesh_model *model,
					size_t len_rd, settings_read_cb read_cb,
					void *cb_arg)
{
	struct bt_mesh_dtt_srv *srv = model->user_data;

	ssize_t bytes = read_cb(cb_arg, &srv->transition_time,
				sizeof(srv->transition_time));
	if (bytes < 0) {
		return bytes;
	}

	if (bytes != 0 && bytes != sizeof(srv->transition_time)) {
		return -EINVAL;
	}

	return 0;
}
#endif

const struct bt_mesh_model_cb _bt_mesh_dtt_srv_cb = {
	.init = bt_mesh_dtt_srv_init,
#ifdef CONFIG_BT_MESH_DTT_SRV_PERSISTENT
	.settings_set = bt_mesh_dtt_srv_settings_set,
#endif
};

int _bt_mesh_dtt_srv_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_dtt_srv *srv = model->user_data;

	encode_status(srv->model->pub->msg, srv->transition_time);
	return 0;
}

void bt_mesh_dtt_srv_set(struct bt_mesh_dtt_srv *srv, u32_t transition_time)
{
	u32_t old = srv->transition_time;

	srv->transition_time = transition_time;

	if (srv->update) {
		srv->update(srv, NULL, old, srv->transition_time);
	}

	(void)bt_mesh_dtt_srv_pub(srv, NULL);
}

int bt_mesh_dtt_srv_pub(struct bt_mesh_dtt_srv *srv,
			struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DTT_OP_STATUS,
				 BT_MESH_DTT_MSG_LEN_STATUS);
	encode_status(&msg, srv->transition_time);

	return model_send(srv->model, ctx, &msg);
}
