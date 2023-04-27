/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>
#include <bluetooth/mesh/light_xyl_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "lightness_internal.h"
#include "model_utils.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_light_xyl_srv);

struct bt_mesh_light_xyl_srv_settings_data {
	struct bt_mesh_light_xy default_params;
	struct bt_mesh_light_xy_range range;
#if !IS_ENABLED(CONFIG_EMDS)
	struct bt_mesh_light_xy xy_last;
#endif
} __packed;

#if CONFIG_BT_SETTINGS
static void bt_mesh_light_xyl_srv_pending_store(struct bt_mesh_model *model)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;

	struct bt_mesh_light_xyl_srv_settings_data data = {
		.default_params = srv->xy_default,
		.range = srv->range,
#if !IS_ENABLED(CONFIG_EMDS)
		.xy_last = srv->transient.xy_last,
#endif
	};

	(void)bt_mesh_model_data_store(srv->model, false, NULL, &data,
				       sizeof(data));
}
#endif

static void store_state(struct bt_mesh_light_xyl_srv *srv)
{
#if CONFIG_BT_SETTINGS
	bt_mesh_model_data_store_schedule(srv->model);
#endif
}

static void xyl_get(struct bt_mesh_light_xyl_srv *srv,
		    struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_light_xyl_status *status)
{
	struct bt_mesh_lightness_status lightness = { 0 };
	struct bt_mesh_light_xy_status xy = { 0 };

	srv->lightness_srv->handlers->light_get(srv->lightness_srv, ctx,
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

static int xyl_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_XYL_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LIGHT_XYL_MSG_MAXLEN_SET) {
		return -EMSGSIZE;
	}

	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_xy_set set;
	struct bt_mesh_light_xy_status status = { 0 };
	struct bt_mesh_lightness_status light_rsp = { 0 };
	struct bt_mesh_lightness_set light;
	struct bt_mesh_light_xyl_status xyl_status;

	light.lvl = from_actual(net_buf_simple_pull_le16(buf));
	set.params.x = net_buf_simple_pull_le16(buf);
	set.params.y = net_buf_simple_pull_le16(buf);
	uint8_t tid = net_buf_simple_pull_u8(buf);

	set.params.x = CLAMP(set.params.x, srv->range.min.x, srv->range.max.x);
	set.params.y = CLAMP(set.params.y, srv->range.min.y, srv->range.max.y);

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* If this is the same transaction, we don't need to send it
		 * to the app, but we still have to respond with a status.
		 */
		if (ack) {
			xyl_get(srv, NULL, &xyl_status);
			xyl_rsp(model, ctx, &xyl_status,
				BT_MESH_LIGHT_XYL_OP_STATUS);
			return 0;
		}
	}

	set.transition = model_transition_get(srv->model, &transition, buf);
	light.transition = set.transition;

	lightness_srv_disable_control(srv->lightness_srv);
	lightness_srv_change_lvl(srv->lightness_srv, ctx, &light, &light_rsp, true);
	srv->handlers->xy_set(srv, ctx, &set, &status);
	srv->transient.xy_last.x = set.params.x;
	srv->transient.xy_last.y = set.params.y;
	xyl_status.params.xy = status.current;
	xyl_status.params.lightness = light_rsp.current;
	xyl_status.remaining_time = status.remaining_time;
	if (!IS_ENABLED(CONFIG_EMDS)) {
		store_state(srv);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	(void)bt_mesh_light_xyl_srv_pub(srv, NULL, &xyl_status);

	if (ack) {
		xyl_rsp(model, ctx, &xyl_status, BT_MESH_LIGHT_XYL_OP_STATUS);
	}

	return 0;
}

static int handle_xyl_get(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xyl_status status = { 0 };

	xyl_get(srv, ctx, &status);
	xyl_rsp(model, ctx, &status, BT_MESH_LIGHT_XYL_OP_STATUS);

	return 0;
}

static int handle_xyl_set(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	return xyl_set(model, ctx, buf, true);
}

static int handle_xyl_set_unack(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	return xyl_set(model, ctx, buf, false);
}

