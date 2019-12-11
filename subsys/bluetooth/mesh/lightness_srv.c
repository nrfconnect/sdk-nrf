/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdlib.h>
#include <bluetooth/mesh/lightness_srv.h>
#include "model_utils.h"
#include "lightness_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_light_srv
#include "common/log.h"

#define LVL_TO_LIGHT(_lvl) (repr_to_light((_lvl) + 32768, ACTUAL))
#define LIGHT_TO_LVL(_light) (light_to_repr((_light), ACTUAL) - 32768)

/** Persistent storage handling */
struct bt_mesh_lightness_srv_settings_data {
	struct bt_mesh_lightness_range range;
	u16_t default_light;
	u16_t last;
	bool is_on;
} __packed;

#ifdef BT_DBG_ENABLED
static const char *const repr_str[] = { "Actual", "Linear" };
#endif

static int store_state(struct bt_mesh_lightness_srv *srv)
{
	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return 0;
	}

	struct bt_mesh_lightness_srv_settings_data data = {
		.default_light = srv->default_light,
		.last = srv->last,
		.is_on = atomic_test_bit(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON),
		.range = srv->range,
	};

	BT_DBG("Store: Last: %u Default: %u State: %s Range: [%u - %u]",
	       data.last, data.default_light, data.is_on ? "On" : "Off",
	       data.range.min, data.range.max);

	return bt_mesh_model_data_store(srv->lightness_model, false, &data,
					sizeof(data));
}

static void lvl_status_encode(struct net_buf_simple *buf,
			      const struct bt_mesh_lightness_status *status,
			      enum light_repr repr)
{
	bt_mesh_model_msg_init(buf, repr == ACTUAL ?
					    BT_MESH_LIGHTNESS_OP_STATUS :
					    BT_MESH_LIGHTNESS_OP_LINEAR_STATUS);

	net_buf_simple_add_le16(buf, light_to_repr(status->current, repr));

	if (status->remaining_time == 0) {
		return;
	}

	net_buf_simple_add_le16(buf, light_to_repr(status->target, repr));
	net_buf_simple_add_u8(buf,
			      model_transition_encode(status->remaining_time));
}

static int pub(struct bt_mesh_lightness_srv *srv, struct bt_mesh_msg_ctx *ctx,
	       const struct bt_mesh_lightness_status *status,
	       enum light_repr repr)
{
	BT_DBG("%u -> %u [%u ms]", status->current, status->target,
	       status->remaining_time);

	if (ctx == NULL) {
		/* We'll always make an attempt to publish to the underlying
		 * models as well, since a publish message typically indicates
		 * a status change, which should always be published.
		 * Ignoring the status codes, so that we're able to publish
		 * in the highest model even if the underlying models don't have
		 * publishing configured.
		 */
		struct bt_mesh_lvl_status lvl_status;

		lvl_status.current = LIGHT_TO_LVL(status->current);
		lvl_status.target = LIGHT_TO_LVL(status->target);
		lvl_status.remaining_time = status->remaining_time;
		(void)bt_mesh_lvl_srv_pub(&srv->lvl, NULL, &lvl_status);

		struct bt_mesh_onoff_status onoff_status;

		onoff_status.present_on_off = status->current > 0;
		onoff_status.target_on_off = status->target > 0;
		onoff_status.remaining_time = status->remaining_time;
		(void)bt_mesh_onoff_srv_pub(&srv->ponoff.onoff, NULL,
					    &onoff_status);
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHTNESS_OP_STATUS,
				 BT_MESH_LIGHTNESS_MSG_MAXLEN_STATUS);
	lvl_status_encode(&msg, status, repr);

	return model_send(srv->lightness_model, ctx, &msg);
}

static void transition_get(struct bt_mesh_lightness_srv *srv,
			   struct bt_mesh_model_transition *transition,
			   struct net_buf_simple *buf)
{
	if (buf->len == 2) {
		model_transition_buf_pull(buf, transition);
	} else {
		bt_mesh_dtt_srv_transition_get(srv->lightness_model,
					       transition);
	}
}

static void rsp_lightness_status(struct bt_mesh_model *mod,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_lightness_status *status,
				 enum light_repr repr)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_STATUS,
				 BT_MESH_LIGHTNESS_MSG_MAXLEN_STATUS);
	lvl_status_encode(&rsp, status, repr);

	BT_DBG("Light %s Response: %u -> %u [%u ms]", repr_str[repr],
	       status->current, status->target, status->remaining_time);

	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void handle_light_get(struct bt_mesh_model *mod,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf, enum light_repr repr)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_LEN_GET) {
		return;
	}

	BT_DBG("%s", repr_str[repr]);

	struct bt_mesh_lightness_srv *srv = mod->user_data;
	struct bt_mesh_lightness_status status = { 0 };

	srv->handlers->light_get(srv, ctx, &status);

	rsp_lightness_status(mod, ctx, &status, repr);
}

