/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <bluetooth/mesh/gen_onoff_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "gen_onoff_internal.h"
#include "model_utils.h"

static void encode_status(struct net_buf_simple *buf,
			  const struct bt_mesh_onoff_status *status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_ONOFF_OP_STATUS);
	net_buf_simple_add_u8(buf, !!status->present_on_off);

	if (status->remaining_time != 0) {
		net_buf_simple_add_u8(buf, status->target_on_off);
		net_buf_simple_add_u8(
			buf, model_transition_encode(status->remaining_time));
	}
}

static void rsp_status(const struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *rx_ctx,
		       const struct bt_mesh_onoff_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_STATUS,
				 BT_MESH_ONOFF_MSG_MAXLEN_STATUS);
	encode_status(&msg, status);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static int handle_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	struct bt_mesh_onoff_srv *srv = model->user_data;
	struct bt_mesh_onoff_status status = { 0 };

	srv->handlers->get(srv, ctx, &status);

	rsp_status(model, ctx, &status);

	return 0;
}

static int onoff_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_ONOFF_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_ONOFF_MSG_MAXLEN_SET) {
		return -EMSGSIZE;
	}

	struct bt_mesh_onoff_srv *srv = model->user_data;
	struct bt_mesh_onoff_status status = { 0 };
	struct bt_mesh_model_transition transition;
	struct bt_mesh_onoff_set set;

	uint8_t on_off = net_buf_simple_pull_u8(buf);
	uint8_t tid = net_buf_simple_pull_u8(buf);

	if (on_off > 1) {
		return -EINVAL;
	}

	set.on_off = on_off;

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* If this is the same transaction, we don't need to send it
		 * to the app, but we still have to respond with a status.
		 */
		srv->handlers->get(srv, NULL, &status);
		goto respond;
	}

	if (buf->len == 2) {
		model_transition_buf_pull(buf, &transition);
		set.transition = &transition;
	} else if (!atomic_test_bit(&srv->flags, GEN_ONOFF_SRV_NO_DTT)) {
		bt_mesh_dtt_srv_transition_get(srv->model, &transition);
		set.transition = &transition;
	} else {
		set.transition = NULL;
	}

	srv->handlers->set(srv, ctx, &set, &status);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	(void)bt_mesh_onoff_srv_pub(srv, NULL, &status);

respond:
	if (ack) {
		rsp_status(model, ctx, &status);
	}

	return 0;
}

static int handle_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	return onoff_set(model, ctx, buf, true);
}

static int handle_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	return onoff_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_onoff_srv_op[] = {
	{
		BT_MESH_ONOFF_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_ONOFF_MSG_LEN_GET),
		handle_get,
	},
	{
		BT_MESH_ONOFF_OP_SET,
		BT_MESH_LEN_MIN(BT_MESH_ONOFF_MSG_MINLEN_SET),
		handle_set,
	},
	{
		BT_MESH_ONOFF_OP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_ONOFF_MSG_MINLEN_SET),
		handle_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

/* .. include_startingpoint_scene_srv_rst_1 */
static ssize_t scene_store(const struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_onoff_srv *srv = model->user_data;
	struct bt_mesh_onoff_status status = { 0 };

	/* Only store the next stable on_off state: */
	srv->handlers->get(srv, NULL, &status);
	data[0] = status.remaining_time ? status.target_on_off :
					  status.present_on_off;

	return 1;
}

static void scene_recall(const struct bt_mesh_model *model, const uint8_t data[],
			 size_t len, struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_onoff_srv *srv = model->user_data;
	struct bt_mesh_onoff_status status = { 0 };
	struct bt_mesh_onoff_set set = {
		.on_off = data[0],
		.transition = transition,
	};

	srv->handlers->set(srv, NULL, &set, &status);
}

static void scene_recall_complete(const struct bt_mesh_model *model)
{
	struct bt_mesh_onoff_srv *srv = model->user_data;
	struct bt_mesh_onoff_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);

	(void)bt_mesh_onoff_srv_pub(srv, NULL, &status);
}

BT_MESH_SCENE_ENTRY_SIG(onoff) = {
	.id.sig = BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
	.maxlen = 1,
	.store = scene_store,
	.recall = scene_recall,
	.recall_complete = scene_recall_complete,
};
/* .. include_endpoint_scene_srv_rst_1 */

static int update_handler(const struct bt_mesh_model *model)
{
	struct bt_mesh_onoff_srv *srv = model->user_data;
	struct bt_mesh_onoff_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);
	encode_status(model->pub->msg, &status);

	return 0;
}

static int bt_mesh_onoff_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_onoff_srv *srv = model->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	return 0;
}

static void bt_mesh_onoff_srv_reset(const struct bt_mesh_model *model)
{
	net_buf_simple_reset(model->pub->msg);
}

const struct bt_mesh_model_cb _bt_mesh_onoff_srv_cb = {
	.init = bt_mesh_onoff_srv_init,
	.reset = bt_mesh_onoff_srv_reset,
};

int bt_mesh_onoff_srv_pub(struct bt_mesh_onoff_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_onoff_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_ONOFF_OP_STATUS,
				 BT_MESH_ONOFF_MSG_MAXLEN_STATUS);
	encode_status(&msg, status);
	return bt_mesh_msg_send(srv->model, ctx, &msg);
}
