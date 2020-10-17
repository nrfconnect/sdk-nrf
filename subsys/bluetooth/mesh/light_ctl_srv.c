/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sys/byteorder.h>
#include <bluetooth/mesh/light_ctl_srv.h>
#include <bluetooth/mesh/light_temp_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "light_ctl_internal.h"
#include "lightness_internal.h"
#include "model_utils.h"
#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_light_ctl_srv
#include "common/log.h"

struct bt_mesh_light_ctl_srv_settings_data {
	struct bt_mesh_light_ctl default_params;
	struct bt_mesh_light_temp_range temp_range;
	uint16_t temp_last;
	int16_t delta_uv_last;
} __packed;

static int store_state(struct bt_mesh_light_ctl_srv *srv)
{
	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return 0;
	}

	struct bt_mesh_light_ctl_srv_settings_data data = {
		.default_params = srv->default_params,
		.temp_range = srv->temp_srv.temp_range,
		.temp_last = srv->temp_srv.temp_last,
		.delta_uv_last = srv->temp_srv.delta_uv_last,
	};

	return bt_mesh_model_data_store(srv->model, false, NULL, &data,
					sizeof(data));
}

static void ctl_encode_status(struct net_buf_simple *buf,
			      struct bt_mesh_light_ctl_status *status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_CTL_STATUS);
	net_buf_simple_add_le16(buf,
				light_to_repr(status->current_light, ACTUAL));
	net_buf_simple_add_le16(buf, status->current_temp);
	if (status->remaining_time != 0) {
		net_buf_simple_add_le16(buf, light_to_repr(status->target_light,
							   ACTUAL));
		net_buf_simple_add_le16(buf, status->target_temp);
		net_buf_simple_add_u8(
			buf, model_transition_encode(status->remaining_time));
	}
}

static void ctl_rsp(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *rx_ctx,
		    struct bt_mesh_light_ctl_rsp *gen_status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_STATUS);
	struct bt_mesh_light_ctl_status status = {
		.current_light = gen_status->current.light,
		.current_temp = gen_status->current.temp,
		.target_light = gen_status->target.light,
		.target_temp = gen_status->target.temp,
		.remaining_time = gen_status->remaining_time,
	};
	ctl_encode_status(&msg, &status);
	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void light_ctl_pub(struct bt_mesh_light_ctl_srv *srv,
			  struct bt_mesh_light_ctl_rsp *gen_status)
{
	struct bt_mesh_light_ctl_status status = {
		.current_light = gen_status->current.light,
		.current_temp = gen_status->current.temp,
		.target_light = gen_status->target.light,
		.target_temp = gen_status->target.temp,
		.remaining_time = gen_status->remaining_time,
	};

	(void)bt_mesh_light_ctl_pub(srv, NULL, &status);
}

static void ctl_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LIGHT_CTL_MSG_MAXLEN_SET) {
		return;
	}

	struct bt_mesh_light_ctl_srv *srv = model->user_data;
	struct bt_mesh_light_ctl_rsp status = { 0 };
	struct bt_mesh_light_ctl_gen_cb_set set;
	struct bt_mesh_model_transition transition;
	uint16_t light = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);
	uint16_t temp =
		set_temp(&(srv->temp_srv), net_buf_simple_pull_le16(buf));
	uint16_t delta_uv = net_buf_simple_pull_le16(buf);
	uint8_t tid = net_buf_simple_pull_u8(buf);

	if (light != 0) {
		if (light > srv->lightness_srv.range.max) {
			light = srv->lightness_srv.range.max;
		} else if (light < srv->lightness_srv.range.min) {
			light = srv->lightness_srv.range.min;
		}
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

	srv->temp_srv.temp_last = temp;
	srv->temp_srv.delta_uv_last = delta_uv;
	store_state(srv);

	set.light = &light;
	set.temp = &temp;
	set.delta_uv = &delta_uv;
	set.time = transition.time;
	set.delay = transition.delay;
	srv->handlers->set(srv, ctx, &set, &status);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(&srv->scene);
	}

	struct bt_mesh_light_temp_status temp_status = {
		.current.temp = status.current.temp,
		.current.delta_uv = status.current.delta_uv,
		.target.temp = status.target.temp,
		.target.delta_uv = status.target.delta_uv,
		.remaining_time = status.remaining_time,
	};
	struct bt_mesh_lvl_status lvl_status = {
		.current = temp_to_lvl(&srv->temp_srv, status.current.temp),
		.target = temp_to_lvl(&srv->temp_srv, status.target.temp),
		.remaining_time = status.remaining_time,
	};
	struct bt_mesh_lightness_status lightness_status = {
		.current = status.current.light,
		.target = status.target.light,
		.remaining_time = status.remaining_time,
	};

	light_ctl_pub(srv, &status);
	(void)bt_mesh_light_temp_srv_pub(&srv->temp_srv, NULL,
					     &temp_status);
	(void)bt_mesh_lvl_srv_pub(&srv->temp_srv.lvl, NULL, &lvl_status);
	(void)bt_mesh_lightness_srv_pub(&srv->lightness_srv, NULL,
					&lightness_status);