static void handle_actual_get(struct bt_mesh_model *mod,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	handle_light_get(mod, ctx, buf, ACTUAL);
}

static void handle_linear_get(struct bt_mesh_model *mod,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	handle_light_get(mod, ctx, buf, LINEAR);
}

static void change_lvl(struct bt_mesh_lightness_srv *srv,
		       struct bt_mesh_msg_ctx *ctx,
		       struct bt_mesh_lightness_set *set,
		       struct bt_mesh_lightness_status *status)
{
	if (!bt_mesh_is_provisioned()) {
		/* Avoid picking up Power OnOff loaded onoff, we'll use our own.
		 */
		return;
	}

	bool state_change =
		(atomic_test_bit(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON) ==
		 (set->lvl == 0));

	if (set->lvl != 0) {
		if (set->lvl > srv->range.max) {
			set->lvl = srv->range.max;
		} else if (set->lvl < srv->range.min) {
			set->lvl = srv->range.min;
		}

		state_change |= (srv->last != set->lvl);
		srv->last = set->lvl;
	}

	atomic_set_bit_to(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON, set->lvl > 0);

	BT_DBG("%u [%u + %u ms]", set->lvl, set->transition->delay,
	       set->transition->time);

	if (state_change) {
		store_state(srv);
	}

	memset(status, 0, sizeof(*status));
	srv->handlers->light_set(srv, ctx, set, status);

	BT_DBG("Publishing Light %s to 0x%04x", repr_str[ACTUAL],
	       srv->pub.addr);

	/* Publishing is always done as an Actual, according to test spec. */
	pub(srv, NULL, status, ACTUAL);
}

static void lightness_set(struct bt_mesh_model *mod,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf, bool ack,
			  enum light_repr repr)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LIGHTNESS_MSG_MAXLEN_SET) {
		return;
	}

	struct bt_mesh_lightness_srv *srv = mod->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_lightness_status status;
	struct bt_mesh_lightness_set set;
	u8_t tid;

	set.lvl = repr_to_light(net_buf_simple_pull_le16(buf), repr);
	tid = net_buf_simple_pull_u8(buf);
	transition_get(srv, &transition, buf);
	set.transition = &transition;

	BT_DBG("Light set %s: %u [%u + %u ms]", repr_str[repr], set.lvl,
	       set.transition->delay, set.transition->time);

	if (!tid_check_and_update(&srv->tid, tid, ctx)) {
		change_lvl(srv, ctx, &set, &status);
	} else if (ack) {
		srv->handlers->light_get(srv, NULL, &status);
	}

	if (ack) {
		rsp_lightness_status(mod, ctx, &status, repr);
	}
}

static void handle_actual_set(struct bt_mesh_model *mod,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	lightness_set(mod, ctx, buf, true, ACTUAL);
}

static void handle_actual_set_unack(struct bt_mesh_model *mod,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	lightness_set(mod, ctx, buf, false, ACTUAL);
}

static void handle_linear_set(struct bt_mesh_model *mod,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	lightness_set(mod, ctx, buf, true, LINEAR);
}

static void handle_linear_set_unack(struct bt_mesh_model *mod,
				    struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	lightness_set(mod, ctx, buf, false, LINEAR);
}

