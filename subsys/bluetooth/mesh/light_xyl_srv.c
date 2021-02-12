/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/byteorder.h>
#include <bluetooth/mesh/light_xyl_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "lightness_internal.h"
#include "model_utils.h"

struct bt_mesh_light_xyl_srv_settings_data {
	struct bt_mesh_light_xy default_params;
	struct bt_mesh_light_xy_range range;
	struct bt_mesh_light_xy xy_last;
} __packed;

static int store_state(struct bt_mesh_light_xyl_srv *srv)
{
	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return 0;
	}

	struct bt_mesh_light_xyl_srv_settings_data data = {
		.default_params = srv->xy_default,
		.range = srv->range,
		.xy_last = srv->xy_last,
	};

	return bt_mesh_model_data_store(srv->model, false, NULL, &data,
					sizeof(data));
}

static void xyl_get(struct bt_mesh_light_xyl_srv *srv,
		    struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_light_xyl_status *status)
{
	struct bt_mesh_lightness_status lightness = { 0 };
	struct bt_mesh_light_xy_status xy = { 0 };

	srv->lightness_srv.handlers->light_get(&srv->lightness_srv, ctx,
					       &lightness);
	srv->handlers->xy_get(srv, ctx, &xy);

	status->params.xy = xy.current;
	status->params.lightness = lightness.current;
	status->remaining_time =
		MAX(xy.remaining_time, lightness.remaining_time);
}

static void xyl_encode_status(struct net_buf_simple *buf,
			      struct bt_mesh_light_xyl_status *status,
			      uint32_t opcode)
{
	bt_mesh_model_msg_init(buf, opcode);
	net_buf_simple_add_le16(buf, status->params.lightness);
	net_buf_simple_add_le16(buf, status->params.xy.x);
	net_buf_simple_add_le16(buf, status->params.xy.y);
	if (status->remaining_time != 0) {
		net_buf_simple_add_u8(
			buf, model_transition_encode(status->remaining_time));
	}
}

static void xyl_rsp(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *rx_ctx,
		    struct bt_mesh_light_xyl_status *status, uint32_t opcode)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, opcode,
				 BT_MESH_LIGHT_XYL_MSG_MAXLEN_STATUS);
	xyl_encode_status(&msg, status, opcode);
	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void xyl_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LIGHT_XYL_MSG_MAXLEN_SET) {
		return;
	}

	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_xy_set set;
	struct bt_mesh_light_xy_status status = { 0 };
	struct bt_mesh_lightness_status light_rsp = { 0 };
	struct bt_mesh_lightness_set light = {
		.transition = &transition,
	};
	struct bt_mesh_light_xyl_status xyl_status;

	light.lvl = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);
	set.params.x = net_buf_simple_pull_le16(buf);
	set.params.y = net_buf_simple_pull_le16(buf);
	uint8_t tid = net_buf_simple_pull_u8(buf);

	if (set.params.x > srv->range.max.x) {
		set.params.x = srv->range.max.x;
	} else if (set.params.x < srv->range.min.x) {
		set.params.x = srv->range.min.x;
	}

	if (set.params.y > srv->range.max.y) {
		set.params.y = srv->range.max.y;
	} else if (set.params.y < srv->range.min.y) {
		set.params.y = srv->range.min.y;
	}

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* If this is the same transaction, we don't need to send it
		 * to the app, but we still have to respond with a status.
		 */
		if (ack) {
			xyl_get(srv, NULL, &xyl_status);
			xyl_rsp(model, ctx, &xyl_status,
				BT_MESH_LIGHT_XYL_OP_STATUS);
			return;
		}
	}

	if (buf->len == 2) {
		model_transition_buf_pull(buf, &transition);
	} else {
		bt_mesh_dtt_srv_transition_get(srv->model, &transition);
	}

	set.transition = &transition;
	lightness_srv_change_lvl(&srv->lightness_srv, ctx, &light, &light_rsp);
	srv->handlers->xy_set(srv, ctx, &set, &status);
	srv->xy_last.x = set.params.x;
	srv->xy_last.y = set.params.y;
	xyl_status.params.xy = status.current;
	xyl_status.params.lightness = light_rsp.current;
	xyl_status.remaining_time = status.remaining_time;
	store_state(srv);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(&srv->scene);
	}

	(void)bt_mesh_light_xyl_srv_pub(srv, NULL, &xyl_status);

	if (ack) {
		xyl_rsp(model, ctx, &xyl_status, BT_MESH_LIGHT_XYL_OP_STATUS);
	}
}