respond:
	if (ack) {
		ctl_rsp(model, ctx, &status);
	}
}

static void handle_ctl_get(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctl_srv *ctl_srv = model->user_data;
	struct bt_mesh_light_ctl_rsp status = { 0 };

	ctl_srv->handlers->get(ctl_srv, NULL, &status);
	ctl_rsp(model, ctx, &status);
}

static void handle_ctl_set(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	ctl_set(model, ctx, buf, true);
}

static void handle_ctl_set_unack(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	ctl_set(model, ctx, buf, false);
}

static void range_encode_status(struct net_buf_simple *buf,
				struct bt_mesh_light_ctl_srv *srv,
				enum bt_mesh_model_status status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_TEMP_RANGE_STATUS);
	net_buf_simple_add_u8(buf, status);
	net_buf_simple_add_le16(buf, srv->temp_srv.temp_range.min);
	net_buf_simple_add_le16(buf, srv->temp_srv.temp_range.max);
}

static void temp_range_rsp(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *rx_ctx,
			   enum bt_mesh_model_status status)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_RANGE_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_STATUS);
	range_encode_status(&msg, srv, status);
	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void temp_range_set(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_SET) {
		return;
	}

	struct bt_mesh_light_ctl_srv *srv = model->user_data;
	enum bt_mesh_model_status status;
	uint16_t new_min = net_buf_simple_pull_le16(buf);
	uint16_t new_max = net_buf_simple_pull_le16(buf);

	if ((new_min < BT_MESH_LIGHT_TEMP_RANGE_MIN) || (new_min >= new_max)) {
		status = BT_MESH_MODEL_ERROR_INVALID_RANGE_MIN;
		goto respond;
	} else if (new_max > BT_MESH_LIGHT_TEMP_RANGE_MAX) {
		status = BT_MESH_MODEL_ERROR_INVALID_RANGE_MAX;
		goto respond;
	}

	srv->temp_srv.temp_range.min = new_min;
	srv->temp_srv.temp_range.max = new_max;
	store_state(srv);
	status = BT_MESH_MODEL_SUCCESS;

	if (srv->handlers->temp_range_update) {
		srv->handlers->temp_range_update(srv, ctx,
						 &srv->temp_srv.temp_range);
	}

	(void)bt_mesh_light_ctl_range_pub(srv, NULL, status);

respond:
	if (ack) {
		temp_range_rsp(model, ctx, status);
	}
}

static void handle_temp_range_get(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_LEN_GET) {
		return;
	}

	temp_range_rsp(model, ctx, BT_MESH_MODEL_SUCCESS);
}

static void handle_temp_range_set(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	temp_range_set(model, ctx, buf, true);
}

static void handle_temp_range_set_unack(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	temp_range_set(model, ctx, buf, false);
}

static void default_encode_status(struct net_buf_simple *buf,
				  struct bt_mesh_light_ctl_srv *srv)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_CTL_DEFAULT_STATUS);
	net_buf_simple_add_le16(buf, light_to_repr(srv->default_params.light,
						   ACTUAL));
	net_buf_simple_add_le16(buf, srv->default_params.temp);
	net_buf_simple_add_le16(buf, srv->default_params.delta_uv);
}