static void handle_last_get(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_LEN_LAST_GET) {
		return;
	}

	struct bt_mesh_lightness_srv *srv = mod->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_LAST_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_LAST_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_LAST_STATUS);

	net_buf_simple_add_le16(&rsp, light_to_repr(srv->last, ACTUAL));
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void handle_default_get(struct bt_mesh_model *mod,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_GET) {
		return;
	}

	struct bt_mesh_lightness_srv *srv = mod->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS);

	net_buf_simple_add_le16(&rsp,
				light_to_repr(srv->default_light, ACTUAL));
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void set_default(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET) {
		return;
	}

	struct bt_mesh_lightness_srv *srv = mod->user_data;
	u16_t new = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);

	if (new != srv->default_light) {
		u16_t old = srv->default_light;

		srv->default_light = new;
		if (srv->handlers->default_update) {
			srv->handlers->default_update(srv, ctx, old, new);
		}

		store_state(srv);
	}

	BT_DBG("%u", new);

	if (!ack) {
		return;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS);
	net_buf_simple_add_le16(&rsp, srv->default_light);

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
	if (buf->len != BT_MESH_LIGHTNESS_MSG_LEN_RANGE_GET) {
		return;
	}

	struct bt_mesh_lightness_srv *srv = mod->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_RANGE_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_RANGE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_RANGE_STATUS);

	net_buf_simple_add_u8(&rsp, BT_MESH_MODEL_SUCCESS);
	net_buf_simple_add_le16(&rsp, light_to_repr(srv->range.min, ACTUAL));
	net_buf_simple_add_le16(&rsp, light_to_repr(srv->range.max, ACTUAL));

	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void set_range(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET) {
		return;
	}

	struct bt_mesh_lightness_srv *srv = mod->user_data;
	struct bt_mesh_lightness_range new;

	new.min = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);
	new.max = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);

	/* The test specification doesn't accept changes to the status, even if
	 * the parameters are wrong. Simply ignore messages with wrong
	 * parameters.
	 */
	if (new.min == 0 || new.max == 0 || new.min > new.max) {
		return;
	}

	if (new.min != srv->range.min || new.max != srv->range.max) {
		struct bt_mesh_lightness_range old = srv->range;

		srv->range = new;
		if (srv->handlers->range_update) {
			srv->handlers->range_update(srv, ctx, &old, &new);
		}

		store_state(srv);
	}

	if (!ack) {
		return;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_RANGE_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_RANGE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_RANGE_STATUS);

	net_buf_simple_add_u8(&rsp, BT_MESH_MODEL_SUCCESS);
	net_buf_simple_add_le16(&rsp, light_to_repr(srv->range.min, ACTUAL));
	net_buf_simple_add_le16(&rsp, light_to_repr(srv->range.max, ACTUAL));

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

const struct bt_mesh_model_op _bt_mesh_lightness_srv_op[] = {
	{ BT_MESH_LIGHTNESS_OP_GET, BT_MESH_LIGHTNESS_MSG_LEN_GET,
	  handle_actual_get },
	{ BT_MESH_LIGHTNESS_OP_SET, BT_MESH_LIGHTNESS_MSG_MINLEN_SET,
	  handle_actual_set },
	{ BT_MESH_LIGHTNESS_OP_SET_UNACK, BT_MESH_LIGHTNESS_MSG_MINLEN_SET,
	  handle_actual_set_unack },
	{ BT_MESH_LIGHTNESS_OP_LINEAR_GET, BT_MESH_LIGHTNESS_MSG_LEN_GET,
	  handle_linear_get },
	{ BT_MESH_LIGHTNESS_OP_LINEAR_SET, BT_MESH_LIGHTNESS_MSG_MINLEN_SET,
	  handle_linear_set },
	{ BT_MESH_LIGHTNESS_OP_LINEAR_SET_UNACK,
	  BT_MESH_LIGHTNESS_MSG_MINLEN_SET, handle_linear_set_unack },
	{ BT_MESH_LIGHTNESS_OP_LAST_GET, BT_MESH_LIGHTNESS_MSG_LEN_LAST_GET,
	  handle_last_get },
	{ BT_MESH_LIGHTNESS_OP_DEFAULT_GET,
	  BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_GET, handle_default_get },
	{ BT_MESH_LIGHTNESS_OP_RANGE_GET, BT_MESH_LIGHTNESS_MSG_LEN_RANGE_GET,
	  handle_range_get },
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_lightness_setup_srv_op[] = {
	{ BT_MESH_LIGHTNESS_OP_DEFAULT_SET,
	  BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET, handle_default_set },
	{ BT_MESH_LIGHTNESS_OP_DEFAULT_SET_UNACK,
	  BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET, handle_default_set_unack },
	{ BT_MESH_LIGHTNESS_OP_RANGE_SET, BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET,
	  handle_range_set },
	{ BT_MESH_LIGHTNESS_OP_RANGE_SET_UNACK,
	  BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET, handle_range_set_unack },
	BT_MESH_MODEL_OP_END,
};

static void lvl_get(struct bt_mesh_lvl_srv *lvl_srv,
		    struct bt_mesh_msg_ctx *ctx, struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_lightness_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_lightness_srv, lvl);
	struct bt_mesh_lightness_status status = { 0 };

	srv->handlers->light_get(srv, ctx, &status);

	rsp->current = LIGHT_TO_LVL(status.current);
	rsp->target = LIGHT_TO_LVL(status.target);
	rsp->remaining_time = status.remaining_time;
	BT_DBG("%u -> %u [%u ms]", rsp->current, rsp->target,
	       rsp->remaining_time);
}

