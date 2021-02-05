/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/light_ctl_srv.h>
#include <bluetooth/mesh/light_temp_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "light_ctl_internal.h"
#include "lightness_internal.h"
#include "model_utils.h"
#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_light_ctl_srv
#include "common/log.h"

static void ctl_encode_status(struct net_buf_simple *buf,
			      struct bt_mesh_light_ctl_status *status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_CTL_STATUS);
	net_buf_simple_add_le16(buf,
				light_to_repr(status->current_light, ACTUAL));
	net_buf_simple_add_le16(buf, status->current_temp);

	if (status->remaining_time) {
		net_buf_simple_add_le16(buf, light_to_repr(status->target_light,
							   ACTUAL));
		net_buf_simple_add_le16(buf, status->target_temp);
		net_buf_simple_add_u8(
			buf, model_transition_encode(status->remaining_time));
	}
}

static void ctl_get(struct bt_mesh_light_ctl_srv *srv,
		    struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_light_ctl_status *status)
{
	struct bt_mesh_light_temp_status temp;
	struct bt_mesh_lightness_status light;

	srv->lightness_srv.handlers->light_get(&srv->lightness_srv, ctx,
					       &light);
	srv->temp_srv.handlers->get(&srv->temp_srv, ctx, &temp);

	status->current_temp = temp.current.temp;
	status->current_light = light.current;
	status->target_temp = temp.target.temp;
	status->target_light = light.target;
	status->remaining_time = temp.remaining_time;
}

static void ctl_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LIGHT_CTL_MSG_MAXLEN_SET) {
		return;
	}

	struct bt_mesh_light_ctl_srv *srv = model->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_lightness_status light_rsp;
	struct bt_mesh_light_temp_status temp_rsp;
	struct bt_mesh_light_ctl_status status;
	struct bt_mesh_lightness_set light = {
		.transition = &transition,
	};
	struct bt_mesh_light_temp_set temp = {
		.transition = &transition,
	};
	uint8_t tid;

	light.lvl = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);
	temp.params.temp = net_buf_simple_pull_le16(buf);
	temp.params.delta_uv = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);

	if ((temp.params.temp < BT_MESH_LIGHT_TEMP_MIN) ||
	    (temp.params.temp > BT_MESH_LIGHT_TEMP_MAX)) {
		return;
	}

	if (light.lvl != 0) {
		light.lvl = CLAMP(light.lvl, srv->lightness_srv.range.min,
				  srv->lightness_srv.range.max);
	}

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* If this is the same transaction, we don't need to send it
		 * to the app, but we still have to respond with a status.
		 */
		ctl_get(srv, ctx, &status);
		goto respond;
	}

	if (buf->len == 2) {
		model_transition_buf_pull(buf, &transition);
	} else {
		bt_mesh_dtt_srv_transition_get(srv->model, &transition);
	}

	lightness_srv_change_lvl(&srv->lightness_srv, ctx, &light, &light_rsp);
	bt_mesh_light_temp_srv_set(&srv->temp_srv, ctx, &temp, &temp_rsp);

	status.current_temp = temp_rsp.current.temp;
	status.current_light = light_rsp.current;
	status.target_temp = temp_rsp.target.temp;
	status.target_light = light_rsp.target;
	status.remaining_time = temp_rsp.remaining_time;

	bt_mesh_light_ctl_pub(srv, NULL, &status);

respond:
	if (ack) {
		bt_mesh_light_ctl_pub(srv, ctx, &status);
	}
}

static void handle_ctl_get(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;
	struct bt_mesh_light_ctl_status status = { 0 };

	ctl_get(srv, ctx, &status);
	bt_mesh_light_ctl_pub(srv, ctx, &status);
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
	net_buf_simple_add_le16(buf, srv->temp_srv.range.min);
	net_buf_simple_add_le16(buf, srv->temp_srv.range.max);
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

static enum bt_mesh_model_status temp_range_set(struct bt_mesh_model *model,
						struct bt_mesh_msg_ctx *ctx,
						struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;
	struct bt_mesh_light_temp_range temp = {
		.min = net_buf_simple_pull_le16(buf),
		.max = net_buf_simple_pull_le16(buf),
	};
	enum bt_mesh_model_status status;

	status = bt_mesh_light_temp_srv_range_set(&srv->temp_srv, ctx, &temp);
	if (status != BT_MESH_MODEL_SUCCESS) {
		return status;
	}

	(void)bt_mesh_light_ctl_range_pub(srv, NULL, status);

	return BT_MESH_MODEL_SUCCESS;
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
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_SET) {
		return;
	}

	temp_range_rsp(model, ctx, temp_range_set(model, ctx, buf));
}