static void default_rsp(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *rx_ctx)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_DEFAULT_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG);
	default_encode_status(&msg, srv);
	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void default_set(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf,
			bool ack)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG) {
		return;
	}

	struct bt_mesh_light_ctl_srv *srv = model->user_data;
	uint16_t light = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);
	uint16_t temp = net_buf_simple_pull_le16(buf);
	uint16_t delta_uv = net_buf_simple_pull_le16(buf);

	if ((temp < BT_MESH_LIGHT_TEMP_RANGE_MIN) ||
	    (temp > BT_MESH_LIGHT_TEMP_RANGE_MAX)) {
		goto respond;
	}

	srv->default_params.light = light;
	srv->default_params.temp = temp;
	srv->default_params.delta_uv = delta_uv;
	store_state(srv);

	if (srv->handlers->default_update) {
		srv->handlers->default_update(srv, ctx, &srv->default_params);
	}

	(void)bt_mesh_light_ctl_default_pub(srv, NULL);

respond:
	if (ack) {
		default_rsp(model, ctx);
	}
}

static void handle_default_get(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_LEN_GET) {
		return;
	}

	default_rsp(model, ctx);
}

static void handle_default_set(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	default_set(model, ctx, buf, true);
}

static void handle_default_set_unack(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	default_set(model, ctx, buf, false);
}

static void temp_set(struct bt_mesh_light_temp_srv *srv,
		     struct bt_mesh_msg_ctx *ctx,
		     const struct bt_mesh_light_temp_cb_set *set,
		     struct bt_mesh_light_temp_status *rsp)
{
	struct bt_mesh_light_ctl_srv *ctl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_ctl_srv, temp_srv);
	struct bt_mesh_light_ctl_rsp gen_rsp = { 0 };
	struct bt_mesh_light_ctl_gen_cb_set gen_set = {
		.light = NULL,
		.temp = set->temp,
		.time = set->time,
		.delay = set->delay,
		.delta_uv = set->delta_uv ? set->delta_uv : NULL,
	};

	ctl_srv->handlers->set(ctl_srv, ctx, &gen_set, &gen_rsp);

	store_state(ctl_srv);
	light_ctl_pub(ctl_srv, &gen_rsp);

	rsp->current.delta_uv = gen_rsp.current.delta_uv;
	rsp->current.temp = gen_rsp.current.temp;
	rsp->remaining_time = gen_rsp.remaining_time;
	rsp->target.delta_uv = gen_rsp.target.delta_uv;
	rsp->target.temp = gen_rsp.target.temp;
}

static void temp_get(struct bt_mesh_light_temp_srv *srv,
		     struct bt_mesh_msg_ctx *ctx,
		     struct bt_mesh_light_temp_status *rsp)
{
	struct bt_mesh_light_ctl_srv *ctl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_ctl_srv, temp_srv);
	struct bt_mesh_light_ctl_rsp gen_rsp = { 0 };

	ctl_srv->handlers->get(ctl_srv, ctx, &gen_rsp);

	rsp->current.delta_uv = gen_rsp.current.delta_uv;
	rsp->current.temp = gen_rsp.current.temp;
	rsp->remaining_time = gen_rsp.remaining_time;
	rsp->target.delta_uv = gen_rsp.target.delta_uv;
	rsp->target.temp = gen_rsp.target.temp;
}

static void light_set(struct bt_mesh_lightness_srv *srv,
		      struct bt_mesh_msg_ctx *ctx,
		      const struct bt_mesh_lightness_set *set,
		      struct bt_mesh_lightness_status *rsp)
{
	struct bt_mesh_light_ctl_srv *ctl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_ctl_srv, lightness_srv);
	struct bt_mesh_light_ctl_rsp gen_rsp = { 0 };
	uint16_t lvl = set->lvl;
	struct bt_mesh_light_ctl_gen_cb_set gen_set = {
		.light = &lvl,
		.temp = NULL,
		.delta_uv = NULL,
		.time = set->transition->time,
		.delay = set->transition->delay,
	};

	ctl_srv->handlers->set(ctl_srv, ctx, &gen_set, &gen_rsp);
	light_ctl_pub(ctl_srv, &gen_rsp);

	rsp->current = gen_rsp.current.light;
	rsp->target = gen_rsp.target.light;
	rsp->remaining_time = gen_rsp.remaining_time;
}

