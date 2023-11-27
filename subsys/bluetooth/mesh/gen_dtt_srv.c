/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "model_utils.h"
#include "mesh/access.h"

static void encode_status(struct net_buf_simple *buf, uint32_t transition_time)
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

static int handle_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	struct bt_mesh_dtt_srv *srv = model->rt->user_data;

	rsp_status(srv, ctx);

	return 0;
}

static int set_dtt(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		   struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_dtt_srv *srv = model->rt->user_data;
	uint32_t old_time = srv->transition_time;
	uint32_t new_time = model_transition_decode(net_buf_simple_pull_u8(buf));

	if (new_time == SYS_FOREVER_MS) {
		/* Invalid parameter */
		return -EINVAL;
	}

	srv->transition_time = new_time;

	if (old_time != new_time && srv->update) {
		srv->update(srv, ctx, old_time, srv->transition_time);
	}

	if (ack) {
		rsp_status(srv, ctx);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_DTT_SRV_PERSISTENT)) {
		(void)bt_mesh_model_data_store(model, false, NULL,
					       &srv->transition_time,
					       sizeof(srv->transition_time));
	}

	(void)bt_mesh_dtt_srv_pub(srv, NULL);

	return 0;
}

static int handle_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	return set_dtt(model, ctx, buf, true);
}

static int handle_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	return set_dtt(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_dtt_srv_op[] = {
	{
		BT_MESH_DTT_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_DTT_MSG_LEN_GET),
		handle_get,
	},
	{
		BT_MESH_DTT_OP_SET,
		BT_MESH_LEN_EXACT(BT_MESH_DTT_MSG_LEN_SET),
		handle_set,
	},
	{
		BT_MESH_DTT_OP_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_DTT_MSG_LEN_SET),
		handle_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

static int update_handler(const struct bt_mesh_model *model)
{
	struct bt_mesh_dtt_srv *srv = model->rt->user_data;

	encode_status(srv->model->pub->msg, srv->transition_time);
	return 0;
}

static int bt_mesh_dtt_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_dtt_srv *srv = model->rt->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	return 0;
}

static void bt_mesh_dtt_srv_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_dtt_srv *srv = model->rt->user_data;

	srv->transition_time = 0;

	if (IS_ENABLED(CONFIG_BT_MESH_DTT_SRV_PERSISTENT)) {
		(void)bt_mesh_model_data_store(model, false, NULL, NULL, 0);
	}

	net_buf_simple_reset(model->pub->msg);
}

#ifdef CONFIG_BT_MESH_DTT_SRV_PERSISTENT
static int bt_mesh_dtt_srv_settings_set(const struct bt_mesh_model *model,
					const char *name,
					size_t len_rd, settings_read_cb read_cb,
					void *cb_arg)
{
	struct bt_mesh_dtt_srv *srv = model->rt->user_data;

	if (name) {
		return -ENOENT;
	}

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
	.reset = bt_mesh_dtt_srv_reset,
#ifdef CONFIG_BT_MESH_DTT_SRV_PERSISTENT
	.settings_set = bt_mesh_dtt_srv_settings_set,
#endif
};

void bt_mesh_dtt_srv_set(struct bt_mesh_dtt_srv *srv, uint32_t transition_time)
{
	uint32_t old = srv->transition_time;

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

	return bt_mesh_msg_send(srv->model, ctx, &msg);
}

struct bt_mesh_dtt_srv *bt_mesh_dtt_srv_get(const struct bt_mesh_elem *elem)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();
	uint16_t index;

	index = elem->rt->addr - comp->elem[0].rt->addr;
	for (int i = index; i >= 0; --i) {
		const struct bt_mesh_elem *element = &comp->elem[i];

		const struct bt_mesh_model *model =
			bt_mesh_model_find(element, BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_SRV);

		if (model) {
			return (struct bt_mesh_dtt_srv *)(model->rt->user_data);
		}
	};

	return NULL;
}