static void xyl_get_handle(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_LEN_GET) {
		return;
	}

	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xyl_status status = { 0 };

	xyl_get(srv, ctx, &status);
	xyl_rsp(model, ctx, &status, BT_MESH_LIGHT_XYL_OP_STATUS);
}

static void xyl_set_handle(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	xyl_set(model, ctx, buf, true);
}

static void xyl_set_unack_handle(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	xyl_set(model, ctx, buf, false);
}

static void target_get_handle(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_LEN_GET) {
		return;
	}

	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xy_status status = { 0 };
	struct bt_mesh_lightness_status light = { 0 };

	srv->lightness_srv.handlers->light_get(&srv->lightness_srv, ctx,
					       &light);
	srv->handlers->xy_get(srv, ctx, &status);

	struct bt_mesh_light_xyl_status xyl_status = {
		.params.xy = status.target,
		.params.lightness = light.target,
		.remaining_time = status.remaining_time,
	};

	xyl_rsp(model, ctx, &xyl_status, BT_MESH_LIGHT_XYL_OP_TARGET_STATUS);
}

static void default_encode_status(struct bt_mesh_light_xyl_srv *srv,
				  struct net_buf_simple *buf)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS);
	net_buf_simple_add_le16(buf, srv->lightness_srv.default_light);
	net_buf_simple_add_le16(buf, srv->xy_default.x);
	net_buf_simple_add_le16(buf, srv->xy_default.y);
}

static void default_rsp(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *rx_ctx)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT);
	default_encode_status(srv, &msg);
	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void default_set(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf,
			bool ack)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT) {
		return;
	}

	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xy old_default = srv->xy_default;
	uint16_t light = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);

	srv->xy_default.x = net_buf_simple_pull_le16(buf);
	srv->xy_default.y = net_buf_simple_pull_le16(buf);
	lightness_srv_default_set(&srv->lightness_srv, ctx, light);
	store_state(srv);

	if (srv->handlers->default_update) {
		srv->handlers->default_update(srv, ctx, &old_default,
					      &srv->xy_default);
	}

	(void)bt_mesh_light_xyl_srv_default_pub(srv, NULL);

	if (ack) {
		default_rsp(model, ctx);
	}
}

static void default_get_handle(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_LEN_GET) {
		return;
	}

	default_rsp(model, ctx);
}

static void default_set_handle(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	default_set(model, ctx, buf, true);
}

static void default_set_unack_handle(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	default_set(model, ctx, buf, false);
}

static void range_encode_status(struct net_buf_simple *buf,
				struct bt_mesh_light_xyl_srv *srv,
				enum bt_mesh_model_status status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_XYL_OP_RANGE_STATUS);
	net_buf_simple_add_u8(buf, status);
	net_buf_simple_add_le16(buf, srv->range.min.x);
	net_buf_simple_add_le16(buf, srv->range.max.x);
	net_buf_simple_add_le16(buf, srv->range.min.y);
	net_buf_simple_add_le16(buf, srv->range.max.y);
}

