/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/sys/byteorder.h>
#include <bluetooth/mesh/light_temp_srv.h>
#include <bluetooth/mesh/light_ctl_srv.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "light_ctl_internal.h"
#include "model_utils.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_light_temp_srv);

struct settings_data {
	struct bt_mesh_light_temp dflt;
	struct bt_mesh_light_temp_range range;
#if !IS_ENABLED(CONFIG_EMDS)
	struct bt_mesh_light_temp last;
#endif
} __packed;

#if CONFIG_BT_SETTINGS
static void bt_mesh_light_temp_srv_pending_store(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_temp_srv *srv = model->rt->user_data;

	struct settings_data data = {
		.dflt = srv->dflt,
		.range = srv->range,
#if !IS_ENABLED(CONFIG_EMDS)
		.last = srv->transient.last,
#endif
	};

	(void)bt_mesh_model_data_store(srv->model, false, NULL, &data,
				       sizeof(data));

}
#endif

static void store_state(struct bt_mesh_light_temp_srv *srv)
{
#if CONFIG_BT_SETTINGS
	bt_mesh_model_data_store_schedule(srv->model);
#endif
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

static int temp_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_SET &&
	    buf->len != BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_SET) {
		return -EMSGSIZE;
	}

	struct bt_mesh_light_temp_srv *srv = model->rt->user_data;
	struct bt_mesh_light_temp_status status = { 0 };
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_temp_set set;

	set.params.temp = net_buf_simple_pull_le16(buf);
	set.params.delta_uv = net_buf_simple_pull_le16(buf);
	uint8_t tid = net_buf_simple_pull_u8(buf);

	if ((set.params.temp < BT_MESH_LIGHT_TEMP_MIN) ||
	    (set.params.temp > BT_MESH_LIGHT_TEMP_MAX)) {
		return -EINVAL;
	}

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* If this is the same transaction, we don't need to send it
		 * to the app, but we still have to respond with a status.
		 */
		srv->handlers->get(srv, NULL, &status);
		goto respond;
	}

	set.transition = model_transition_get(srv->model, &transition, buf);

	bt_mesh_light_temp_srv_set(srv, ctx, &set, &status);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

respond:
	if (ack) {
		(void)bt_mesh_light_temp_srv_pub(srv, ctx, &status);
	}

	return 0;
}

static int handle_temp_get(const struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_light_temp_srv *srv = model->rt->user_data;
	struct bt_mesh_light_temp_status status = { 0 };

	srv->handlers->get(srv, ctx, &status);
	(void)bt_mesh_light_temp_srv_pub(srv, ctx, &status);

	return 0;
}

static int handle_temp_set(const struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	return temp_set(model, ctx, buf, true);
}

static int handle_temp_set_unack(const struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	return temp_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_light_temp_srv_op[] = {
	{
		BT_MESH_LIGHT_TEMP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTL_MSG_LEN_GET),
		handle_temp_get,
	},
	{
		BT_MESH_LIGHT_TEMP_SET,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_SET),
		handle_temp_set,
	},
	{
		BT_MESH_LIGHT_TEMP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTL_MSG_MINLEN_TEMP_SET),
		handle_temp_set_unack,
	},
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
			.delta_uv = srv->transient.last.delta_uv,
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
		.params = srv->transient.last,
		.transition = delta_set->transition,
	};
	int16_t start_lvl, target_lvl;

	if (delta_set->new_transaction) {
		srv->handlers->get(srv, NULL, &status);
		start_lvl = temp_to_lvl(srv, status.current.temp);
	} else {
		start_lvl = temp_to_lvl(srv, srv->corrective_delta);
	}

	/* Clamp to int16_t range before storing the value in a 16 bit integer
	 * to prevent overflow.
	 */
	target_lvl = CLAMP(start_lvl + delta_set->delta, BT_MESH_LVL_MIN,
			   BT_MESH_LVL_MAX);

	set.params.temp = lvl_to_temp(srv, target_lvl);

	bt_mesh_light_temp_srv_set(srv, ctx, &set, &status);

	/* Copy start level to be able to make corrective deltas when
	 * new_transaction is false.
	 */
	srv->corrective_delta = lvl_to_temp(srv, start_lvl);

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
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_temp_set set = {
		.params = srv->transient.last,
		.transition = NULL,
	};

	srv->handlers->get(srv, NULL, &status);

	if (move_set->delta > 0) {
		set.params.temp = get_range_max(srv->range.max);
	} else if (move_set->delta < 0) {
		set.params.temp = get_range_min(srv->range.min);
	} else {
		set.params.temp = status.current.temp;
	}

	if (move_set->delta != 0 && move_set->transition) {
		/* Calculate the transition time to edge in the Generic Level space: */
		int32_t lvl_target = (move_set->delta > 0 ? INT16_MAX : INT16_MIN);
		int64_t lvl_delta = lvl_target - temp_to_lvl(srv, status.current.temp);
		uint32_t time_to_edge = (move_set->transition->time * lvl_delta) / move_set->delta;

		if (time_to_edge > 0) {
			transition.time = time_to_edge;
			transition.delay = move_set->transition->delay;
			set.transition = &transition;
		}
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

struct __packed scene_data {
	uint16_t temp;
	int16_t delta_uv;
};

static ssize_t scene_store(const struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_light_temp_srv *srv = model->rt->user_data;
	struct bt_mesh_light_temp_status status = { 0 };
	struct scene_data *scene = (struct scene_data *)&data[0];

	srv->handlers->get(srv, NULL, &status);

	if (status.remaining_time) {
		scene->temp = status.target.temp;
		scene->delta_uv = status.target.delta_uv;
	} else {
		scene->temp = status.current.temp;
		scene->delta_uv = status.current.delta_uv;
	}

	return sizeof(struct scene_data);
}

static void temp_srv_set(struct bt_mesh_light_temp_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 struct bt_mesh_light_temp_set *set,
			 struct bt_mesh_light_temp_status *status)
{
	uint16_t min = get_range_min(srv->range.min);
	uint16_t max = get_range_max(srv->range.max);

	set->params.temp = CLAMP(set->params.temp, min, max);

	srv->transient.last = set->params;
	if (!IS_ENABLED(CONFIG_EMDS)) {
		store_state(srv);
	}

	srv->handlers->set(srv, ctx, set, status);
}

static void scene_recall(const struct bt_mesh_model *model, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_light_temp_srv *srv = model->rt->user_data;
	struct scene_data *scene = (struct scene_data *)&data[0];
	struct bt_mesh_light_temp_status status = { 0 };
	struct bt_mesh_light_temp_set set = {
		.params = {
			.temp = scene->temp,
			.delta_uv = scene->delta_uv,
		},
		.transition = transition,
	};

	temp_srv_set(srv, NULL, &set, &status);
}

static void scene_recall_complete(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_temp_srv *srv = model->rt->user_data;
	struct bt_mesh_light_temp_status status = { 0 };

	srv->handlers->get(srv, NULL, &status);

	(void)bt_mesh_light_temp_srv_pub(srv, NULL, &status);
}

BT_MESH_SCENE_ENTRY_SIG(light_temp) = {
	.id.sig = BT_MESH_MODEL_ID_LIGHT_CTL_TEMP_SRV,
	.store = scene_store,
	.recall = scene_recall,
	.recall_complete = scene_recall_complete,
	.maxlen = sizeof(struct scene_data),
};

static void light_temp_srv_reset(struct bt_mesh_light_temp_srv *srv)
{
	srv->dflt.delta_uv = 0;
	srv->dflt.temp = BT_MESH_LIGHT_TEMP_MIN;
	srv->transient.last.delta_uv = 0;
	srv->transient.last.temp = BT_MESH_LIGHT_TEMP_MIN;
	srv->range.min = BT_MESH_LIGHT_TEMP_MIN;
	srv->range.max = BT_MESH_LIGHT_TEMP_MAX;
}

static int bt_mesh_light_temp_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_temp_srv *srv = model->rt->user_data;
	int err;

	srv->model = model;
	light_temp_srv_reset(srv);
	net_buf_simple_init(srv->pub.msg, 0);

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
	err = bt_mesh_model_correspond(srv->ctl->model, model);

	if (err) {
		return err;
	}
#endif

	err = bt_mesh_model_extend(model, srv->lvl.model);
	return err;
}

