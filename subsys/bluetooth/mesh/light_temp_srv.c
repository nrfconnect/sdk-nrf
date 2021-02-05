/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <sys/byteorder.h>
#include <bluetooth/mesh/light_temp_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "light_ctl_internal.h"
#include "model_utils.h"

struct settings_data {
	struct bt_mesh_light_temp dflt;
	struct bt_mesh_light_temp_range range;
	struct bt_mesh_light_temp last;
} __packed;

static void store_state(const struct bt_mesh_light_temp_srv *srv)
{
	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return;
	}

	struct settings_data data = {
		.dflt = srv->dflt,
		.range = srv->range,
		.last = srv->last,
	};

	bt_mesh_model_data_store(srv->model, false, NULL, &data, sizeof(data));
}

static void encode_status(struct net_buf_simple *buf,
			  const struct bt_mesh_light_temp_status *status)
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
	struct bt_mesh_light_temp_status status = { 0 };
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_temp_set set = {
		.transition = &transition,
	};

	set.params.temp = net_buf_simple_pull_le16(buf);
	set.params.delta_uv = net_buf_simple_pull_le16(buf);
	uint8_t tid = net_buf_simple_pull_u8(buf);

	if ((set.params.temp < BT_MESH_LIGHT_TEMP_MIN) ||
	    (set.params.temp > BT_MESH_LIGHT_TEMP_MAX)) {
		return;
	}

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

	bt_mesh_light_temp_srv_set(srv, ctx, &set, &status);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(&srv->lvl.scene);
	}

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
	struct bt_mesh_light_temp_status status = { 0 };
	struct bt_mesh_light_temp_set set = {
		.params = {
			.temp = lvl_to_temp(srv, lvl_set->lvl),
			.delta_uv = srv->last.delta_uv,
		},
		.transition = lvl_set->transition,
	};

	if (lvl_set->new_transaction) {
		bt_mesh_light_temp_srv_set(srv, ctx, &set, &status);
	} else if (rsp) {
		srv->handlers->get(srv, NULL, &status);
	}

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
	struct bt_mesh_light_temp_set set = {
		.params = srv->last,
		.transition = delta_set->transition,
	};
	int16_t start_lvl, target_lvl;

	if (delta_set->new_transaction) {
		srv->handlers->get(srv, NULL, &status);
		start_lvl = temp_to_lvl(srv, status.current.temp);
	} else {
		start_lvl = temp_to_lvl(srv, srv->last.temp);
	}

	/* Clamp to int16_t range before storing the value in a 16 bit integer
	 * to prevent overflow.
	 */
	target_lvl = CLAMP(start_lvl + delta_set->delta, BT_MESH_LVL_MIN,
			   BT_MESH_LVL_MAX);

	set.params.temp = lvl_to_temp(srv, target_lvl);

	bt_mesh_light_temp_srv_set(srv, ctx, &set, &status);

	/* Override last temp value to be able to make corrective deltas when
	 * new_transaction is false. Note that the last temp value in
	 * persistent storage will still be the target value, allowing us to
	 * recover correctly on power loss.
	 */
	srv->last.temp = lvl_to_temp(srv, start_lvl);

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
	struct bt_mesh_model_transition transition = {
		.delay = move_set->transition->delay,
	};
	struct bt_mesh_light_temp_set set = {
		.params = srv->last,
		.transition = &transition,
	};

	srv->handlers->get(srv, NULL, &status);

	if (move_set->delta > 0) {
		set.params.temp = srv->range.max;
	} else if (move_set->delta < 0) {
		set.params.temp = srv->range.min;
	} else {
		set.params.temp = status.current.temp;
	}

	if (move_set->delta != 0 && move_set->transition) {
		uint64_t distance = abs(set.params.temp - status.current.temp);

		transition.time =
			(distance * (uint64_t)move_set->transition->time) /
			abs(move_set->delta);
	}

	bt_mesh_light_temp_srv_set(srv, ctx, &set, &status);

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

static int scene_store(struct bt_mesh_model *mod, uint8_t data[])
{
	struct bt_mesh_light_temp_srv *srv = mod->user_data;

	sys_put_le16(srv->last.delta_uv, data);

	return sizeof(int16_t);
}

