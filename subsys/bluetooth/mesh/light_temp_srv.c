/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <bluetooth/mesh/light_temp_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "light_ctl_internal.h"
#include "model_utils.h"

static void encode_status(struct net_buf_simple *buf,
			  struct bt_mesh_light_temp_status *status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_TEMP_STATUS);
	net_buf_simple_add_le16(buf, status->current.temp);
	net_buf_simple_add_le16(buf, status->current.delta_uv);

	if (status->remaining_time != 0) {
		net_buf_simple_add_le16(buf, status->target.temp);
		net_buf_simple_add_le16(buf, status->target.delta_uv);
		net_buf_simple_add_u8(
			buf, model_transition_encode(status->remaining_time));
	}
}

static void rsp_status(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *rx_ctx,
		       struct bt_mesh_light_temp_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_STATUS);
	encode_status(&msg, status);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void temp_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_SET &&
	    buf->len != BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_SET) {
		return;
	}

	struct bt_mesh_light_temp_srv *srv = model->user_data;
	struct bt_mesh_light_temp_cb_set cb_msg;
	struct bt_mesh_light_temp_status status = { 0 };
	struct bt_mesh_model_transition transition;
	uint16_t temp = set_temp(srv, net_buf_simple_pull_le16(buf));
	int16_t delta_uv = net_buf_simple_pull_le16(buf);
	uint8_t tid = net_buf_simple_pull_u8(buf);

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* If this is the same transaction, we don't need to send it
		 * to the app, but we still have to respond with a status.
		 */
		srv->handlers->get(srv, NULL, &status);
		goto respond;
	}

	if (buf->len == 2) {
		model_transition_buf_pull(buf, &transition);
	} else {
		bt_mesh_dtt_srv_transition_get(srv->model, &transition);
	}

	cb_msg.temp = &temp;
	cb_msg.delta_uv = &delta_uv;
	cb_msg.time = transition.time;
	cb_msg.delay = transition.delay;

	srv->temp_last = temp;
	srv->delta_uv_last = delta_uv;
	srv->handlers->set(srv, ctx, &cb_msg, &status);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(&srv->lvl.scene);
	}

	struct bt_mesh_lvl_status lvl_status = {
		.current = temp_to_lvl(srv, status.current.temp),
		.target = temp_to_lvl(srv, status.target.temp),
		.remaining_time = status.remaining_time,

	};

	(void)bt_mesh_light_temp_srv_pub(srv, NULL, &status);
	(void)bt_mesh_lvl_srv_pub(&srv->lvl, NULL, &lvl_status);

respond:
	if (ack) {
		rsp_status(model, ctx, &status);
	}
}

static void temp_get_handle(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_LEN_GET) {
		return;
	}

	struct bt_mesh_light_temp_srv *srv = model->user_data;
	struct bt_mesh_light_temp_status status = { 0 };

	srv->handlers->get(srv, ctx, &status);
	rsp_status(model, ctx, &status);
}

static void temp_set_handle(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	temp_set(model, ctx, buf, true);
}

static void temp_set_unack_handle(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	temp_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_light_temp_srv_op[] = {
	{ BT_MESH_LIGHT_TEMP_GET, BT_MESH_LIGHT_CTL_MSG_LEN_GET,
	  temp_get_handle },
	{ BT_MESH_LIGHT_TEMP_SET, BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_SET,
	  temp_set_handle },
	{ BT_MESH_LIGHT_TEMP_SET_UNACK,
	  BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_SET, temp_set_unack_handle },
	BT_MESH_MODEL_OP_END,
};

static void lvl_get(struct bt_mesh_lvl_srv *lvl_srv,
		    struct bt_mesh_msg_ctx *ctx, struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_light_temp_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_light_temp_srv, lvl);
	struct bt_mesh_light_temp_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);

	rsp->current = temp_to_lvl(srv, status.current.temp);
	rsp->target = temp_to_lvl(srv, status.target.temp);
	rsp->remaining_time = status.remaining_time;
}