static int handle_target_get(struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xy_status status = { 0 };
	struct bt_mesh_lightness_status light = { 0 };

	srv->lightness_srv->handlers->light_get(srv->lightness_srv, ctx,
						&light);
	srv->handlers->xy_get(srv, ctx, &status);

	struct bt_mesh_light_xyl_status xyl_status = {
		.params.xy = status.target,
		.params.lightness = light.target,
		.remaining_time = status.remaining_time,
	};

	xyl_rsp(model, ctx, &xyl_status, BT_MESH_LIGHT_XYL_OP_TARGET_STATUS);

	return 0;
}

static void default_encode_status(struct bt_mesh_light_xyl_srv *srv,
				  struct net_buf_simple *buf)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS);
	net_buf_simple_add_le16(buf, srv->lightness_srv->default_light);
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

static int default_set(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf,
			bool ack)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xy old_default = srv->xy_default;
	uint16_t light = from_actual(net_buf_simple_pull_le16(buf));

	srv->xy_default.x = net_buf_simple_pull_le16(buf);
	srv->xy_default.y = net_buf_simple_pull_le16(buf);
	lightness_srv_default_set(srv->lightness_srv, ctx, light);
	store_state(srv);

	if (srv->handlers->default_update) {
		srv->handlers->default_update(srv, ctx, &old_default,
					      &srv->xy_default);
	}

	(void)bt_mesh_light_xyl_srv_default_pub(srv, NULL);

	if (ack) {
		default_rsp(model, ctx);
	}

	return 0;
}

static int handle_default_get(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	default_rsp(model, ctx);

	return 0;
}

static int handle_default_set(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	return default_set(model, ctx, buf, true);
}

static int handle_default_set_unack(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	return default_set(model, ctx, buf, false);
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

static int range_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
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
		return -EINVAL;
	}

	status_code = BT_MESH_MODEL_SUCCESS;
	old_range = srv->range;
	srv->range = new_range;
	store_state(srv);

	if (srv->handlers->range_update) {
		srv->handlers->range_update(srv, ctx, &old_range, &srv->range);
	}

	(void)bt_mesh_light_xyl_srv_range_pub(srv, NULL, status_code);

	if (ack) {
		range_rsp(model, ctx, status_code);
	}

	return 0;
}

static int handle_range_get(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	range_rsp(model, ctx, BT_MESH_MODEL_SUCCESS);

	return 0;
}

static int handle_range_set(struct bt_mesh_model *model,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	return range_set(model, ctx, buf, true);
}

static int handle_range_set_unack(struct bt_mesh_model *model,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	return range_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_light_xyl_srv_op[] = {
	{
		BT_MESH_LIGHT_XYL_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_XYL_MSG_LEN_GET),
		handle_xyl_get,
	},
	{
		BT_MESH_LIGHT_XYL_OP_SET,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_XYL_MSG_MINLEN_SET),
		handle_xyl_set,
	},
	{
		BT_MESH_LIGHT_XYL_OP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_XYL_MSG_MINLEN_SET),
		handle_xyl_set_unack,
	},
	{
		BT_MESH_LIGHT_XYL_OP_TARGET_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_XYL_MSG_LEN_GET),
		handle_target_get,
	},
	{
		BT_MESH_LIGHT_XYL_OP_DEFAULT_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_XYL_MSG_LEN_GET),
		handle_default_get,
	},
	{
		BT_MESH_LIGHT_XYL_OP_RANGE_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_XYL_MSG_LEN_GET),
		handle_range_get,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_light_xyl_setup_srv_op[] = {
	{
		BT_MESH_LIGHT_XYL_OP_DEFAULT_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT),
		handle_default_set,
	},
	{
		BT_MESH_LIGHT_XYL_OP_DEFAULT_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT),
		handle_default_set_unack,
	},
	{
		BT_MESH_LIGHT_XYL_OP_RANGE_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_SET),
		handle_range_set,
	},
	{
		BT_MESH_LIGHT_XYL_OP_RANGE_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_SET),
		handle_range_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