static void scene_recall(struct bt_mesh_model *mod, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_light_temp_srv *srv = mod->user_data;
	struct bt_mesh_light_temp_set set = {
		.params = {
			.temp = srv->last.temp,
			.delta_uv = sys_get_le16(data),
		},
		.transition = transition,
	};

	bt_mesh_light_temp_srv_set(srv, NULL, &set, NULL);
}

static const struct bt_mesh_scene_entry_type scene_type = {
	.store = scene_store,
	.recall = scene_recall,
	.maxlen = sizeof(int16_t),
};

static int bt_mesh_light_temp_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_temp_srv *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(srv->pub.msg, 0);

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		bt_mesh_model_extend(model, srv->lvl.model);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_SCENES)) {
		bt_mesh_scene_entry_add(model, &srv->scene, &scene_type, false);
	}

	return 0;
}

static int bt_mesh_light_temp_srv_settings_set(struct bt_mesh_model *mod,
					       const char *name, size_t len_rd,
					       settings_read_cb read_cb,
					       void *cb_data)
{
	struct bt_mesh_light_temp_srv *srv = mod->user_data;
	struct settings_data data;
	ssize_t len;

	len = read_cb(cb_data, &data, sizeof(data));
	if (len < sizeof(data)) {
		return -EINVAL;
	}

	srv->last = data.last;
	srv->dflt = data.dflt;
	srv->range = data.range;

	return 0;
}

static void bt_mesh_light_temp_srv_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_light_temp_srv *srv = model->user_data;

	net_buf_simple_reset(srv->pub.msg);
}

const struct bt_mesh_model_cb _bt_mesh_light_temp_srv_cb = {
	.init = bt_mesh_light_temp_srv_init,
	.reset = bt_mesh_light_temp_srv_reset,
	.settings_set = bt_mesh_light_temp_srv_settings_set,
};

void bt_mesh_light_temp_srv_set(struct bt_mesh_light_temp_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_light_temp_set *set,
				struct bt_mesh_light_temp_status *rsp)
{
	struct bt_mesh_light_temp_status status = { 0 };

	set->params.temp =
		MIN(MAX(set->params.temp, srv->range.min), srv->range.max);

	srv->last = set->params;
	store_state(srv);

	srv->handlers->set(srv, ctx, set, &status);
	if (rsp) {
		*rsp = status;
	}

	(void)bt_mesh_light_temp_srv_pub(srv, NULL, &status);

	struct bt_mesh_lvl_status lvl_status = {
		.current = temp_to_lvl(srv, status.current.temp),
		.target = temp_to_lvl(srv, status.target.temp),
		.remaining_time = status.remaining_time,
	};

	(void)bt_mesh_lvl_srv_pub(&srv->lvl, NULL, &lvl_status);
}

enum bt_mesh_model_status
bt_mesh_light_temp_srv_range_set(struct bt_mesh_light_temp_srv *srv,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_light_temp_range *range)
{
	const struct bt_mesh_light_temp_range old = srv->range;

	if ((range->min < BT_MESH_LIGHT_TEMP_MIN) ||
	    (range->min > range->max)) {
		return BT_MESH_MODEL_ERROR_INVALID_RANGE_MIN;
	}

	if (range->max > BT_MESH_LIGHT_TEMP_MAX) {
		return BT_MESH_MODEL_ERROR_INVALID_RANGE_MAX;
	}

	srv->range = *range;
	store_state(srv);

	if (srv->handlers->range_update) {
		srv->handlers->range_update(srv, ctx, &old, &srv->range);
	}

	return BT_MESH_MODEL_SUCCESS;
}

void bt_mesh_light_temp_srv_default_set(struct bt_mesh_light_temp_srv *srv,
					struct bt_mesh_msg_ctx *ctx,
					const struct bt_mesh_light_temp *dflt)
{
	const struct bt_mesh_light_temp old = srv->dflt;

	if ((dflt->temp < BT_MESH_LIGHT_TEMP_MIN) ||
	    (dflt->temp > BT_MESH_LIGHT_TEMP_MAX)) {
		return;
	}

	srv->dflt = *dflt;
	store_state(srv);

	if (srv->handlers->default_update) {
		srv->handlers->default_update(srv, ctx, &old, &srv->dflt);
	}
}

int bt_mesh_light_temp_srv_pub(struct bt_mesh_light_temp_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_light_temp_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_STATUS);
	encode_status(&msg, status);
	return model_send(srv->model, ctx, &msg);
}