static void range_rsp(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *rx_ctx,
		      enum bt_mesh_model_status status)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_RANGE_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_STATUS);
	range_encode_status(&msg, srv, status);
	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void range_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_SET) {
		return;
	}

	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xy_range new_range;
	struct bt_mesh_light_xy_range old_range;
	enum bt_mesh_model_status status_code;

	new_range.min.x = net_buf_simple_pull_le16(buf);
	new_range.max.x = net_buf_simple_pull_le16(buf);
	new_range.min.y = net_buf_simple_pull_le16(buf);
	new_range.max.y = net_buf_simple_pull_le16(buf);

	if ((new_range.min.x > new_range.max.x) ||
	    (new_range.min.y > new_range.max.y)) {
		status_code = BT_MESH_MODEL_STATUS_INVALID;
		goto respond;
	}

	status_code = BT_MESH_MODEL_SUCCESS;
	old_range = srv->range;
	srv->range = new_range;
	store_state(srv);

	if (srv->handlers->range_update) {
		srv->handlers->range_update(srv, ctx, &old_range, &srv->range);
	}

	(void)bt_mesh_light_xyl_srv_range_pub(srv, NULL, status_code);

respond:
	if (ack) {
		range_rsp(model, ctx, status_code);
	}
}

static void range_get_handle(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_LEN_GET) {
		return;
	}

	range_rsp(model, ctx, BT_MESH_MODEL_SUCCESS);
}

static void range_set_handle(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	range_set(model, ctx, buf, true);
}

static void range_set_unack_handle(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	range_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_light_xyl_srv_op[] = {
	{ BT_MESH_LIGHT_XYL_OP_GET, BT_MESH_LIGHT_XYL_MSG_LEN_GET,
	  xyl_get_handle },
	{ BT_MESH_LIGHT_XYL_OP_SET, BT_MESH_LIGHT_XYL_MSG_MINLEN_SET,
	  xyl_set_handle },
	{ BT_MESH_LIGHT_XYL_OP_SET_UNACK, BT_MESH_LIGHT_XYL_MSG_MINLEN_SET,
	  xyl_set_unack_handle },
	{ BT_MESH_LIGHT_XYL_OP_TARGET_GET, BT_MESH_LIGHT_XYL_MSG_LEN_GET,
	  target_get_handle },
	{ BT_MESH_LIGHT_XYL_OP_DEFAULT_GET, BT_MESH_LIGHT_XYL_MSG_LEN_GET,
	  default_get_handle },
	{ BT_MESH_LIGHT_XYL_OP_RANGE_GET, BT_MESH_LIGHT_XYL_MSG_LEN_GET,
	  range_get_handle },
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_light_xyl_setup_srv_op[] = {
	{ BT_MESH_LIGHT_XYL_OP_DEFAULT_SET, BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT,
	  default_set_handle },
	{ BT_MESH_LIGHT_XYL_OP_DEFAULT_SET_UNACK,
	  BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT, default_set_unack_handle },
	{ BT_MESH_LIGHT_XYL_OP_RANGE_SET, BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_SET,
	  range_set_handle },
	{ BT_MESH_LIGHT_XYL_OP_RANGE_SET_UNACK,
	  BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_SET, range_set_unack_handle },
	BT_MESH_MODEL_OP_END,
};

static ssize_t scene_store(struct bt_mesh_model *mod, uint8_t data[])
{
	struct bt_mesh_light_xyl_srv *srv = mod->user_data;
	struct bt_mesh_light_xy_status xy_rsp = { 0 };

	srv->handlers->xy_get(srv, NULL, &xy_rsp);

	if (xy_rsp.remaining_time) {
		sys_put_le16(xy_rsp.target.x, data);
		sys_put_le16(xy_rsp.target.y, data + sizeof(uint16_t));
	} else {
		sys_put_le16(xy_rsp.current.x, data);
		sys_put_le16(xy_rsp.current.y, data + sizeof(uint16_t));
	}

	return sizeof(struct bt_mesh_light_xy);
}

static void scene_recall(struct bt_mesh_model *mod, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_light_xyl_srv *srv = mod->user_data;
	struct bt_mesh_light_xy_status xy_dummy;
	struct bt_mesh_light_xy_set xy_set = {
		.params.x = sys_get_le16(data),
		.params.y = sys_get_le16(data + sizeof(uint16_t)),
		.transition = transition,
	};

	srv->handlers->xy_set(srv, NULL, &xy_set, &xy_dummy);
}

static const struct bt_mesh_scene_entry_type scene_type = {
	.maxlen = sizeof(struct bt_mesh_light_xy),
	.store = scene_store,
	.recall = scene_recall,
};

static int update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xyl_status status = { 0 };

	xyl_get(srv, NULL, &status);
	xyl_encode_status(srv->pub.msg, &status, BT_MESH_LIGHT_XYL_OP_STATUS);

	return 0;
}

static int bt_mesh_light_xyl_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		/* Model extensions:
		 * To simplify the model extension tree, we're flipping the
		 * relationship between the Light xyL server and the Light xyL
		 * setup server. In the specification, the Light xyL setup
		 * server extends the time server, which is the opposite of
		 * what we're doing here. This makes no difference for the mesh
		 * stack, but it makes it a lot easier to extend this model, as
		 * we won't have to support multiple extenders.
		 */
		bt_mesh_model_extend(model, srv->lightness_srv.lightness_model);
		bt_mesh_model_extend(
			model, bt_mesh_model_find(
				       bt_mesh_model_elem(model),
				       BT_MESH_MODEL_ID_LIGHT_XYL_SETUP_SRV));
	}

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_entry_add(model, &srv->scene, &scene_type, false);
	}

	return 0;
}

