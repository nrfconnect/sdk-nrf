/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <bluetooth/mesh/gen_plvl_srv.h>
#include "model_utils.h"

#define LVL_TO_POWER(_lvl) ((_lvl) + 32768)
#define POWER_TO_LVL(_power) ((_power)-32768)

/** Persistent storage handling */
struct bt_mesh_plvl_srv_settings_data {
	struct bt_mesh_plvl_range range;
	u16_t default_power;
	u16_t last;
	bool is_on;
} __packed;

static int store_state(struct bt_mesh_plvl_srv *srv)
{
	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return 0;
	}

	struct bt_mesh_plvl_srv_settings_data data = {
		.default_power = srv->default_power,
		.last = srv->last,
		.is_on = srv->is_on,
		.range = srv->range,
	};

	return bt_mesh_model_data_store(srv->plvl_model, false, &data,
					sizeof(data));
}

static void lvl_status_encode(struct net_buf_simple *buf,
			      const struct bt_mesh_plvl_status *status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_PLVL_OP_LEVEL_STATUS);

	net_buf_simple_add_le16(buf, status->current);
	if (status->remaining_time != 0) {
		net_buf_simple_add_le16(buf, status->target);
		net_buf_simple_add_u8(
			buf, model_transition_encode(status->remaining_time));
	}
}

static void transition_get(struct bt_mesh_plvl_srv *srv,
			   struct bt_mesh_model_transition *transition,
			   struct net_buf_simple *buf)
{
	if (buf->len == 2) {
		model_transition_buf_pull(buf, transition);
	} else {
		bt_mesh_dtt_srv_transition_get(srv->plvl_model, transition);
	}
}

static void rsp_plvl_status(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct bt_mesh_plvl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_LEVEL_STATUS,
				 BT_MESH_PLVL_MSG_MAXLEN_LEVEL_STATUS);
	lvl_status_encode(&rsp, status);

	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void handle_lvl_get(struct bt_mesh_model *mod,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_PLVL_MSG_LEN_LEVEL_GET) {
		return;
	}

	struct bt_mesh_plvl_srv *srv = mod->user_data;
	struct bt_mesh_plvl_status status = { 0 };

	srv->handlers->power_get(srv, ctx, &status);

	rsp_plvl_status(mod, ctx, &status);
}

static void change_lvl(struct bt_mesh_plvl_srv *srv,
		       struct bt_mesh_msg_ctx *ctx,
		       struct bt_mesh_plvl_set *set,
		       struct bt_mesh_plvl_status *status)
{
	bool state_change = (srv->is_on == (set->power_lvl == 0));

	if (set->power_lvl != 0) {
		if (set->power_lvl > srv->range.max) {
			set->power_lvl = srv->range.max;
		} else if (set->power_lvl < srv->range.min) {
			set->power_lvl = srv->range.min;
		}

		state_change |= (srv->last != set->power_lvl);
		srv->last = set->power_lvl;
	}

	if (state_change) {
		store_state(srv);
	}

	memset(status, 0, sizeof(*status));
	srv->handlers->power_set(srv, ctx, set, status);
}

static void plvl_set(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_PLVL_MSG_MINLEN_LEVEL_SET &&
	    buf->len != BT_MESH_PLVL_MSG_MAXLEN_LEVEL_SET) {
		return;
	}

	struct bt_mesh_plvl_srv *srv = mod->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_plvl_status status;
	struct bt_mesh_plvl_set set;

	set.power_lvl = net_buf_simple_pull_le16(buf);
	transition_get(srv, &transition, buf);
	set.transition = &transition;

	if (tid_check_and_update(&srv->tid, net_buf_simple_pull_u8(buf), ctx)) {
		srv->handlers->power_get(srv, NULL, &status);
	} else {
		change_lvl(srv, ctx, &set, &status);
	}

	if (ack) {
		rsp_plvl_status(mod, ctx, &status);
	}

	(void)bt_mesh_plvl_srv_pub(srv, NULL, &status);
}

static void handle_plvl_set(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	plvl_set(mod, ctx, buf, true);
}

static void handle_plvl_set_unack(struct bt_mesh_model *mod,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	plvl_set(mod, ctx, buf, false);
}