static void handle_temp_range_set_unack(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_SET) {
		return;
	}

	temp_range_set(model, ctx, buf);
}

static void default_encode_status(struct net_buf_simple *buf,
				  struct bt_mesh_light_ctl_srv *srv)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_CTL_DEFAULT_STATUS);
	net_buf_simple_add_le16(
		buf, light_to_repr(srv->lightness_srv.default_light, ACTUAL));
	net_buf_simple_add_le16(buf, srv->temp_srv.dflt.temp);
	net_buf_simple_add_le16(buf, srv->temp_srv.dflt.delta_uv);
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
			struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;
	struct bt_mesh_light_temp temp;
	uint16_t light;

	light = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);
	temp.temp = net_buf_simple_pull_le16(buf);
	temp.delta_uv = net_buf_simple_pull_le16(buf);

	if ((temp.temp < BT_MESH_LIGHT_TEMP_MIN) ||
	    (temp.temp > BT_MESH_LIGHT_TEMP_MAX)) {
		return;
	}

	lightness_srv_default_set(&srv->lightness_srv, ctx, light);
	bt_mesh_light_temp_srv_default_set(&srv->temp_srv, ctx, &temp);

	(void)bt_mesh_light_ctl_default_pub(srv, NULL);
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
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG) {
		return;
	}

	default_set(model, ctx, buf);
	default_rsp(model, ctx);
}

static void handle_default_set_unack(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG) {
		return;
	}

	default_set(model, ctx, buf);
}

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

static int update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;
	struct bt_mesh_light_ctl_status status = { 0 };

	ctl_get(srv, NULL, &status);
	ctl_encode_status(srv->pub.msg, &status);

	return 0;
}

static int bt_mesh_light_ctl_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

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

	return 0;
}

static void bt_mesh_light_ctl_srv_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctl_srv *srv = model->user_data;

	net_buf_simple_reset(srv->pub.msg);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->model, false, NULL, NULL,
					       0);
	}
}

static int bt_mesh_light_ctl_srv_start(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_ctl_srv *srv = mod->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_temp_set temp = {
		.params = srv->temp_srv.dflt,
		.transition = &transition,
	};
	struct bt_mesh_lightness_set light = {
		.lvl = srv->lightness_srv.default_light,
		.transition = &transition,
	};

	if (!srv->temp_srv.model ||
	    (srv->model->elem_idx > srv->temp_srv.model->elem_idx)) {
		BT_ERR("Light CTL srv[%d]: Temp. srv not properly initialized",
		       srv->model->elem_idx);
		return -EINVAL;
	}

	bt_mesh_dtt_srv_transition_get(mod, &transition);

	switch (srv->lightness_srv.ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_OFF:
		bt_mesh_light_temp_srv_set(&srv->temp_srv, NULL, &temp, NULL);
		break;

	case BT_MESH_ON_POWER_UP_ON:
		bt_mesh_light_temp_srv_set(&srv->temp_srv, NULL, &temp, NULL);
		lightness_srv_change_lvl(&srv->lightness_srv, NULL, &light,
					 NULL);
		break;

	case BT_MESH_ON_POWER_UP_RESTORE:
		temp.params = srv->temp_srv.last;
		bt_mesh_light_temp_srv_set(&srv->temp_srv, NULL, &temp, NULL);

		if (!atomic_test_bit(&srv->lightness_srv.flags,
				     LIGHTNESS_SRV_FLAG_IS_ON)) {
			break;
		}

		light.lvl = srv->lightness_srv.last;
		lightness_srv_change_lvl(&srv->lightness_srv, NULL, &light,
					 NULL);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_ctl_srv_cb = {
	.init = bt_mesh_light_ctl_srv_init,
	.reset = bt_mesh_light_ctl_srv_reset,
	.start = bt_mesh_light_ctl_srv_start,
};

int bt_mesh_light_ctl_pub(struct bt_mesh_light_ctl_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_ctl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_STATUS);

	ctl_encode_status(&msg, status);
	return model_send(srv->model, ctx, &msg);
}

int bt_mesh_light_ctl_range_pub(struct bt_mesh_light_ctl_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				enum bt_mesh_model_status status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_TEMP_RANGE_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_LEN_TEMP_RANGE_STATUS);
	range_encode_status(&msg, srv, status);
	return model_send(srv->model, ctx, &msg);
}

int bt_mesh_light_ctl_default_pub(struct bt_mesh_light_ctl_srv *srv,
				  struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_CTL_DEFAULT_STATUS,
				 BT_MESH_LIGHT_CTL_MSG_LEN_DEFAULT_MSG);
	default_encode_status(&msg, srv);
	return model_send(srv->model, ctx, &msg);
}
