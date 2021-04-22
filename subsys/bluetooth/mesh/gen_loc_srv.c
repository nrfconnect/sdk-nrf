/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/gen_loc_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include <string.h>
#include "model_utils.h"
#include "gen_loc_internal.h"

#define LOC_GLOBAL_DEFAULT                                                     \
	{                                                                      \
		.latitude = 0.0, .longitude = 0.0,                             \
		.altitude = BT_MESH_LOC_ALTITUDE_UNKNOWN,                      \
	}

#define LOC_LOCAL_DEFAULT                                                      \
	{                                                                      \
		.north = 0, .east = 0,                                         \
		.altitude = BT_MESH_LOC_ALTITUDE_UNKNOWN,                      \
		.floor_number = BT_MESH_LOC_FLOOR_NUMBER_UNKNOWN,              \
		.is_mobile = false, .time_delta = 0, .precision_mm = 4096000,  \
	}

static bool pub_in_progress(struct bt_mesh_loc_srv *srv)
{
	return k_work_delayable_is_pending(&srv->pub.timer);
}

/* Global location */

static void rsp_global(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *rx_ctx,
		       const struct bt_mesh_loc_global *loc)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_GLOBAL_STATUS,
				 BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_GLOBAL_STATUS);
	bt_mesh_loc_global_encode(&msg, loc);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void handle_global_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LOC_MSG_LEN_GLOBAL_GET) {
		return;
	}

	struct bt_mesh_loc_srv *srv = model->user_data;
	struct bt_mesh_loc_global global = LOC_GLOBAL_DEFAULT;

	srv->handlers->global_get(srv, ctx, &global);

	rsp_global(model, ctx, &global);
}

static void global_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LOC_MSG_LEN_GLOBAL_SET) {
		return;
	}

	struct bt_mesh_loc_srv *srv = model->user_data;
	struct bt_mesh_loc_global global;

	bt_mesh_loc_global_decode(buf, &global);

	srv->handlers->global_set(srv, ctx, &global);

	if (ack) {
		rsp_global(model, ctx, &global);
	}

	if (!pub_in_progress(srv) ||
	    srv->pub_op != BT_MESH_LOC_OP_GLOBAL_STATUS) {
		(void)bt_mesh_loc_srv_global_pub(srv, NULL, &global);
	}
}

static void handle_global_set(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	global_set(model, ctx, buf, true);
}

static void handle_global_set_unack(struct bt_mesh_model *model,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	global_set(model, ctx, buf, false);
}

/* Local location */

static void rsp_local(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *rx_ctx,
		      const struct bt_mesh_loc_local *loc)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_LOCAL_STATUS,
				 BT_MESH_LOC_MSG_LEN_LOCAL_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_LOCAL_STATUS);
	bt_mesh_loc_local_encode(&msg, loc);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void handle_local_get(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LOC_MSG_LEN_LOCAL_GET) {
		return;
	}

	struct bt_mesh_loc_srv *srv = model->user_data;
	struct bt_mesh_loc_local local = LOC_LOCAL_DEFAULT;

	srv->handlers->local_get(srv, ctx, &local);

	rsp_local(model, ctx, &local);
}

static void local_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LOC_MSG_LEN_LOCAL_SET) {
		return;
	}

	struct bt_mesh_loc_srv *srv = model->user_data;
	struct bt_mesh_loc_local local;

	bt_mesh_loc_local_decode(buf, &local);

	srv->handlers->local_set(srv, ctx, &local);

	if (ack) {
		rsp_local(model, ctx, &local);
	}

	if (!pub_in_progress(srv) ||
	    srv->pub_op != BT_MESH_LOC_OP_LOCAL_STATUS) {
		(void)bt_mesh_loc_srv_local_pub(srv, NULL, &local);
	}
}

static void handle_local_set(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	local_set(model, ctx, buf, true);
}

static void handle_local_set_unack(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	local_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_loc_srv_op[] = {
	{
		BT_MESH_LOC_OP_GLOBAL_GET,
		BT_MESH_LOC_MSG_LEN_GLOBAL_GET,
		handle_global_get,
	},
	{
		BT_MESH_LOC_OP_LOCAL_GET,
		BT_MESH_LOC_MSG_LEN_LOCAL_GET,
		handle_local_get,
	},
	BT_MESH_MODEL_OP_END
};
const struct bt_mesh_model_op _bt_mesh_loc_setup_srv_op[] = {
	{
		BT_MESH_LOC_OP_GLOBAL_SET,
		BT_MESH_LOC_MSG_LEN_GLOBAL_SET,
		handle_global_set,
	},
	{
		BT_MESH_LOC_OP_GLOBAL_SET_UNACK,
		BT_MESH_LOC_MSG_LEN_GLOBAL_SET,
		handle_global_set_unack,
	},
	{
		BT_MESH_LOC_OP_LOCAL_SET,
		BT_MESH_LOC_MSG_LEN_LOCAL_SET,
		handle_local_set,
	},
	{
		BT_MESH_LOC_OP_LOCAL_SET_UNACK,
		BT_MESH_LOC_MSG_LEN_LOCAL_SET,
		handle_local_set_unack,
	},
	BT_MESH_MODEL_OP_END
};

static int update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_loc_srv *srv = model->user_data;

	if (srv->pub_op == BT_MESH_LOC_OP_GLOBAL_STATUS) {
		struct bt_mesh_loc_global loc = LOC_GLOBAL_DEFAULT;

		srv->handlers->global_get(srv, NULL, &loc);

		bt_mesh_model_msg_init(srv->pub.msg,
				       BT_MESH_LOC_OP_GLOBAL_STATUS);
		bt_mesh_loc_global_encode(srv->pub.msg, &loc);
	} else if (srv->pub_op == BT_MESH_LOC_OP_LOCAL_STATUS) {
		struct bt_mesh_loc_local loc = LOC_LOCAL_DEFAULT;

		srv->handlers->local_get(srv, NULL, &loc);

		bt_mesh_model_msg_init(srv->pub.msg,
				       BT_MESH_LOC_OP_LOCAL_STATUS);
		bt_mesh_loc_local_encode(srv->pub.msg, &loc);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int bt_mesh_loc_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_loc_srv *srv = model->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	return 0;
}

static void bt_mesh_loc_srv_reset(struct bt_mesh_model *model)
{
	net_buf_simple_reset(model->pub->msg);
}

const struct bt_mesh_model_cb _bt_mesh_loc_srv_cb = {
	.init = bt_mesh_loc_srv_init,
	.reset = bt_mesh_loc_srv_reset,
};

int bt_mesh_loc_srv_global_pub(struct bt_mesh_loc_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_loc_global *global)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_GLOBAL_STATUS,
				 BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_GLOBAL_STATUS);
	bt_mesh_loc_global_encode(&msg, global);
	srv->pub_op = BT_MESH_LOC_OP_GLOBAL_STATUS;

	return model_send(srv->model, ctx, &msg);
}

int bt_mesh_loc_srv_local_pub(struct bt_mesh_loc_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_loc_local *local)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_LOCAL_STATUS,
				 BT_MESH_LOC_MSG_LEN_LOCAL_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_LOCAL_STATUS);
	bt_mesh_loc_local_encode(&msg, local);
	srv->pub_op = BT_MESH_LOC_OP_LOCAL_STATUS;

	return model_send(srv->model, ctx, &msg);
}
