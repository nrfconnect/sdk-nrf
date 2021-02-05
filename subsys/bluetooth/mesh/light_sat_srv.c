/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <bluetooth/mesh/light_sat_srv.h>
#include <bluetooth/mesh/light_hsl_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "light_hsl_internal.h"
#include "model_utils.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_light_sat_srv
#include "common/log.h"

#define LVL_TO_SAT(_lvl) ((_lvl) + 32768)
#define SAT_TO_LVL(_satur) ((_satur) - 32768)

struct settings_data {
	struct bt_mesh_light_hsl_range range;
	uint16_t last;
	uint16_t dflt;
} __packed;

static int store(struct bt_mesh_light_sat_srv *srv)
{
	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return 0;
	}

	struct settings_data data = {
		.range = srv->range,
		.last = srv->last,
		.dflt = srv->dflt,
	};

	return bt_mesh_model_data_store(srv->model, false, NULL, &data,
					sizeof(data));
}

static void encode_status(struct net_buf_simple *buf,
			  const struct bt_mesh_light_sat_status *status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_SAT_OP_STATUS);
	net_buf_simple_add_le16(buf, status->current);

	if (status->remaining_time != 0) {
		net_buf_simple_add_le16(buf, status->target);
		net_buf_simple_add_u8(
			buf, model_transition_encode(status->remaining_time));
	}
}

static void rsp_status(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *rx_ctx,
		       struct bt_mesh_light_sat_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_SAT_OP_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_SAT_STATUS);
	encode_status(&msg, status);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void sat_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_MINLEN_SAT &&
	    buf->len != BT_MESH_LIGHT_HSL_MSG_MAXLEN_SAT) {
		return;
	}

	struct bt_mesh_light_sat_srv *srv = model->user_data;
	struct bt_mesh_light_sat_status status = { 0 };
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_sat set = {
		.transition = &transition,
	};
	uint8_t tid;

	/* Perform pull and clamp in two steps to avoid duplicate evaluation
	 * in macro:
	 */
	set.lvl = net_buf_simple_pull_le16(buf);
	set.lvl = CLAMP(set.lvl, srv->range.min, srv->range.max);
	tid = net_buf_simple_pull_u8(buf);

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* If this is the same transaction, we don't need to send it
		 * to the app, but we still have to respond with a status.
		 */
		srv->handlers->get(srv, NULL, &status);
		rsp_status(model, ctx, &status);
		return;
	}

	if (buf->len == 2) {
		model_transition_buf_pull(buf, &transition);
	} else if (srv->hsl) {
		/* According to the Bluetooth Mesh Model specification,
		 * section 6.4.8.2.2: The Saturation Server shall use the
		 * Default Transition Time server on the HSL element if no
		 * transition time is set.
		 */
		bt_mesh_dtt_srv_transition_get(srv->hsl->model, &transition);
	} else {
		/* Unspecified behavior in spec: We'll fall back to the
		 * transition server on the server's own element, if any, to
		 * stay consistent with other models.
		 */
		bt_mesh_dtt_srv_transition_get(srv->model, &transition);
	}

	bt_mesh_light_sat_srv_set(srv, ctx, &set, &status);

	struct bt_mesh_lvl_status lvl_status = {
		.current = SAT_TO_LVL(status.current),
		.target = SAT_TO_LVL(status.target),
		.remaining_time = status.remaining_time,
	};

	if (ack) {
		rsp_status(model, ctx, &status);
	}

	(void)bt_mesh_light_sat_srv_pub(srv, NULL, &status);
	(void)bt_mesh_lvl_srv_pub(&srv->lvl, NULL, &lvl_status);
}

static void sat_get_handle(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_GET) {
		return;
	}

	struct bt_mesh_light_sat_srv *srv = model->user_data;
	struct bt_mesh_light_sat_status status = { 0 };

	srv->handlers->get(srv, ctx, &status);
	rsp_status(model, ctx, &status);
}

static void sat_set_handle(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	sat_set(model, ctx, buf, true);
}

static void sat_set_unack_handle(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	sat_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_light_sat_srv_op[] = {
	{
		BT_MESH_LIGHT_SAT_OP_GET,
		BT_MESH_LIGHT_HSL_MSG_LEN_GET,
		sat_get_handle,
	},
	{
		BT_MESH_LIGHT_SAT_OP_SET,
		BT_MESH_LIGHT_HSL_MSG_MINLEN_SAT,
		sat_set_handle,
	},
	{
		BT_MESH_LIGHT_SAT_OP_SET_UNACK,
		BT_MESH_LIGHT_HSL_MSG_MINLEN_SAT,
		sat_set_unack_handle,
	},
	BT_MESH_MODEL_OP_END,
};

static void lvl_get(struct bt_mesh_lvl_srv *lvl_srv,
		    struct bt_mesh_msg_ctx *ctx, struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_light_sat_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_light_sat_srv, lvl);
	struct bt_mesh_light_sat_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);

	rsp->current = SAT_TO_LVL(status.current);
	rsp->target = SAT_TO_LVL(status.target);
	rsp->remaining_time = status.remaining_time;
}

static void lvl_set(struct bt_mesh_lvl_srv *lvl_srv,
		    struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_lvl_set *lvl_set,
		    struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_light_sat_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_light_sat_srv, lvl);
	struct bt_mesh_light_sat set;
	struct bt_mesh_light_sat_status status = { 0 };

	uint16_t sat = LVL_TO_SAT(lvl_set->lvl);

	set.lvl = sat;
	set.transition = lvl_set->transition;
	srv->handlers->set(srv, NULL, &set, &status);
	srv->last = sat;

	(void)bt_mesh_light_sat_srv_pub(srv, NULL, &status);

	if (rsp) {
		rsp->current = SAT_TO_LVL(status.current);
		rsp->target = SAT_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
	}
}