struct __packed scene_data {
	struct bt_mesh_light_xy xy;
	uint16_t light;
};

static ssize_t scene_store(struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xy_status xy_rsp = { 0 };
	struct bt_mesh_lightness_status light = { 0 };
	struct scene_data *scene = (struct scene_data *)&data[0];

	srv->lightness_srv->handlers->light_get(srv->lightness_srv, NULL, &light);
	srv->handlers->xy_get(srv, NULL, &xy_rsp);

	if (xy_rsp.remaining_time) {
		scene->xy.x = xy_rsp.target.x;
		scene->xy.y = xy_rsp.target.y;
	} else {
		scene->xy.x = xy_rsp.current.x;
		scene->xy.y = xy_rsp.current.y;
	}

	if (light.remaining_time) {
		scene->light = to_actual(light.target);
	} else {
		scene->light = to_actual(light.current);
	}

	return sizeof(struct scene_data);
}

static void scene_recall(struct bt_mesh_model *model, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct scene_data *scene = (struct scene_data *)&data[0];
	struct bt_mesh_light_xy_status xy_status = { 0 };
	struct bt_mesh_light_xy_set xy_set = {
		.params.x = scene->xy.x,
		.params.y = scene->xy.y,
		.transition = transition,
	};

	srv->handlers->xy_set(srv, NULL, &xy_set, &xy_status);
	srv->transient.xy_last.x = xy_set.params.x;
	srv->transient.xy_last.y = xy_set.params.y;
	if (!IS_ENABLED(CONFIG_EMDS)) {
		store_state(srv);
	}

	if (atomic_test_bit(&srv->lightness_srv->flags,
			    LIGHTNESS_SRV_FLAG_EXTENDED_BY_LIGHT_CTRL)) {
		return;
	}

	struct bt_mesh_lightness_status light_status = { 0 };
	struct bt_mesh_lightness_set light = {
		.lvl = from_actual(scene->light),
		.transition = transition,
	};

	lightness_srv_change_lvl(srv->lightness_srv, NULL, &light, &light_status, false);
}

static void scene_recall_complete(struct bt_mesh_model *model)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xyl_status xyl_status = { 0 };
	struct bt_mesh_lightness_status light_status = { 0 };
	struct bt_mesh_light_xy_status xy_status = { 0 };

	srv->handlers->xy_get(srv, NULL, &xy_status);
	srv->lightness_srv->handlers->light_get(srv->lightness_srv, NULL, &light_status);

	xyl_status.params.xy = xy_status.current;
	xyl_status.params.lightness = light_status.current;
	xyl_status.remaining_time = xy_status.remaining_time;

	(void)bt_mesh_light_xyl_srv_pub(srv, NULL, &xyl_status);
}

/*  MeshMDL1.0.1, section 5.1.3.1.1:
 *  If a model is extending another model, the extending model shall determine
 *  the Stored with Scene behavior of that model.
 *
 *  Use Setup Model to handle Scene Store/Recall as it is not extended
 *  by other models.
 */
BT_MESH_SCENE_ENTRY_SIG(light_xyl) = {
	.id.sig = BT_MESH_MODEL_ID_LIGHT_XYL_SETUP_SRV,
	.maxlen = sizeof(struct scene_data),
	.store = scene_store,
	.recall = scene_recall,
	.recall_complete = scene_recall_complete,
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
	struct bt_mesh_model *lightness_srv;

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

#if IS_ENABLED(CONFIG_BT_SETTINGS) && IS_ENABLED(CONFIG_EMDS)
	srv->emds_entry.entry.id = EMDS_MODEL_ID(model);
	srv->emds_entry.entry.data = (uint8_t *)&srv->transient;
	srv->emds_entry.entry.len = sizeof(srv->transient);
	int err = emds_entry_add(&srv->emds_entry);

	if (err) {
		return err;
	}
#endif

	lightness_srv =
		bt_mesh_model_find(bt_mesh_model_elem(model), BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV);

	if (!lightness_srv) {
		LOG_ERR("Failed to find Lightness Server on element");
		return -EINVAL;
	}

	return bt_mesh_model_extend(model, lightness_srv);
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
#if !IS_ENABLED(CONFIG_EMDS)
	srv->transient.xy_last = data.xy_last;
#endif

	return 0;
}