static void light_get(struct bt_mesh_lightness_srv *srv,
		      struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_lightness_status *rsp)
{
	struct bt_mesh_light_ctl_srv *ctl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_ctl_srv, lightness_srv);
	struct bt_mesh_light_ctl_rsp gen_rsp = { 0 };

	ctl_srv->handlers->get(ctl_srv, ctx, &gen_rsp);

	rsp->current = gen_rsp.current.light;
	rsp->target = gen_rsp.target.light;
	rsp->remaining_time = gen_rsp.remaining_time;
}

static void
lightness_range_update(struct bt_mesh_lightness_srv *srv,
		       struct bt_mesh_msg_ctx *ctx,
		       const struct bt_mesh_lightness_range *old_range,
		       const struct bt_mesh_lightness_range *new_range)
{
	struct bt_mesh_light_ctl_srv *ctl_srv =
		CONTAINER_OF(srv, struct bt_mesh_light_ctl_srv, lightness_srv);
	if (ctl_srv->handlers->lightness_range_update) {
		ctl_srv->handlers->lightness_range_update(ctl_srv, ctx,
							  old_range, new_range);
	}
}

const struct bt_mesh_lightness_srv_handlers
	_bt_mesh_light_ctl_lightness_handlers = {
		.light_set = light_set,
		.light_get = light_get,
		.default_update = NULL,
		.range_update = lightness_range_update,
	};

const struct bt_mesh_light_temp_srv_handlers
	_bt_mesh_light_temp_handlers = {
		.set = temp_set,
		.get = temp_get,
	};

const struct bt_mesh_model_op _bt_mesh_light_ctl_srv_op[] = {
	{ BT_MESH_LIGHT_CTL_GET, BT_MESH_LIGHT_CTL_MSG_LEN_GET,
	  handle_ctl_get },
	{ BT_MESH_LIGHT_CTL_SET, BT_MESH_LIGHT_CTL_MSG_MINLEN_SET,
	  handle_ctl_set },
	{ BT_MESH_LIGHT_CTL_SET_UNACK, BT_MESH_LIGHT_CTL_MSG_MINLEN_SET,
	  handle_ctl_set_unack },
	{ BT_MESH_LIGHT_TEMP_RANGE_GET, BT_MESH_LIGHT_CTL_MSG_LEN_GET,
	  handle_temp_range_get },
	{ BT_MESH_LIGHT_CTL_DEFAULT_GET, BT_MESH_LIGHT_CTL_MSG_LEN_GET,
	  handle_default_get },
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_light_ctl_setup_srv_op[] = {
	{ BT_MESH_LIGHT_TEMP_RANGE_SET,
	  BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_SET, handle_temp_range_set },
	{ BT_MESH_LIGHT_TEMP_RANGE_SET_UNACK,
	  BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_SET,
	  handle_temp_range_set_unack },
	{ BT_MESH_LIGHT_CTL_DEFAULT_SET, BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG,
	  handle_default_set },
	{ BT_MESH_LIGHT_CTL_DEFAULT_SET_UNACK,
	  BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG, handle_default_set_unack },
	BT_MESH_MODEL_OP_END,
};

static ssize_t scene_store(struct bt_mesh_model *mod, uint8_t data[])
{
	struct bt_mesh_light_ctl_srv *srv = mod->user_data;
	struct bt_mesh_light_ctl_rsp rsp;

	srv->handlers->get(srv, NULL, &rsp);

	/* Only need to store the delta UV in the scene, the rest is stored by
	 * the extended models.
	 */
	if (rsp.remaining_time) {
		sys_put_le16(rsp.target.delta_uv, data);
	} else {
		sys_put_le16(rsp.current.delta_uv, data);
	}

	return 2;
}

static void scene_recall(struct bt_mesh_model *mod, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_light_ctl_srv *srv = mod->user_data;
	uint16_t delta_uv = sys_get_le16(data);
	struct bt_mesh_light_ctl_gen_cb_set set = {
		.delta_uv = &delta_uv,
		.time = transition->time,
		.delay = transition->delay,
	};

	srv->handlers->set(srv, NULL, &set, NULL);
}

static const struct bt_mesh_scene_entry_type scene_type = {
	.maxlen = 2,
	.store = scene_store,
	.recall = scene_recall,
};

static int bt_mesh_light_ctl_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(srv->pub.msg, 0);

	/* Disable On power up procedure on lightness server */
	atomic_set_bit(&srv->lightness_srv.flags, LIGHTNESS_SRV_FLAG_NO_START);

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		/* Model extensions:
		 * To simplify the model extension tree, we're flipping the
		 * relationship between the Light CTL server and the Light CTL
		 * setup server. In the specification, the Light CTL setup
		 * server extends the time server, which is the opposite of
		 * what we're doing here. This makes no difference for the mesh
		 * stack, but it makes it a lot easier to extend this model, as
		 * we won't have to support multiple extenders.
		 */
		bt_mesh_model_extend(model, srv->lightness_srv.lightness_model);
		bt_mesh_model_extend(
			model, bt_mesh_model_find(
				       bt_mesh_model_elem(model),
				       BT_MESH_MODEL_ID_LIGHT_CTL_SETUP_SRV));
	}

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_entry_add(model, &srv->scene, &scene_type, false);
	}

	return 0;
}