static int bt_mesh_light_xyl_srv_settings_set(struct bt_mesh_model *model,
					      const char *name, size_t len_rd,
					      settings_read_cb read_cb,
					      void *cb_arg)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xyl_srv_settings_data data;

	if (read_cb(cb_arg, &data, sizeof(data)) != sizeof(data)) {
		return -EINVAL;
	}

	srv->xy_default = data.default_params;
	srv->range = data.range;
	srv->xy_last = data.xy_last;

	return 0;
}

static int bt_mesh_light_xyl_srv_start(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_xyl_srv *srv = mod->user_data;
	struct bt_mesh_light_xy_status xy_dummy;
	struct bt_mesh_model_transition transition = {
		.time = srv->lightness_srv.ponoff.dtt.transition_time,
		.delay = 0,
	};
	struct bt_mesh_light_xy_set set = {
		.transition = &transition,
	};

	switch (srv->lightness_srv.ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_ON:
		/* Intentional fallthrough */
	case BT_MESH_ON_POWER_UP_OFF:
		set.params = srv->xy_default;
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		set.params = srv->xy_last;
		break;
	default:
		return -EINVAL;
	}

	srv->handlers->xy_set(srv, NULL, &set, &xy_dummy);
	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_xyl_srv_cb = {
	.init = bt_mesh_light_xyl_srv_init,
	.start = bt_mesh_light_xyl_srv_start,
	.settings_set = bt_mesh_light_xyl_srv_settings_set,
};

int bt_mesh_light_xyl_srv_pub(struct bt_mesh_light_xyl_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_light_xyl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_MAXLEN_STATUS);
	xyl_encode_status(&msg, status, BT_MESH_LIGHT_XYL_OP_STATUS);
	return model_send(srv->model, ctx, &msg);
}

int bt_mesh_light_xyl_srv_target_pub(struct bt_mesh_light_xyl_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     struct bt_mesh_light_xyl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_TARGET_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_MAXLEN_STATUS);
	xyl_encode_status(&msg, status, BT_MESH_LIGHT_XYL_OP_TARGET_STATUS);
	return model_send(srv->model, ctx, &msg);
}

int bt_mesh_light_xyl_srv_range_pub(struct bt_mesh_light_xyl_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    enum bt_mesh_model_status status_code)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_RANGE_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_STATUS);
	range_encode_status(&msg, srv, status_code);
	return model_send(srv->model, ctx, &msg);
}

int bt_mesh_light_xyl_srv_default_pub(struct bt_mesh_light_xyl_srv *srv,
				      struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT);
	default_encode_status(srv, &msg);
	return model_send(srv->model, ctx, &msg);
}