static int bt_mesh_light_xyl_srv_start(struct bt_mesh_model *model)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_light_xy_status status;
	struct bt_mesh_model_transition transition = {
		.time = srv->lightness_srv->ponoff.dtt.transition_time,
		.delay = 0,
	};
	struct bt_mesh_light_xy_set set = {
		.transition = &transition,
	};

	switch (srv->lightness_srv->ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_ON:
		/* Intentional fallthrough */
	case BT_MESH_ON_POWER_UP_OFF:
		set.params = srv->xy_default;
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		set.params = srv->transient.xy_last;
		break;
	default:
		return -EINVAL;
	}

	srv->handlers->xy_set(srv, NULL, &set, &status);

	struct bt_mesh_light_xyl_status xyl_status;

	xyl_get(srv, NULL, &xyl_status);

	/* Ignore error: Will fail if there are no publication parameters, but
	 * it doesn't matter for the startup procedure.
	 */
	(void)bt_mesh_light_xyl_srv_pub(srv, NULL, &xyl_status);
	return 0;
}

static void bt_mesh_light_xyl_srv_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;

	net_buf_simple_reset(srv->pub.msg);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->model, false, NULL, NULL, 0);
	}
}

const struct bt_mesh_model_cb _bt_mesh_light_xyl_srv_cb = {
	.init = bt_mesh_light_xyl_srv_init,
	.start = bt_mesh_light_xyl_srv_start,
	.settings_set = bt_mesh_light_xyl_srv_settings_set,
#if CONFIG_BT_SETTINGS
	.pending_store = bt_mesh_light_xyl_srv_pending_store,
#endif
	.reset = bt_mesh_light_xyl_srv_reset,
};

static int bt_mesh_light_xyl_setup_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_xyl_srv *srv = model->user_data;
	struct bt_mesh_model *lightness_setup_srv;
	int err;

	err = bt_mesh_model_extend(model, srv->model);
	if (err) {
		return err;
	}

	lightness_setup_srv = bt_mesh_model_find(bt_mesh_model_elem(model),
						 BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV);

	if (!lightness_setup_srv) {
		LOG_ERR("Failed to find Lightness Setup Server on element");
		return -EINVAL;
	}

	return bt_mesh_model_extend(model, lightness_setup_srv);
}

const struct bt_mesh_model_cb _bt_mesh_light_xyl_setup_srv_cb = {
	.init = bt_mesh_light_xyl_setup_srv_init,
};

int bt_mesh_light_xyl_srv_pub(struct bt_mesh_light_xyl_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_light_xyl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_MAXLEN_STATUS);
	xyl_encode_status(&msg, status, BT_MESH_LIGHT_XYL_OP_STATUS);
	return bt_mesh_msg_send(srv->model, ctx, &msg);
}

int bt_mesh_light_xyl_srv_target_pub(struct bt_mesh_light_xyl_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     struct bt_mesh_light_xyl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_TARGET_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_MAXLEN_STATUS);
	xyl_encode_status(&msg, status, BT_MESH_LIGHT_XYL_OP_TARGET_STATUS);
	return bt_mesh_msg_send(srv->model, ctx, &msg);
}

int bt_mesh_light_xyl_srv_range_pub(struct bt_mesh_light_xyl_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    enum bt_mesh_model_status status_code)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_RANGE_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_STATUS);
	range_encode_status(&msg, srv, status_code);
	return bt_mesh_msg_send(srv->model, ctx, &msg);
}

int bt_mesh_light_xyl_srv_default_pub(struct bt_mesh_light_xyl_srv *srv,
				      struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_XYL_OP_DEFAULT_STATUS,
				 BT_MESH_LIGHT_XYL_MSG_LEN_DEFAULT);
	default_encode_status(srv, &msg);
	return bt_mesh_msg_send(srv->model, ctx, &msg);
}
