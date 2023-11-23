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

/* Global location */

static void rsp_global(const struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *rx_ctx,
		       const struct bt_mesh_loc_global *loc)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_GLOBAL_STATUS,
				 BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_GLOBAL_STATUS);
	bt_mesh_loc_global_encode(&msg, loc);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static int handle_global_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_loc_srv *srv = model->user_data;
	struct bt_mesh_loc_global global = LOC_GLOBAL_DEFAULT;

	srv->handlers->global_get(srv, ctx, &global);

	rsp_global(model, ctx, &global);

	return 0;
}

static int global_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_loc_srv *srv = model->user_data;
	struct bt_mesh_loc_global global;

	bt_mesh_loc_global_decode(buf, &global);
	srv->pub_state.is_global_available = 1;

	srv->handlers->global_set(srv, ctx, &global);

	if (ack) {
		rsp_global(model, ctx, &global);
	}

	(void)bt_mesh_loc_srv_global_pub(srv, NULL, &global);

	return 0;
}

static int handle_global_set(const struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	return global_set(model, ctx, buf, true);
}

static int handle_global_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	return global_set(model, ctx, buf, false);
}

/* Local location */

static void rsp_local(const struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *rx_ctx,
		      const struct bt_mesh_loc_local *loc)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_LOCAL_STATUS,
				 BT_MESH_LOC_MSG_LEN_LOCAL_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_LOCAL_STATUS);
	bt_mesh_loc_local_encode(&msg, loc);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static int handle_local_get(const struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_loc_srv *srv = model->user_data;
	struct bt_mesh_loc_local local = LOC_LOCAL_DEFAULT;

	srv->handlers->local_get(srv, ctx, &local);

	rsp_local(model, ctx, &local);

	return 0;
}

static int local_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_loc_srv *srv = model->user_data;
	struct bt_mesh_loc_local local;

	bt_mesh_loc_local_decode(buf, &local);
	srv->pub_state.is_local_available = 1;

	srv->handlers->local_set(srv, ctx, &local);

	if (ack) {
		rsp_local(model, ctx, &local);
	}

	(void)bt_mesh_loc_srv_local_pub(srv, NULL, &local);

	return 0;
}

static int handle_local_set(const struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	return local_set(model, ctx, buf, true);
}

static int handle_local_set_unack(const struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	return local_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_loc_srv_op[] = {
	{
		BT_MESH_LOC_OP_GLOBAL_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LOC_MSG_LEN_GLOBAL_GET),
		handle_global_get,
	},
	{
		BT_MESH_LOC_OP_LOCAL_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LOC_MSG_LEN_LOCAL_GET),
		handle_local_get,
	},
	BT_MESH_MODEL_OP_END
};
const struct bt_mesh_model_op _bt_mesh_loc_setup_srv_op[] = {
	{
		BT_MESH_LOC_OP_GLOBAL_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LOC_MSG_LEN_GLOBAL_SET),
		handle_global_set,
	},
	{
		BT_MESH_LOC_OP_GLOBAL_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LOC_MSG_LEN_GLOBAL_SET),
		handle_global_set_unack,
	},
	{
		BT_MESH_LOC_OP_LOCAL_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LOC_MSG_LEN_LOCAL_SET),
		handle_local_set,
	},
	{
		BT_MESH_LOC_OP_LOCAL_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LOC_MSG_LEN_LOCAL_SET),
		handle_local_set_unack,
	},
	BT_MESH_MODEL_OP_END
};

static void global_update_handler(struct bt_mesh_loc_srv *srv)
{
	struct bt_mesh_loc_global loc = LOC_GLOBAL_DEFAULT;

	srv->pub_state.was_last_local = 0;
	srv->handlers->global_get(srv, NULL, &loc);

	bt_mesh_model_msg_init(srv->pub.msg, BT_MESH_LOC_OP_GLOBAL_STATUS);
	bt_mesh_loc_global_encode(srv->pub.msg, &loc);
}

static void local_update_handler(struct bt_mesh_loc_srv *srv)
{
	struct bt_mesh_loc_local loc = LOC_LOCAL_DEFAULT;

	srv->pub_state.was_last_local = 1;
	srv->handlers->local_get(srv, NULL, &loc);

	bt_mesh_model_msg_init(srv->pub.msg, BT_MESH_LOC_OP_LOCAL_STATUS);
	bt_mesh_loc_local_encode(srv->pub.msg, &loc);
}

static int update_handler(const struct bt_mesh_model *model)
{
	struct bt_mesh_loc_srv *srv = model->user_data;

	if (!srv->pub_state.is_local_available && !srv->pub_state.is_global_available) {
		return -EINVAL;
	}

	if (!srv->pub_state.was_last_local) {
		if (!!srv->pub_state.is_local_available) {
			local_update_handler(srv);
		} else {
			global_update_handler(srv);
		}
	} else {
		if (!!srv->pub_state.is_global_available) {
			global_update_handler(srv);
		} else {
			local_update_handler(srv);
		}
	}

	return 0;
}

static int bt_mesh_loc_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_loc_srv *srv = model->user_data;

	memset(&srv->pub_state, 0, sizeof(srv->pub_state));
	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	return 0;
}

static void bt_mesh_loc_srv_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_loc_srv *srv = model->user_data;

	memset(&srv->pub_state, 0, sizeof(srv->pub_state));
	net_buf_simple_reset(model->pub->msg);
}

const struct bt_mesh_model_cb _bt_mesh_loc_srv_cb = {
	.init = bt_mesh_loc_srv_init,
	.reset = bt_mesh_loc_srv_reset,
};

static int bt_mesh_loc_setup_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_loc_srv *srv = model->user_data;
#if defined(CONFIG_BT_MESH_COMP_PAGE_1)
	int err = bt_mesh_model_correspond(model, srv->model);

	if (err) {
		return err;
	}
#endif

	return bt_mesh_model_extend(model, srv->model);
}

const struct bt_mesh_model_cb _bt_mesh_loc_setup_srv_cb = {
	.init = bt_mesh_loc_setup_srv_init,
};

int bt_mesh_loc_srv_global_pub(struct bt_mesh_loc_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_loc_global *global)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_GLOBAL_STATUS,
				 BT_MESH_LOC_MSG_LEN_GLOBAL_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_GLOBAL_STATUS);
	bt_mesh_loc_global_encode(&msg, global);

	return bt_mesh_msg_send(srv->model, ctx, &msg);
}

int bt_mesh_loc_srv_local_pub(struct bt_mesh_loc_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_loc_local *local)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LOC_OP_LOCAL_STATUS,
				 BT_MESH_LOC_MSG_LEN_LOCAL_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_LOC_OP_LOCAL_STATUS);
	bt_mesh_loc_local_encode(&msg, local);

	return bt_mesh_msg_send(srv->model, ctx, &msg);
}