static void handle_last_get(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_PLVL_MSG_LEN_LAST_GET) {
		return;
	}

	struct bt_mesh_plvl_srv *srv = mod->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_LAST_STATUS,
				 BT_MESH_PLVL_MSG_LEN_LAST_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_LAST_STATUS);

	net_buf_simple_add_le16(&rsp, srv->last);
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void handle_default_get(struct bt_mesh_model *mod,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_PLVL_MSG_LEN_DEFAULT_GET) {
		return;
	}

	struct bt_mesh_plvl_srv *srv = mod->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS,
				 BT_MESH_PLVL_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS);

	net_buf_simple_add_le16(&rsp, srv->default_power);
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void set_default(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_PLVL_MSG_LEN_DEFAULT_SET) {
		return;
	}

	struct bt_mesh_plvl_srv *srv = mod->user_data;
	u8_t new = net_buf_simple_pull_le16(buf);

	if (new != srv->default_power) {
		u8_t old = srv->default_power;

		srv->default_power = new;
		if (srv->handlers->default_update) {
			srv->handlers->default_update(srv, ctx, old, new);
		}

		store_state(srv);
	}

	if (!ack) {
		return;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS,
				 BT_MESH_PLVL_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS);
	net_buf_simple_add_le16(&rsp, srv->default_power);

	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void handle_default_set(struct bt_mesh_model *mod,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	set_default(mod, ctx, buf, true);
}

static void handle_default_set_unack(struct bt_mesh_model *mod,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	set_default(mod, ctx, buf, false);
}

static void handle_range_get(struct bt_mesh_model *mod,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_PLVL_MSG_LEN_RANGE_GET) {
		return;
	}

	struct bt_mesh_plvl_srv *srv = mod->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS,
				 BT_MESH_PLVL_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS);

	net_buf_simple_add_u8(&rsp, BT_MESH_MODEL_SUCCESS);
	net_buf_simple_add_le16(&rsp, srv->range.min);
	net_buf_simple_add_le16(&rsp, srv->range.max);

	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void set_range(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_PLVL_MSG_LEN_RANGE_SET) {
		return;
	}

	struct bt_mesh_plvl_srv *srv = mod->user_data;
	enum bt_mesh_model_status status = BT_MESH_MODEL_SUCCESS;
	struct bt_mesh_plvl_range new;

	new.min = net_buf_simple_pull_le16(buf);
	new.max = net_buf_simple_pull_le16(buf);
	if (new.min != srv->range.min || new.max != srv->range.max) {
		if (new.min > new.max) {
			status = BT_MESH_MODEL_ERROR_INVALID_RANGE_MIN;
		} else {
			struct bt_mesh_plvl_range old = srv->range;

			srv->range = new;
			if (srv->handlers->range_update) {
				srv->handlers->range_update(srv, ctx, &old,
							    &new);
			}

			store_state(srv);
		}
	}

	if (!ack) {
		return;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS,
				 BT_MESH_PLVL_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS);

	net_buf_simple_add_u8(&rsp, status);
	net_buf_simple_add_le16(&rsp, srv->range.min);
	net_buf_simple_add_le16(&rsp, srv->range.max);

	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void handle_range_set(struct bt_mesh_model *mod,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	set_range(mod, ctx, buf, true);
}