static void lvl_delta_set(struct bt_mesh_lvl_srv *lvl_srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_lvl_delta_set *lvl_delta,
			  struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_light_sat_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_light_sat_srv, lvl);
	struct bt_mesh_light_sat set;
	struct bt_mesh_light_sat_status status = { 0 };
	uint16_t start;

	if (lvl_delta->new_transaction) {
		srv->handlers->get(srv, NULL, &status);
		start = status.current;
	} else {
		start = srv->last;
	}

	set.lvl =
		CLAMP(start + lvl_delta->delta, srv->range.min, srv->range.max);
	set.transition = lvl_delta->transition;
	srv->handlers->set(srv, NULL, &set, &status);
	bt_mesh_light_sat_srv_set(srv, ctx, &set, &status);

	/* Override "last" value to be able to make corrective deltas when
	 * new_transaction is false. Note that the "last" value in persistent
	 * storage will still be the target value, allowing us to recover
	 * correctly on power loss.
	 */
	srv->last = start;

	if (rsp) {
		rsp->current = SAT_TO_LVL(status.current);
		rsp->target = SAT_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
	}
}

static void lvl_move_set(struct bt_mesh_lvl_srv *lvl_srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_lvl_move_set *move_set,
			 struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_light_sat_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_light_sat_srv, lvl);
	struct bt_mesh_light_sat_status status = { 0 };
	uint16_t target;

	srv->handlers->get(srv, NULL, &status);

	if (move_set->delta > 0) {
		target = srv->range.max;
	} else if (move_set->delta < 0) {
		target = srv->range.min;
	} else {
		target = status.current;
	}

	struct bt_mesh_model_transition transition = { 0 };
	struct bt_mesh_light_sat set = { .lvl = target,
					     .transition = &transition };

	if (move_set->delta != 0 && move_set->transition) {
		uint32_t distance = abs(target - status.current);
		/* Note: We're not actually converting from the lightness actual
		 * space to the linear space here, even if configured. This
		 * means that a generic level server communicating with a
		 * lightness server running in linear space will see the
		 * server's transition as non-linear. The transition time and
		 * end points are unaffected by this.
		 */
		uint32_t time_to_edge = ((uint64_t)distance *
					 (uint64_t)move_set->transition->time) /
					abs(move_set->delta);

		BT_DBG("Move: distance: %u delta: %u step: %u ms time: %u ms",
		       (uint32_t)distance, move_set->delta,
		       move_set->transition->time, time_to_edge);

		if (time_to_edge > 0) {
			transition.delay = move_set->transition->delay;
			transition.time = time_to_edge;
		}
	}

	bt_mesh_light_sat_srv_set(srv, ctx, &set, &status);

	if (rsp) {
		rsp->current = SAT_TO_LVL(status.current);
		rsp->target = SAT_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
	}
}

const struct bt_mesh_lvl_srv_handlers _bt_mesh_light_sat_srv_lvl_handlers = {
	.get = lvl_get,
	.set = lvl_set,
	.delta_set = lvl_delta_set,
	.move_set = lvl_move_set,
};

static int sat_srv_pub_update(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_sat_srv *srv = mod->user_data;
	struct bt_mesh_light_sat_status status;

	srv->handlers->get(srv, NULL, &status);

	encode_status(&srv->buf, &status);

	return 0;
}

static int sat_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;

	srv->model = model;
	srv->pub.update = sat_srv_pub_update;
	srv->pub.msg = &srv->buf;
	net_buf_simple_init_with_data(&srv->buf, srv->pub_data,
				      ARRAY_SIZE(srv->pub_data));

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		bt_mesh_model_extend(model, srv->lvl.model);
	}

	return 0;
}

static int sat_srv_settings_set(struct bt_mesh_model *mod, const char *name,
				size_t len_rd, settings_read_cb read_cb,
				void *cb_data)
{
	struct bt_mesh_light_sat_srv *srv = mod->user_data;
	struct settings_data data;
	ssize_t len;

	len = read_cb(cb_data, &data, sizeof(data));
	if (len < sizeof(data)) {
		return -EINVAL;
	}

	srv->range = data.range;
	srv->last = data.last;
	srv->dflt = data.dflt;

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_sat_srv_cb = {
	.init = sat_srv_init,
	.settings_set = sat_srv_settings_set,
};

void bt_mesh_light_sat_srv_set(struct bt_mesh_light_sat_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_light_sat *set,
			       struct bt_mesh_light_sat_status *status)
{
	srv->last = set->lvl;
	srv->handlers->set(srv, ctx, set, status);

	store(srv);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(&srv->lvl.scene);
	}
}

void bt_mesh_light_sat_srv_default_set(struct bt_mesh_light_sat_srv *srv,
				       struct bt_mesh_msg_ctx *ctx,
				       uint16_t dflt)
{
	uint16_t old = srv->dflt;

	srv->dflt = dflt;

	if (srv->handlers->default_update) {
		srv->handlers->default_update(srv, ctx, old, srv->dflt);
	}

	store(srv);
}

void bt_mesh_light_sat_srv_range_set(
	struct bt_mesh_light_sat_srv *srv, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_hsl_range *range)
{
	const struct bt_mesh_light_hsl_range old = srv->range;

	srv->range = *range;

	if (srv->handlers->range_update) {
		srv->handlers->range_update(srv, ctx, &old, &srv->range);
	}

	store(srv);
}

int bt_mesh_light_sat_srv_pub(struct bt_mesh_light_sat_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_light_sat_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_SAT_OP_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_SAT_STATUS);
	encode_status(&msg, status);
	return model_send(srv->model, ctx, &msg);
}