static void lvl_set(struct bt_mesh_lvl_srv *lvl_srv,
		    struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_lvl_set *lvl_set,
		    struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_lightness_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_lightness_srv, lvl);
	struct bt_mesh_lightness_set set = {
		.lvl = LVL_TO_LIGHT(lvl_set->lvl),
		.transition = lvl_set->transition,
	};
	struct bt_mesh_lightness_status status = { 0 };

	if (lvl_set->new_transaction) {
		change_lvl(srv, ctx, &set, &status);
	} else if (rsp) {
		srv->handlers->light_get(srv, NULL, &status);
	}

	if (rsp) {
		rsp->current = LIGHT_TO_LVL(status.current);
		rsp->target = LIGHT_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
		BT_DBG("%u -> %u [%u ms]", rsp->current, rsp->target,
		       rsp->remaining_time);
	}
}

static void lvl_delta_set(struct bt_mesh_lvl_srv *lvl_srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_lvl_delta_set *delta_set,
			  struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_lightness_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_lightness_srv, lvl);
	struct bt_mesh_lightness_status status = { 0 };

	u16_t start_value = srv->last;

	if (delta_set->new_transaction) {
		srv->handlers->light_get(srv, NULL, &status);
		start_value = status.current;
	}

	/* Delta lvl is bound to the lightness actual state, so the calculation
	 * must happen in that space:
	 */
	u16_t target_actual =
		light_to_repr(start_value, ACTUAL) + delta_set->delta;

	struct bt_mesh_lightness_set set = {
		/* Converting back to configured space: */
		.lvl = repr_to_light(target_actual, ACTUAL),
		.transition = delta_set->transition,
	};

	change_lvl(srv, ctx, &set, &status);

	/* Override "last" value to be able to make corrective deltas when
	 * new_transaction is false. Note that the "last" value in persistent
	 * storage will still be the target value, allowing us to recover
	 * correctly on power loss.
	 */
	srv->last = start_value;

	if (rsp) {
		rsp->current = LIGHT_TO_LVL(status.current);
		rsp->target = LIGHT_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
	}
}

static void lvl_move_set(struct bt_mesh_lvl_srv *lvl_srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_lvl_move_set *move_set,
			 struct bt_mesh_lvl_status *rsp)
{
	struct bt_mesh_lightness_srv *srv =
		CONTAINER_OF(lvl_srv, struct bt_mesh_lightness_srv, lvl);
	struct bt_mesh_lightness_status status = { 0 };
	u16_t target;

	srv->handlers->light_get(srv, NULL, &status);

	if (move_set->delta > 0) {
		target = srv->range.max;
	} else if (move_set->delta < 0) {
		target = srv->range.min;
	} else {
		target = status.current;
	}

	struct bt_mesh_model_transition transition = { 0 };
	struct bt_mesh_lightness_set set = { .lvl = target,
					     .transition = &transition };

	if (move_set->delta != 0 && move_set->transition) {
		u32_t distance = abs(target - status.current);
		/* Note: We're not actually converting from the lightness actual
		 * space to the linear space here, even if configured. This
		 * means that a generic level server communicating with a
		 * lightness server running in linear space will see the
		 * server's transition as non-linear. The transition time and
		 * end points are unaffected by this.
		 */
		u32_t time_to_edge =
			((u64_t)distance * (u64_t)move_set->transition->time) /
			abs(move_set->delta);

		BT_DBG("Move: distance: %u delta: %u step: %u ms time: %u ms",
		       (u32_t)distance, move_set->delta,
		       move_set->transition->time, time_to_edge);

		if (time_to_edge > 0) {
			transition.delay = move_set->transition->delay;
			transition.time = time_to_edge;
		}
	}

	change_lvl(srv, ctx, &set, &status);

	if (rsp) {
		rsp->current = LIGHT_TO_LVL(status.current);
		rsp->target = LIGHT_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;

		BT_DBG("Move rsp: %u (light: %u) -> %u (light: %u) [%u ms]",
		       rsp->current, status.current, rsp->target, status.target,
		       status.remaining_time);
	}
}

