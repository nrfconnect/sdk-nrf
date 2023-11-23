/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/sys/byteorder.h>
#include <bluetooth/mesh/light_sat_srv.h>
#include <bluetooth/mesh/light_hsl_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "light_hsl_internal.h"
#include "model_utils.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_light_sat_srv);

#define LVL_TO_SAT(_lvl) ((_lvl) + 32768)
#define SAT_TO_LVL(_satur) ((_satur) - 32768)

struct settings_data {
	struct bt_mesh_light_hsl_range range;
	uint16_t dflt;
#if !IS_ENABLED(CONFIG_EMDS)
	uint16_t last;
#endif
} __packed;

#if CONFIG_BT_SETTINGS
static void sat_srv_pending_store(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;

	struct settings_data data = {
		.range = srv->range,
		.dflt = srv->dflt,
#if !IS_ENABLED(CONFIG_EMDS)
		.last = srv->transient.last,
#endif
	};

	(void)bt_mesh_model_data_store(srv->model, false, NULL, &data,
				       sizeof(data));
}
#endif

static void store(struct bt_mesh_light_sat_srv *srv)
{
#if CONFIG_BT_SETTINGS
	bt_mesh_model_data_store_schedule(srv->model);
#endif
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

static int sat_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_MINLEN_SAT &&
	    buf->len != BT_MESH_LIGHT_HSL_MSG_MAXLEN_SAT) {
		return -EMSGSIZE;
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
	tid = net_buf_simple_pull_u8(buf);

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* If this is the same transaction, we don't need to send it
		 * to the app, but we still have to respond with a status.
		 */
		srv->handlers->get(srv, NULL, &status);
		(void)bt_mesh_light_sat_srv_pub(srv, ctx, &status);
		return 0;
	}

	if (buf->len == 2) {
		model_transition_buf_pull(buf, &transition);
	} else if (srv->hsl) {
		/* According to MshMDLv1.1: 6.4.8.2.2: The Saturation Server shall use the Default
		 * Transition Time server on the HSL element if no transition time is set.
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
		(void)bt_mesh_light_sat_srv_pub(srv, ctx, &status);
	}

	(void)bt_mesh_light_sat_srv_pub(srv, NULL, &status);
	(void)bt_mesh_lvl_srv_pub(&srv->lvl, NULL, &lvl_status);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	return 0;
}

static int handle_sat_get(const struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;
	struct bt_mesh_light_sat_status status = { 0 };

	srv->handlers->get(srv, ctx, &status);
	(void)bt_mesh_light_sat_srv_pub(srv, ctx, &status);

	return 0;
}

static int handle_sat_set(const struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	return sat_set(model, ctx, buf, true);
}

static int handle_sat_set_unack(const struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	return sat_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_light_sat_srv_op[] = {
	{
		BT_MESH_LIGHT_SAT_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_HSL_MSG_LEN_GET),
		handle_sat_get,
	},
	{
		BT_MESH_LIGHT_SAT_OP_SET,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_HSL_MSG_MINLEN_SAT),
		handle_sat_set,
	},
	{
		BT_MESH_LIGHT_SAT_OP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_HSL_MSG_MINLEN_SAT),
		handle_sat_set_unack,
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
	bt_mesh_light_sat_srv_set(srv, ctx, &set, &status);

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
		start = srv->transient.last;
	}

	set.lvl =
		CLAMP(start + lvl_delta->delta, srv->range.min, srv->range.max);
	set.transition = lvl_delta->transition;
	bt_mesh_light_sat_srv_set(srv, ctx, &set, &status);

	/* Override "last" value to be able to make corrective deltas when
	 * new_transaction is false. Note that the "last" value in persistent
	 * storage will still be the target value, allowing us to recover
	 * correctly on power loss.
	 */
	srv->transient.last = start;

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

	struct bt_mesh_light_sat set = { .lvl = target, .transition = NULL };
	struct bt_mesh_model_transition transition;

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

		LOG_DBG("Move: distance: %u delta: %u step: %u ms time: %u ms",
		       (uint32_t)distance, move_set->delta,
		       move_set->transition->time, time_to_edge);

		if (time_to_edge > 0) {
			transition.delay = move_set->transition->delay;
			transition.time = time_to_edge;
			set.transition = &transition;
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

static ssize_t scene_store(const struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;
	struct bt_mesh_light_sat_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);
	sys_put_le16(status.remaining_time ? status.target : status.current,
		     &data[0]);

	return 2;
}

static void scene_recall(const struct bt_mesh_model *model, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;
	struct bt_mesh_light_sat_status status = { 0 };
	struct bt_mesh_light_sat set = {
		.lvl = sys_get_le16(data),
		.transition = transition,
	};

	bt_mesh_light_sat_srv_set(srv, NULL, &set, &status);
}

static void scene_recall_complete(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;
	struct bt_mesh_light_sat_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);

	(void)bt_mesh_light_sat_srv_pub(srv, NULL, &status);
}