static void handle_range_set_unack(struct bt_mesh_model *mod,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	set_range(mod, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_plvl_srv_op[] = {
	{ BT_MESH_PLVL_OP_LEVEL_GET, BT_MESH_PLVL_MSG_LEN_LEVEL_GET,
	  handle_lvl_get },
	{ BT_MESH_PLVL_OP_LEVEL_SET, BT_MESH_PLVL_MSG_MINLEN_LEVEL_SET,
	  handle_plvl_set },
	{ BT_MESH_PLVL_OP_LEVEL_SET_UNACK, BT_MESH_PLVL_MSG_MINLEN_LEVEL_SET,
	  handle_plvl_set_unack },
	{ BT_MESH_PLVL_OP_LAST_GET, BT_MESH_PLVL_MSG_LEN_LAST_GET,
	  handle_last_get },
	{ BT_MESH_PLVL_OP_DEFAULT_GET, BT_MESH_PLVL_MSG_LEN_DEFAULT_GET,
	  handle_default_get },
	{ BT_MESH_PLVL_OP_RANGE_GET, BT_MESH_PLVL_MSG_LEN_RANGE_GET,
	  handle_range_get },
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_plvl_setup_srv_op[] = {
	{ BT_MESH_PLVL_OP_DEFAULT_SET, BT_MESH_PLVL_MSG_LEN_DEFAULT_SET,
	  handle_default_set },
	{ BT_MESH_PLVL_OP_DEFAULT_SET_UNACK, BT_MESH_PLVL_MSG_LEN_DEFAULT_SET,
	  handle_default_set_unack },
	{ BT_MESH_PLVL_OP_RANGE_SET, BT_MESH_PLVL_MSG_LEN_RANGE_SET,
	  handle_range_set },
	{ BT_MESH_PLVL_OP_RANGE_SET_UNACK, BT_MESH_PLVL_MSG_LEN_RANGE_SET,
	  handle_range_set_unack },
	BT_MESH_MODEL_OP_END,
};

static void lvl_get(struct bt_mesh_lvl_srv *lvl_srv,
		    struct bt_mesh_msg_ctx *ctx, struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_plvl_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_plvl_srv, lvl);
	struct bt_mesh_plvl_status status = { 0 };

	srv->handlers->power_get(srv, ctx, &status);

	rsp->current = POWER_TO_LVL(status.current);
	rsp->target = POWER_TO_LVL(status.target);
	rsp->remaining_time = status.remaining_time;
}

static void lvl_set(struct bt_mesh_lvl_srv *lvl_srv,
		    struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_lvl_set *lvl_set,
		    struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_plvl_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_plvl_srv, lvl);
	struct bt_mesh_plvl_set set = {
		.power_lvl = LVL_TO_POWER(lvl_set->lvl),
		.transition = lvl_set->transition,
	};
	struct bt_mesh_plvl_status status;

	change_lvl(srv, ctx, &set, &status);

	if (rsp) {
		rsp->current = POWER_TO_LVL(status.current);
		rsp->target = POWER_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
	}
}

static void lvl_delta_set(struct bt_mesh_lvl_srv *lvl_srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_lvl_delta_set *delta_set,
			  struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_plvl_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_plvl_srv, lvl);
	struct bt_mesh_plvl_status status = { 0 };

	srv->handlers->power_get(srv, NULL, &status);

	struct bt_mesh_plvl_set set = {
		.power_lvl = status.current + delta_set->delta,
		.transition = delta_set->transition,
	};

	change_lvl(srv, ctx, &set, &status);

	if (rsp) {
		rsp->current = POWER_TO_LVL(status.current);
		rsp->target = POWER_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
	}
}

static void lvl_move_set(struct bt_mesh_lvl_srv *lvl_srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_lvl_move_set *move_set,
			 struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_plvl_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_plvl_srv, lvl);
	struct bt_mesh_plvl_status status = { 0 };
	u16_t target;

	srv->handlers->power_get(srv, NULL, &status);

	if (move_set->delta > 0) {
		target = srv->range.max;
	} else if (move_set->delta < 0) {
		target = srv->range.min;
	} else {
		target = status.current;
	}

	struct bt_mesh_plvl_set set = {
		.power_lvl = target,
	};
	struct bt_mesh_model_transition transition;

	if (move_set->delta != 0 && move_set->transition) {
		s32_t distance = target - status.current;

		s32_t time_to_edge = (distance * move_set->transition->time) /
				     move_set->delta;

		if (time_to_edge > 0) {
			transition.delay = move_set->transition->delay;
			transition.time = time_to_edge;
			set.transition = &transition;
		}
	}

	change_lvl(srv, ctx, &set, &status);

	if (rsp) {
		rsp->current = POWER_TO_LVL(status.current);
		rsp->target = POWER_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
	}
}

const struct bt_mesh_lvl_srv_handlers bt_mesh_plvl_srv_lvl_handlers = {
	.get = lvl_get,
	.set = lvl_set,
	.delta_set = lvl_delta_set,
	.move_set = lvl_move_set,
};

static void onoff_set(struct bt_mesh_onoff_srv *onoff_srv,
		      struct bt_mesh_msg_ctx *ctx,
		      const struct bt_mesh_onoff_set *onoff_set,
		      struct bt_mesh_onoff_status *rsp)
{
	struct bt_mesh_plvl_srv *srv =
		CONTAINER_OF(onoff_srv, struct bt_mesh_plvl_srv, ponoff.onoff);
	struct bt_mesh_plvl_set set = {
		.transition = onoff_set->transition,
	};
	struct bt_mesh_plvl_status status;