static void lvl_set(struct bt_mesh_lvl_srv *lvl_srv,
		    struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_lvl_set *lvl_set,
		    struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_light_temp_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_light_temp_srv, lvl);
	struct bt_mesh_light_temp_cb_set cb_msg;
	struct bt_mesh_light_temp_status status = { 0 };
	uint16_t temp = lvl_to_temp(srv, lvl_set->lvl);

	cb_msg.temp = &temp;
	cb_msg.delta_uv = NULL;
	cb_msg.time = lvl_set->transition->time;
	cb_msg.delay = lvl_set->transition->delay;

	if (lvl_set->new_transaction) {
		srv->handlers->set(srv, NULL, &cb_msg, &status);
		srv->temp_last = temp;
	} else if (rsp) {
		srv->handlers->get(srv, NULL, &status);
	}

	(void)bt_mesh_light_temp_srv_pub(srv, NULL, &status);

	if (rsp) {
		rsp->current = temp_to_lvl(srv, status.current.temp);
		rsp->target = temp_to_lvl(srv, status.target.temp);
		rsp->remaining_time = status.remaining_time;
	}
}

static void lvl_delta_set(struct bt_mesh_lvl_srv *lvl_srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_lvl_delta_set *delta_set,
			  struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_light_temp_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_light_temp_srv, lvl);
	struct bt_mesh_light_temp_status status = { 0 };
	struct bt_mesh_light_temp_cb_set cb_msg;
	int16_t start_value = temp_to_lvl(srv, srv->temp_last);
	int16_t target_value;

	if (delta_set->new_transaction) {
		srv->handlers->get(srv, NULL, &status);
		start_value = temp_to_lvl(srv, status.current.temp);
	}

	if (delta_set->delta > (INT16_MAX - start_value)) {
		target_value = INT16_MAX;
	} else if (delta_set->delta < (INT16_MIN - start_value)) {
		target_value = INT16_MIN;
	} else {
		target_value = start_value + delta_set->delta;
	}

	uint16_t new_temp = lvl_to_temp(srv, target_value);

	cb_msg.temp = &new_temp;
	cb_msg.delta_uv = NULL;
	cb_msg.time = delta_set->transition->time;
	cb_msg.delay = delta_set->transition->delay;
	srv->handlers->set(srv, ctx, &cb_msg, &status);

	/* Override "temp_last" value to be able to make corrective deltas when
	 * new_transaction is false. Note that the "temp_last" value in
	 * persistent storage will still be the target value, allowing us to
	 * recover Scorrectly on power loss.
	 */
	srv->temp_last = lvl_to_temp(srv, start_value);

	(void)bt_mesh_light_temp_srv_pub(srv, NULL, &status);

	if (rsp) {
		rsp->current = temp_to_lvl(srv, status.current.temp);
		rsp->target = temp_to_lvl(srv, status.target.temp);
		rsp->remaining_time = status.remaining_time;
	}
}

static void lvl_move_set(struct bt_mesh_lvl_srv *lvl_srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_lvl_move_set *move_set,
			 struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_light_temp_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_light_temp_srv, lvl);
	struct bt_mesh_light_temp_status status = { 0 };
	uint16_t target;

	srv->handlers->get(srv, NULL, &status);

	if (move_set->delta > 0) {
		target = srv->temp_range.max;
	} else if (move_set->delta < 0) {
		target = srv->temp_range.min;
	} else {
		target = status.current.temp;
	}

	struct bt_mesh_light_temp_cb_set cb_msg = {
		.temp = &target,
		.delta_uv = NULL,
	};

	if (move_set->delta != 0 && move_set->transition) {
		uint32_t distance = abs(target - status.current.temp);
		uint32_t time_to_edge = ((uint64_t)distance *
					 (uint64_t)move_set->transition->time) /
					abs(move_set->delta);

		if (time_to_edge > 0) {
			cb_msg.delay = move_set->transition->delay;
			cb_msg.time = time_to_edge;
		}
	}

	srv->handlers->set(srv, ctx, &cb_msg, &status);

	if (rsp) {
		rsp->current = temp_to_lvl(srv, status.current.temp);
		rsp->target = temp_to_lvl(srv, status.target.temp);
		rsp->remaining_time = status.remaining_time;
	}
}

const struct bt_mesh_lvl_srv_handlers _bt_mesh_light_temp_srv_lvl_handlers = {
	.get = lvl_get,
	.set = lvl_set,
	.delta_set = lvl_delta_set,
	.move_set = lvl_move_set,
};

static int bt_mesh_light_temp_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_temp_srv *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(srv->pub.msg, 0);

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		bt_mesh_model_extend(model, srv->lvl.model);
	}

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_temp_srv_cb = {
	.init = bt_mesh_light_temp_srv_init,
};

int32_t bt_mesh_light_temp_srv_pub(struct bt_mesh_light_temp_srv *srv,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_light_temp_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_STATUS);
	encode_status(&msg, status);
	return model_send(srv->model, ctx, &msg);
}