BT_MESH_SCENE_ENTRY_SIG(light_hue) = {
	.id.sig = BT_MESH_MODEL_ID_LIGHT_HSL_SAT_SRV,
	.maxlen = 2,
	.store = scene_store,
	.recall = scene_recall,
	.recall_complete = scene_recall_complete,
};

static int sat_srv_pub_update(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;
	struct bt_mesh_light_sat_status status;

	srv->handlers->get(srv, NULL, &status);

	encode_status(&srv->buf, &status);

	return 0;
}

static int sat_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;
	int err;

	srv->model = model;
	srv->pub.update = sat_srv_pub_update;
	srv->pub.msg = &srv->buf;
	net_buf_simple_init_with_data(&srv->buf, srv->pub_data,
				      ARRAY_SIZE(srv->pub_data));

#if IS_ENABLED(CONFIG_BT_SETTINGS) && IS_ENABLED(CONFIG_EMDS)
	srv->emds_entry.entry.id = EMDS_MODEL_ID(model);
	srv->emds_entry.entry.data = (uint8_t *)&srv->transient;
	srv->emds_entry.entry.len = sizeof(srv->transient);
	err = emds_entry_add(&srv->emds_entry);

	if (err) {
		return err;
	}
#endif

#if defined(CONFIG_BT_MESH_COMP_PAGE_1)
	err = bt_mesh_model_correspond(srv->hsl->model, model);

	if (err) {
		return err;
	}
#endif

	err = bt_mesh_model_extend(model, srv->lvl.model);
	return err;
}

static int sat_srv_settings_set(const struct bt_mesh_model *model, const char *name,
				size_t len_rd, settings_read_cb read_cb,
				void *cb_data)
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;
	struct settings_data data;
	ssize_t len;

	len = read_cb(cb_data, &data, sizeof(data));
	if (len < sizeof(data)) {
		return -EINVAL;
	}

	srv->range = data.range;
	srv->dflt = data.dflt;
#if !IS_ENABLED(CONFIG_EMDS)
	srv->transient.last = data.last;
#endif

	return 0;
}

static void sat_srv_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_sat_srv *srv = model->user_data;

	srv->range.min = BT_MESH_LIGHT_HSL_MIN;
	srv->range.max = BT_MESH_LIGHT_HSL_MAX;
	srv->transient.last = 0;
	srv->dflt = 0;

	net_buf_simple_reset(srv->pub.msg);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->model, false, NULL, NULL, 0);
	}
}

const struct bt_mesh_model_cb _bt_mesh_light_sat_srv_cb = {
	.init = sat_srv_init,
	.settings_set = sat_srv_settings_set,
	.reset = sat_srv_reset,
#if CONFIG_BT_SETTINGS
	.pending_store = sat_srv_pending_store,
#endif
};

void bt_mesh_light_sat_srv_set(struct bt_mesh_light_sat_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_light_sat *set,
			       struct bt_mesh_light_sat_status *status)
{
	set->lvl = CLAMP(set->lvl, srv->range.min, srv->range.max);

	srv->transient.last = set->lvl;
	srv->handlers->set(srv, ctx, set, status);

	if (!IS_ENABLED(CONFIG_EMDS)) {
		store(srv);
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
	return bt_mesh_msg_send(srv->model, ctx, &msg);
}