static int bt_mesh_light_temp_srv_settings_set(const struct bt_mesh_model *model,
					       const char *name, size_t len_rd,
					       settings_read_cb read_cb,
					       void *cb_data)
{
	struct bt_mesh_light_temp_srv *srv = model->rt->user_data;
	struct settings_data data;
	ssize_t len;

	len = read_cb(cb_data, &data, sizeof(data));
	if (len < sizeof(data)) {
		return -EINVAL;
	}

	srv->dflt = data.dflt;
	srv->range = data.range;
#if !IS_ENABLED(CONFIG_EMDS)
	srv->transient.last = data.last;
#endif

	return 0;
}

static void bt_mesh_light_temp_srv_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_temp_srv *srv = model->rt->user_data;

	light_temp_srv_reset(srv);
	net_buf_simple_reset(srv->pub.msg);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->model, false, NULL, NULL, 0);
	}
}

const struct bt_mesh_model_cb _bt_mesh_light_temp_srv_cb = {
	.init = bt_mesh_light_temp_srv_init,
	.reset = bt_mesh_light_temp_srv_reset,
	.settings_set = bt_mesh_light_temp_srv_settings_set,
#if CONFIG_BT_SETTINGS
	.pending_store = bt_mesh_light_temp_srv_pending_store,
#endif
};

void bt_mesh_light_temp_srv_set(struct bt_mesh_light_temp_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_light_temp_set *set,
				struct bt_mesh_light_temp_status *rsp)
{
	temp_srv_set(srv, ctx, set, rsp);

	(void)bt_mesh_light_temp_srv_pub(srv, NULL, rsp);

	struct bt_mesh_lvl_status lvl_status = {
		.current = temp_to_lvl(srv, rsp->current.temp),
		.target = temp_to_lvl(srv, rsp->target.temp),
		.remaining_time = rsp->remaining_time,
	};

	(void)bt_mesh_lvl_srv_pub(&srv->lvl, NULL, &lvl_status);
}

enum bt_mesh_model_status bt_mesh_light_temp_srv_range_set(struct bt_mesh_light_temp_srv *srv,
							   struct bt_mesh_msg_ctx *ctx,
							   struct bt_mesh_light_temp_range *range)
{
	const struct bt_mesh_light_temp_range old = srv->range;

	if (range->min < BT_MESH_LIGHT_TEMP_MIN) {
		return BT_MESH_MODEL_ERROR_INVALID_RANGE_MIN;
	}

	if (range->max > BT_MESH_LIGHT_TEMP_MAX && range->max != BT_MESH_LIGHT_TEMP_UNKNOWN) {
		return BT_MESH_MODEL_ERROR_INVALID_RANGE_MAX;
	}

	if (range->max != BT_MESH_LIGHT_TEMP_UNKNOWN && range->min != BT_MESH_LIGHT_TEMP_UNKNOWN &&
	    range->min > range->max) {
		return BT_MESH_MODEL_ERROR_INVALID_RANGE_MIN;
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
	return bt_mesh_msg_send(srv->model, ctx, &msg);
}