const struct bt_mesh_lvl_srv_handlers _bt_mesh_lightness_srv_lvl_handlers = {
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
	struct bt_mesh_lightness_srv *srv = CONTAINER_OF(
		onoff_srv, struct bt_mesh_lightness_srv, ponoff.onoff);
	struct bt_mesh_lightness_set set = {
		.transition = onoff_set->transition,
	};
	struct bt_mesh_lightness_status status;

	if (onoff_set->on_off) {
		set.lvl = (srv->default_light ? srv->default_light : srv->last);
	} else {
		set.lvl = 0;
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
	struct bt_mesh_lightness_srv *srv = CONTAINER_OF(
		onoff_srv, struct bt_mesh_lightness_srv, ponoff.onoff);
	struct bt_mesh_lightness_status status = { 0 };

	srv->handlers->light_get(srv, ctx, &status);

	rsp->present_on_off = (status.current > 0);
	rsp->remaining_time = status.remaining_time;
	rsp->target_on_off = (status.target > 0);
}

const struct bt_mesh_onoff_srv_handlers
	_bt_mesh_lightness_srv_onoff_handlers = {
		.set = onoff_set,
		.get = onoff_get,
	};

static void bt_mesh_lightness_srv_reset(struct bt_mesh_model *mod)
{
	struct bt_mesh_lightness_srv *srv = mod->user_data;

	srv->range.min = 0;
	srv->range.max = UINT16_MAX;
	srv->default_light = 0;
	srv->last = UINT16_MAX;
	atomic_clear_bit(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON);
}

static int bt_mesh_lightness_srv_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_lightness_srv *srv = mod->user_data;

	srv->lightness_model = mod;
	bt_mesh_lightness_srv_reset(mod);
	net_buf_simple_init(mod->pub->msg, 0);

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		/* Model extensions:
		 * To simplify the model extension tree, we're flipping the
		 * relationship between the lightness server and the lightness
		 * setup server. In the specification, the lightness setup
		 * server extends the lightness server, which is the opposite of
		 * what we're doing here. This makes no difference for the mesh
		 * stack, but it makes it a lot easier to extend this model, as
		 * we won't have to support multiple extenders.
		 */
		bt_mesh_model_extend(mod, srv->ponoff.ponoff_model);
		bt_mesh_model_extend(
			mod,
			bt_mesh_model_find(
				bt_mesh_model_elem(mod),
				BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV));
	}

	return 0;
}

#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_lightness_srv_settings_set(struct bt_mesh_model *mod,
					      size_t len_rd,
					      settings_read_cb read_cb,
					      void *cb_arg)
{
	struct bt_mesh_lightness_srv *srv = mod->user_data;
	struct bt_mesh_lightness_srv_settings_data data;
	ssize_t result;

	result = read_cb(cb_arg, &data, sizeof(data));
	if (result <= 0) {
		return result;
	} else if (result < sizeof(data)) {
		return -EINVAL;
	}

	srv->default_light = data.default_light;
	srv->range = data.range;
	srv->last = data.last;
	atomic_set_bit_to(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON, data.is_on);

	return 0;
}

static int bt_mesh_lightness_srv_start(struct bt_mesh_model *mod)
{
	struct bt_mesh_lightness_srv *srv = mod->user_data;
	struct bt_mesh_lightness_status dummy = { 0 };
	struct bt_mesh_model_transition transition = {
		.time = srv->ponoff.dtt.transition_time,
	};
	struct bt_mesh_lightness_set set = { .transition = &transition };

	if (atomic_test_bit(&srv->flags, LIGHTNESS_SRV_FLAG_NO_START)) {
		return 0;
	}

	switch (srv->ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_OFF:
		return 0;
	case BT_MESH_ON_POWER_UP_ON:
		set.lvl = (srv->default_light ? srv->default_light : srv->last);
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		if (!atomic_test_bit(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON)) {
			return 0;
		}

		set.lvl = srv->last;
		break;
	default:
		return -EINVAL;
	}

	BT_DBG("Loading POnOff: %u -> %u [%u ms]", srv->ponoff.on_power_up,
	       set.lvl, transition.time);

	change_lvl(srv, NULL, &set, &dummy);
	return 0;
}
#endif

const struct bt_mesh_model_cb _bt_mesh_lightness_srv_cb = {
	.init = bt_mesh_lightness_srv_init,
	.reset = bt_mesh_lightness_srv_reset,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = bt_mesh_lightness_srv_settings_set,
	.start = bt_mesh_lightness_srv_start,
#endif
};

int bt_mesh_lightness_srv_pub(struct bt_mesh_lightness_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_lightness_status *status)
{
	return pub(srv, ctx, status, LIGHT_USER_REPR);
}

int _bt_mesh_lightness_srv_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	struct bt_mesh_lightness_status status = { 0 };

	srv->handlers->light_get(srv, NULL, &status);
	BT_DBG("Republishing: %u -> %u [%u ms]", status.current, status.target,
	       status.remaining_time);
	lvl_status_encode(model->pub->msg, &status, LIGHT_USER_REPR);
	return 0;
}
