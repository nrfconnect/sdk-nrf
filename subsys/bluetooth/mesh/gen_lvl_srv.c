/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <bluetooth/mesh/gen_lvl_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include <bluetooth/mesh/scene_srv.h>
#include <zephyr/sys/byteorder.h>
#include "model_utils.h"

static void encode_status(const struct bt_mesh_lvl_status *status,
			  struct net_buf_simple *buf)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LVL_OP_STATUS);
	net_buf_simple_add_le16(buf, status->current);

	if (status->remaining_time == 0) {
		return;
	}

	net_buf_simple_add_le16(buf, status->target);
	net_buf_simple_add_u8(buf,
			      model_transition_encode(status->remaining_time));
}

static void rsp_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       const struct bt_mesh_lvl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LVL_OP_STATUS,
				 BT_MESH_LVL_MSG_MAXLEN_STATUS);
	encode_status(status, &rsp);

	bt_mesh_model_send(model, ctx, &rsp, NULL, 0);
}

static int handle_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };

	srv->handlers->get(srv, ctx, &status);

	rsp_status(model, ctx, &status);

	return 0;
}

static int set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
	       struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LVL_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LVL_MSG_MAXLEN_SET) {
		return -EMSGSIZE;
	}

	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_lvl_status status = { 0 };
	struct bt_mesh_lvl_set set;

	set.lvl = net_buf_simple_pull_le16(buf);
	set.new_transaction = !tid_check_and_update(
		&srv->tid, net_buf_simple_pull_u8(buf), ctx);
	set.transition = model_transition_get(srv->model, &transition, buf);

	srv->handlers->set(srv, ctx, &set, &status);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	if (ack) {
		rsp_status(model, ctx, &status);
	}

	(void)bt_mesh_lvl_srv_pub(srv, NULL, &status);

	return 0;
}

static int delta_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LVL_MSG_MINLEN_DELTA_SET &&
	    buf->len != BT_MESH_LVL_MSG_MAXLEN_DELTA_SET) {
		return -EMSGSIZE;
	}

	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };
	struct bt_mesh_lvl_delta_set delta_set;
	struct bt_mesh_model_transition transition;

	delta_set.delta = net_buf_simple_pull_le32(buf);
	delta_set.new_transaction = !tid_check_and_update(
		&srv->tid, net_buf_simple_pull_u8(buf), ctx);
	delta_set.transition = model_transition_get(srv->model, &transition, buf);

	srv->handlers->delta_set(srv, ctx, &delta_set, &status);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	if (ack) {
		rsp_status(model, ctx, &status);
	}

	(void)bt_mesh_lvl_srv_pub(srv, NULL, &status);

	return 0;
}

static int move_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LVL_MSG_MINLEN_MOVE_SET &&
	    buf->len != BT_MESH_LVL_MSG_MAXLEN_MOVE_SET) {
		return -EMSGSIZE;
	}

	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };
	struct bt_mesh_model_transition transition;
	struct bt_mesh_lvl_move_set move_set;

	move_set.delta = net_buf_simple_pull_le16(buf);
	move_set.new_transaction = !tid_check_and_update(
		&srv->tid, net_buf_simple_pull_u8(buf), ctx);
	move_set.transition = model_transition_get(srv->model, &transition, buf);

	/* If transition.time is 0, we shouldn't move. Align these two
	 * parameters to simplify application logic for this case:
	 */
	if ((!move_set.transition || move_set.transition->time == 0) ||
	    move_set.delta == 0) {
		move_set.delta = 0;
		move_set.transition = NULL;
	}

	srv->handlers->move_set(srv, ctx, &move_set, &status);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	if (ack) {
		rsp_status(model, ctx, &status);
	}

	(void)bt_mesh_lvl_srv_pub(srv, ctx, &status);

	return 0;
}

/* Message handlers */

static int handle_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	return set(model, ctx, buf, true);
}

static int handle_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	return set(model, ctx, buf, false);
}

static int handle_delta_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	return delta_set(model, ctx, buf, true);
}

static int handle_delta_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	return delta_set(model, ctx, buf, false);
}

static int handle_move_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	return move_set(model, ctx, buf, true);
}

static int handle_move_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	return move_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_lvl_srv_op[] = {
	{
		BT_MESH_LVL_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LVL_MSG_LEN_GET),
		handle_get,
	},
	{
		BT_MESH_LVL_OP_SET,
		BT_MESH_LEN_MIN(BT_MESH_LVL_MSG_MINLEN_SET),
		handle_set,
	},
	{
		BT_MESH_LVL_OP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LVL_MSG_MINLEN_SET),
		handle_set_unack,
	},
	{
		BT_MESH_LVL_OP_DELTA_SET,
		BT_MESH_LEN_MIN(BT_MESH_LVL_MSG_MINLEN_DELTA_SET),
		handle_delta_set,
	},
	{
		BT_MESH_LVL_OP_DELTA_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LVL_MSG_MINLEN_DELTA_SET),
		handle_delta_set_unack,
	},
	{
		BT_MESH_LVL_OP_MOVE_SET,
		BT_MESH_LEN_MIN(BT_MESH_LVL_MSG_MINLEN_MOVE_SET),
		handle_move_set,
	},
	{
		BT_MESH_LVL_OP_MOVE_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LVL_MSG_MINLEN_MOVE_SET),
		handle_move_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

static ssize_t scene_store(const struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);
	sys_put_le16(status.remaining_time ? status.target : status.current,
		     &data[0]);

	return 2;
}

static void scene_recall(const struct bt_mesh_model *model, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };
	struct bt_mesh_lvl_set set = {
		.lvl = sys_get_le16(data),
		.new_transaction = true,
		.transition = transition,
	};

	srv->handlers->set(srv, NULL, &set, &status);
}

static void scene_recall_complete(const struct bt_mesh_model *model)
{
	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);

	(void)bt_mesh_lvl_srv_pub(srv, NULL, &status);
}

BT_MESH_SCENE_ENTRY_SIG(lvl) = {
	.id.sig = BT_MESH_MODEL_ID_GEN_LEVEL_SRV,
	.maxlen = 2,
	.store = scene_store,
	.recall = scene_recall,
	.recall_complete = scene_recall_complete,
};

static int update_handler(const struct bt_mesh_model *model)
{
	struct bt_mesh_lvl_srv *srv = model->user_data;
	struct bt_mesh_lvl_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);
	encode_status(&status, model->pub->msg);

	return 0;
}

static int bt_mesh_lvl_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_lvl_srv *srv = model->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	return 0;
}

static void bt_mesh_lvl_srv_reset(const struct bt_mesh_model *model)
{
	net_buf_simple_reset(model->pub->msg);
}

const struct bt_mesh_model_cb _bt_mesh_lvl_srv_cb = {
	.init = bt_mesh_lvl_srv_init,
	.reset = bt_mesh_lvl_srv_reset,
};

int bt_mesh_lvl_srv_pub(struct bt_mesh_lvl_srv *srv,
			struct bt_mesh_msg_ctx *ctx,
			const struct bt_mesh_lvl_status *status)
{
	if (!srv->pub.addr) {
		return 0;
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LVL_OP_STATUS,
				 BT_MESH_LVL_MSG_MAXLEN_STATUS);
	encode_status(status, &msg);

	return bt_mesh_msg_send(srv->model, ctx, &msg);
}