static int bt_mesh_light_ctl_srv_settings_set(struct bt_mesh_model *model,
					 const char *name, size_t len_rd,
					 settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;
	struct bt_mesh_light_ctl_srv_settings_data data;

	if (read_cb(cb_arg, &data, sizeof(data)) != sizeof(data)) {
		return -EINVAL;
	}

	srv->default_params = data.default_params;
	srv->temp_srv.temp_range = data.temp_range;
	srv->temp_srv.temp_last = data.temp_last;
	srv->temp_srv.delta_uv_last = data.delta_uv_last;

	return 0;
}

static int bt_mesh_light_ctl_srv_start(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_ctl_srv *srv = mod->user_data;

	if (!srv->temp_srv.model ||
	    (srv->model->elem_idx > srv->temp_srv.model->elem_idx)) {
		BT_ERR("Light CTL srv[%d]: Temp. srv not properly initialized",
		       srv->model->elem_idx);
		return -EINVAL;
	}

	struct bt_mesh_light_ctl_rsp dummy;
	struct bt_mesh_light_ctl_gen_cb_set set = {
		.light = NULL,
		.delta_uv = NULL,
		.time = srv->lightness_srv.ponoff.dtt.transition_time,
		.delay = 0,
	};
	uint16_t temp = set_temp(&(srv->temp_srv), srv->default_params.temp);

	switch (srv->lightness_srv.ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_ON:
		set.light = &srv->default_params.light;
		/* Intentional fallthrough */
	case BT_MESH_ON_POWER_UP_OFF:
		set.delta_uv = &srv->default_params.delta_uv;
		set.temp = &temp;
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		set.temp = &srv->temp_srv.temp_last;
		set.delta_uv = &srv->temp_srv.delta_uv_last;

		if (atomic_test_bit(&srv->lightness_srv.flags,
				    LIGHTNESS_SRV_FLAG_IS_ON)) {
			set.light = &srv->lightness_srv.last;
		}

		break;
	default:
		return -EINVAL;
	}

	srv->handlers->set(srv, NULL, &set, &dummy);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_ctl_srv_cb = {
	.init = bt_mesh_light_ctl_srv_init,
	.start = bt_mesh_light_ctl_srv_start,
	.settings_set = bt_mesh_light_ctl_srv_settings_set,
};

int32_t bt_mesh_light_ctl_pub(struct bt_mesh_light_ctl_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_light_ctl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_STATUS);

	ctl_encode_status(&msg, status);
	return model_send(srv->model, ctx, &msg);
}

int32_t bt_mesh_light_ctl_range_pub(struct bt_mesh_light_ctl_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    enum bt_mesh_model_status status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_RANGE_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_STATUS);
	range_encode_status(&msg, srv, status);
	return model_send(srv->model, ctx, &msg);
}

int32_t bt_mesh_light_ctl_default_pub(struct bt_mesh_light_ctl_srv *srv,
				      struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_DEFAULT_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG);
	default_encode_status(&msg, srv);
	return model_send(srv->model, ctx, &msg);
}