	if (onoff_set->on_off) {
		set.power_lvl =
			(srv->default_power ? srv->default_power : srv->last);
	} else {
		set.power_lvl = 0;
	}

	change_lvl(srv, ctx, &set, &status);

	if (rsp) {
		rsp->present_on_off = (status.current > 0);
		rsp->remaining_time = status.remaining_time;
		rsp->target_on_off = (status.target > 0);
	}
}

static void onoff_get(struct bt_mesh_onoff_srv *onoff_srv,
		      struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_onoff_status *rsp)
{
	struct bt_mesh_plvl_srv *srv =
		CONTAINER_OF(onoff_srv, struct bt_mesh_plvl_srv, ponoff.onoff);
	struct bt_mesh_plvl_status status = { 0 };

	srv->handlers->power_get(srv, ctx, &status);

	rsp->present_on_off = (status.current > 0);
	rsp->remaining_time = status.remaining_time;
	rsp->target_on_off = (status.target > 0);
}

const struct bt_mesh_onoff_srv_handlers bt_mesh_plvl_srv_onoff_handlers = {
	.set = onoff_set,
	.get = onoff_get,
};

static void bt_mesh_plvl_srv_reset(struct bt_mesh_model *mod)
{
	struct bt_mesh_plvl_srv *srv = mod->user_data;

	srv->range.min = 0;
	srv->range.max = UINT16_MAX;
	srv->default_power = 0;
	srv->last = UINT16_MAX;
	srv->is_on = false;
}

static int bt_mesh_plvl_srv_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_plvl_srv *srv = mod->user_data;

	srv->plvl_model = mod;
	bt_mesh_plvl_srv_reset(mod);
	net_buf_simple_init(mod->pub->msg, 0);

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		/* Model extensions:
		 * To simplify the model extension tree, we're flipping the
		 * relationship between the plvl server and the plvl setup
		 * server. In the specification, the plvl setup server extends
		 * the plvl server, which is the opposite of what we're doing
		 * here. This makes no difference for the mesh stack, but it
		 * makes it a lot easier to extend this model, as we won't have
		 * to support multiple extenders.
		 */
		bt_mesh_model_extend(mod, srv->ponoff.ponoff_model);
		bt_mesh_model_extend(
			mod,
			bt_mesh_model_find(
				bt_mesh_model_elem(mod),
				BT_MESH_MODEL_ID_GEN_POWER_LEVEL_SETUP_SRV));
	}

	return 0;
}

#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_plvl_srv_settings_set(struct bt_mesh_model *mod,
					 size_t len_rd,
					 settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_plvl_srv *srv = mod->user_data;
	struct bt_mesh_plvl_srv_settings_data data;

	if (read_cb(cb_arg, &data, sizeof(data)) != sizeof(data)) {
		return -EINVAL;
	}

	srv->default_power = data.default_power;
	srv->range = data.range;
	srv->last = data.last;
	srv->is_on = data.is_on;

	return 0;
}

static int bt_mesh_plvl_srv_start(struct bt_mesh_model *mod)
{
	struct bt_mesh_plvl_srv *srv = mod->user_data;
	struct bt_mesh_plvl_status dummy = { 0 };
	struct bt_mesh_plvl_set set;

	switch (srv->ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_OFF:
		set.power_lvl = 0;
		break;
	case BT_MESH_ON_POWER_UP_ON:
		set.power_lvl =
			(srv->default_power ? srv->default_power : srv->last);
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		set.power_lvl = srv->is_on ? srv->last : 0;
		break;
	default:
		return -EINVAL;
	}

	srv->handlers->power_set(srv, NULL, &set, &dummy);
	return 0;
}
#endif

const struct bt_mesh_model_cb _bt_mesh_plvl_srv_cb = {
	.init = bt_mesh_plvl_srv_init,
	.reset = bt_mesh_plvl_srv_reset,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = bt_mesh_plvl_srv_settings_set,
	.start = bt_mesh_plvl_srv_start,
#endif
};

int bt_mesh_plvl_srv_pub(struct bt_mesh_plvl_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_plvl_status *status)
{
	lvl_status_encode(srv->pub.msg, status);

	return model_send(srv->plvl_model, ctx, srv->pub.msg);
}

int _bt_mesh_plvl_srv_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_plvl_status status = { 0 };

	srv->handlers->power_get(srv, NULL, &status);
	lvl_status_encode(model->pub->msg, &status);
	return 0;
}
