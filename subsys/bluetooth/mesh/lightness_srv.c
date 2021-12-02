/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdlib.h>
#include <sys/byteorder.h>
#include <bluetooth/mesh/lightness_srv.h>
#include "model_utils.h"
#include "lightness_internal.h"
#include <bluetooth/mesh/light_ctrl_srv.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_light_srv
#include "common/log.h"

#define LVL_TO_LIGHT(_lvl) (from_actual((_lvl) + 32768))
#define LIGHT_TO_LVL(_light) (to_actual((_light)) - 32768)

/** Persistent storage handling */
struct bt_mesh_lightness_srv_settings_data {
	struct bt_mesh_lightness_range range;
	uint16_t default_light;
	uint16_t last;
	bool is_on;
} __packed;

#ifdef BT_DBG_ENABLED
static const char *const repr_str[] = { "Actual", "Linear" };
#endif

#if CONFIG_BT_SETTINGS
static void store_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_lightness_srv *srv = CONTAINER_OF(
		dwork, struct bt_mesh_lightness_srv, store_timer);

	struct bt_mesh_lightness_srv_settings_data data = {
		.default_light = srv->default_light,
		.last = srv->last,
		.is_on = atomic_test_bit(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON),
		.range = srv->range,
	};

	BT_DBG("Store: Last: %u Default: %u State: %s Range: [%u - %u]",
	       data.last, data.default_light, data.is_on ? "On" : "Off",
	       data.range.min, data.range.max);

	(void)bt_mesh_model_data_store(srv->lightness_model, false, NULL,
				       &data, sizeof(data));
}
#endif

static void store_state(struct bt_mesh_lightness_srv *srv)
{
#if CONFIG_BT_SETTINGS
	k_work_schedule(
		&srv->store_timer,
		K_SECONDS(CONFIG_BT_MESH_MODEL_SRV_STORE_TIMEOUT));
#endif
}

static void lvl_status_encode(struct net_buf_simple *buf,
			      const struct bt_mesh_lightness_status *status,
			      enum light_repr repr)
{
	bt_mesh_model_msg_init(buf, op_get(LIGHTNESS_OP_TYPE_STATUS, repr));

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

static void rsp_lightness_status(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_lightness_status *status,
				 enum light_repr repr)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_STATUS,
				 BT_MESH_LIGHTNESS_MSG_MAXLEN_STATUS);
	lvl_status_encode(&rsp, status, repr);

	BT_DBG("Light %s Response: %u -> %u [%u ms]", repr_str[repr],
		light_to_repr(status->current, repr),
		light_to_repr(status->target, repr),
		status->remaining_time);

	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);
}

static int handle_light_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf, enum light_repr repr)
{
	BT_DBG("%s", repr_str[repr]);

	struct bt_mesh_lightness_srv *srv = model->user_data;
	struct bt_mesh_lightness_status status = { 0 };

	srv->handlers->light_get(srv, ctx, &status);

	rsp_lightness_status(model, ctx, &status, repr);

	return 0;
}

static int handle_actual_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	return handle_light_get(model, ctx, buf, ACTUAL);
}

static int handle_linear_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	return handle_light_get(model, ctx, buf, LINEAR);
}

void lightness_srv_disable_control(struct bt_mesh_lightness_srv *srv)
{
#if defined(CONFIG_BT_MESH_LIGHT_CTRL_SRV)
	if (srv->ctrl) {
		bt_mesh_light_ctrl_srv_disable(srv->ctrl);
	}
#endif
}

void bt_mesh_lightness_srv_set(struct bt_mesh_lightness_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_lightness_set *set,
			       struct bt_mesh_lightness_status *status)
{
	if (bt_mesh_model_transition_time(set->transition)) {
		BT_DBG("%u [%u + %u ms]", set->lvl, set->transition->delay,
		       set->transition->time);
	} else {
		BT_DBG("%u", set->lvl);
	}

	memset(status, 0, sizeof(*status));

	if (set->lvl != 0) {
		set->lvl = CLAMP(set->lvl, srv->range.min, srv->range.max);
	}

	atomic_set_bit_to(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON, set->lvl > 0);

	srv->handlers->light_set(srv, ctx, set, status);
}

void lightness_srv_change_lvl(struct bt_mesh_lightness_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_lightness_set *set,
			      struct bt_mesh_lightness_status *status,
			      bool publish)
{
	bool state_change =
		(atomic_test_bit(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON) ==
		 (set->lvl == 0));

	bt_mesh_lightness_srv_set(srv, ctx, set, status);

	if (set->lvl != 0) {
		state_change |= (srv->last != set->lvl);
		srv->last = set->lvl;
	}

	if (state_change) {
		store_state(srv);
	}

	if (publish) {
		BT_DBG("Publishing Light %s to 0x%04x", repr_str[ACTUAL], srv->pub.addr);

		/* Publishing is always done as an Actual, according to test spec. */
		pub(srv, NULL, status, ACTUAL);
	}
}

static int lightness_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf, bool ack, enum light_repr repr)
{
	if (buf->len != BT_MESH_LIGHTNESS_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LIGHTNESS_MSG_MAXLEN_SET) {
		return -EMSGSIZE;
	}

	struct bt_mesh_lightness_srv *srv = model->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_lightness_status status;
	struct bt_mesh_lightness_set set;
	uint8_t tid;

	set.lvl = repr_to_light(net_buf_simple_pull_le16(buf), repr);
	tid = net_buf_simple_pull_u8(buf);
	set.transition = model_transition_get(model, &transition, buf);

	if (bt_mesh_model_transition_time(set.transition)) {
		BT_DBG("Light set %s: %u [%u + %u ms]", repr_str[repr], set.lvl,
		set.transition->delay, set.transition->time);
	} else {
		BT_DBG("Light set %s: %u", repr_str[repr], set.lvl);
	}

	if (!tid_check_and_update(&srv->tid, tid, ctx)) {
		/* According to the Mesh Model Specification section 6.2.3.1,
		 * manual changes to the lightness should disable control.
		 */
		lightness_srv_disable_control(srv);
		lightness_srv_change_lvl(srv, ctx, &set, &status, true);

		if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
			bt_mesh_scene_invalidate(srv->lightness_model);
		}
	} else if (ack) {
		srv->handlers->light_get(srv, NULL, &status);
	}

	if (ack) {
		rsp_lightness_status(model, ctx, &status, repr);
	}

	return 0;
}

static int handle_actual_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	return lightness_set(model, ctx, buf, true, ACTUAL);
}

static int handle_actual_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	return lightness_set(model, ctx, buf, false, ACTUAL);
}

static int handle_linear_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	return lightness_set(model, ctx, buf, true, LINEAR);
}

static int handle_linear_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	return lightness_set(model, ctx, buf, false, LINEAR);
}

static int handle_last_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_LAST_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_LAST_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_LAST_STATUS);

	net_buf_simple_add_le16(&rsp, to_actual(srv->last));
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_default_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS);

	net_buf_simple_add_le16(&rsp, to_actual(srv->default_light));
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

void lightness_srv_default_set(struct bt_mesh_lightness_srv *srv,
			       struct bt_mesh_msg_ctx *ctx, uint16_t set)
{
	uint16_t old = srv->default_light;

	if (set == old) {
		return;
	}

	BT_DBG("%u", set);

	srv->default_light = set;
	if (srv->handlers->default_update) {
		srv->handlers->default_update(srv, ctx, old, set);
	}

	store_state(srv);
}

static int set_default(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	uint16_t new = from_actual(net_buf_simple_pull_le16(buf));

	lightness_srv_default_set(srv, ctx, new);
	if (!ack) {
		return 0;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_DEFAULT_STATUS);
	net_buf_simple_add_le16(&rsp, srv->default_light);

	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_default_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	return set_default(model, ctx, buf, true);
}

static int handle_default_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	return set_default(model, ctx, buf, false);
}

static int handle_range_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_RANGE_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_RANGE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_RANGE_STATUS);

	net_buf_simple_add_u8(&rsp, BT_MESH_MODEL_SUCCESS);
	net_buf_simple_add_le16(&rsp, to_actual(srv->range.min));
	net_buf_simple_add_le16(&rsp, to_actual(srv->range.max));

	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int set_range(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	struct bt_mesh_lightness_range new;

	new.min = from_actual(net_buf_simple_pull_le16(buf));
	new.max = from_actual(net_buf_simple_pull_le16(buf));

	/* The test specification doesn't accept changes to the status, even if
	 * the parameters are wrong. Simply ignore messages with wrong
	 * parameters.
	 */
	if (new.min == 0 || new.max == 0 || new.min > new.max) {
		return -EINVAL;
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
		return 0;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHTNESS_OP_RANGE_STATUS,
				 BT_MESH_LIGHTNESS_MSG_LEN_RANGE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHTNESS_OP_RANGE_STATUS);

	net_buf_simple_add_u8(&rsp, BT_MESH_MODEL_SUCCESS);
	net_buf_simple_add_le16(&rsp, to_actual(srv->range.min));
	net_buf_simple_add_le16(&rsp, to_actual(srv->range.max));

	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_range_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	return set_range(model, ctx, buf, true);
}

static int handle_range_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	return set_range(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_lightness_srv_op[] = {
	{
		BT_MESH_LIGHTNESS_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_GET),
		handle_actual_get,
	},
	{
		BT_MESH_LIGHTNESS_OP_SET,
		BT_MESH_LEN_MIN(BT_MESH_LIGHTNESS_MSG_MINLEN_SET),
		handle_actual_set,
	},
	{
		BT_MESH_LIGHTNESS_OP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LIGHTNESS_MSG_MINLEN_SET),
		handle_actual_set_unack,
	},
	{
		BT_MESH_LIGHTNESS_OP_LINEAR_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_GET),
		handle_linear_get,
	},
	{
		BT_MESH_LIGHTNESS_OP_LINEAR_SET,
		BT_MESH_LEN_MIN(BT_MESH_LIGHTNESS_MSG_MINLEN_SET),
		handle_linear_set,
	},
	{
		BT_MESH_LIGHTNESS_OP_LINEAR_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LIGHTNESS_MSG_MINLEN_SET),
		handle_linear_set_unack,
	},
	{
		BT_MESH_LIGHTNESS_OP_LAST_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_LAST_GET),
		handle_last_get,
	},
	{
		BT_MESH_LIGHTNESS_OP_DEFAULT_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_GET),
		handle_default_get,
	},
	{
		BT_MESH_LIGHTNESS_OP_RANGE_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_RANGE_GET),
		handle_range_get,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_lightness_setup_srv_op[] = {
	{
		BT_MESH_LIGHTNESS_OP_DEFAULT_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET),
		handle_default_set,
	},
	{
		BT_MESH_LIGHTNESS_OP_DEFAULT_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_DEFAULT_SET),
		handle_default_set_unack,
	},
	{
		BT_MESH_LIGHTNESS_OP_RANGE_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET),
		handle_range_set,
	},
	{
		BT_MESH_LIGHTNESS_OP_RANGE_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHTNESS_MSG_LEN_RANGE_SET),
		handle_range_set_unack,
	},
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
	BT_DBG("%i -> %i [%u ms]", rsp->current, rsp->target,
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
		/* According to the Mesh Model Specification section 6.2.3.1,
		 * manual changes to the lightness should disable control.
		 */
		lightness_srv_disable_control(srv);
		lightness_srv_change_lvl(srv, ctx, &set, &status, true);
	} else if (rsp) {
		srv->handlers->light_get(srv, NULL, &status);
	}

	if (rsp) {
		rsp->current = LIGHT_TO_LVL(status.current);
		rsp->target = LIGHT_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
		BT_DBG("%i -> %i [%u ms]", rsp->current, rsp->target,
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
	int32_t target_actual;

	if (delta_set->new_transaction) {
		srv->handlers->light_get(srv, NULL, &status);
		/* Delta lvl is bound to the lightness actual state, so the
		 * calculation must happen in that space:
		 */
		srv->delta_start = to_actual(status.current);
	}

	/* Clamp the value to the lightness range before storing it in an
	 * unsigned 16 bit value, as this would overflow if the target is beyond
	 * its storage limits, causing invalid values.
	 */
	target_actual = CLAMP(srv->delta_start + delta_set->delta,
			      BT_MESH_LIGHTNESS_MIN, BT_MESH_LIGHTNESS_MAX);

	struct bt_mesh_lightness_set set = {
		/* Converting back to configured space: */
		.lvl = from_actual(target_actual),
		.transition = delta_set->transition,
	};

	/* According to the Mesh Model Specification section 6.2.3.1,
	 * manual changes to the lightness should disable control.
	 */
	lightness_srv_disable_control(srv);
	lightness_srv_change_lvl(srv, ctx, &set, &status, true);

	/* Override the "last" value if this is dimmed all the way to off. Then
	 * set the "last" value to the value it was before the dimming was
	 * started.
	 */
	if ((set.lvl == 0) && (from_actual(srv->delta_start) != 0) &&
	    (from_actual(srv->delta_start) != srv->last)) {
		/* Recalulate it back to light state when overriding the "last"
		 * value.
		 */
		srv->last = from_actual(srv->delta_start);
		store_state(srv);
	}

	if (rsp) {
		rsp->current = LIGHT_TO_LVL(status.current);
		rsp->target = LIGHT_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;
		BT_DBG("Delta set rsp: %i (light: %u) -> %i (light: %u) [%u ms]",
			rsp->current, status.current, rsp->target, status.target,
			status.remaining_time);
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
	uint16_t target;

	srv->handlers->light_get(srv, NULL, &status);

	if (move_set->delta > 0) {
		target = srv->range.max;
	} else if (move_set->delta < 0) {
		target = srv->range.min;
	} else {
		target = status.current;
	}

	struct bt_mesh_model_transition transition;
	struct bt_mesh_lightness_set set = {
		.lvl = target,
		.transition = NULL,
	};

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

		BT_DBG("Move: distance: %u delta: %i step: %u ms time: %u ms",
		       (uint32_t)distance, move_set->delta,
		       move_set->transition->time, time_to_edge);

		if (time_to_edge > 0) {
			transition.delay = move_set->transition->delay;
			transition.time = time_to_edge;
			set.transition = &transition;
		}
	}

	/* According to the Mesh Model Specification section 6.2.3.1,
	 * manual changes to the lightness should disable control.
	 */
	lightness_srv_disable_control(srv);
	lightness_srv_change_lvl(srv, ctx, &set, &status, true);

	if (rsp) {
		rsp->current = LIGHT_TO_LVL(status.current);
		rsp->target = LIGHT_TO_LVL(status.target);
		rsp->remaining_time = status.remaining_time;

		BT_DBG("Move set rsp: %i (light: %u) -> %i (light: %u) [%u ms]",
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

	/* According to the Mesh Model Specification section 6.2.3.1,
	 * manual changes to the lightness should disable control.
	 */
	lightness_srv_disable_control(srv);
	lightness_srv_change_lvl(srv, ctx, &set, &status, true);

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

const struct bt_mesh_onoff_srv_handlers _bt_mesh_lightness_srv_onoff_handlers = {
	.set = onoff_set,
	.get = onoff_get,
};

static void lightness_srv_reset(struct bt_mesh_lightness_srv *srv)
{
	srv->range.min = 1;
	srv->range.max = UINT16_MAX;
	srv->default_light = 0;
	srv->last = UINT16_MAX;
	atomic_clear_bit(&srv->flags, LIGHTNESS_SRV_FLAG_IS_ON);
}

static void bt_mesh_lightness_srv_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;

	lightness_srv_reset(srv);
	net_buf_simple_reset(srv->pub.msg);
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->lightness_model, false,
					       NULL, NULL, 0);
	}
}

static ssize_t scene_store(struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	struct bt_mesh_lightness_status status = { 0 };

	srv->handlers->light_get(srv, NULL, &status);
	sys_put_le16(to_actual(status.remaining_time ? status.target : status.current), &data[0]);
	return 2;
}

static void scene_recall(struct bt_mesh_model *model, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	struct bt_mesh_lightness_status dummy_status;
	struct bt_mesh_lightness_set set = {
		.lvl = from_actual(sys_get_le16(data)),
		.transition = transition,
	};

	lightness_srv_change_lvl(srv, NULL, &set, &dummy_status, false);
}

static void scene_recall_complete(struct bt_mesh_model *model)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	struct bt_mesh_lightness_status status;

	srv->handlers->light_get(srv, NULL, &status);

	(void)pub(srv, NULL, &status, ACTUAL);
}

/*  MeshMDL1.0.1, section 5.1.3.1.1:
 *  If a model is extending another model, the extending model shall determine
 *  the Stored with Scene behavior of that model.
 *
 *  Use Setup Model to handle Scene Store/Recall as it is not extended
 *  by other models.
 */
BT_MESH_SCENE_ENTRY_SIG(lightness) = {
	.id.sig = BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV,
	.maxlen = 2,
	.store = scene_store,
	.recall = scene_recall,
	.recall_complete = scene_recall_complete,
};

static int update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	struct bt_mesh_lightness_status status = { 0 };

	srv->handlers->light_get(srv, NULL, &status);
	BT_DBG("Republishing: %u -> %u [%u ms]", status.current, status.target,
	       status.remaining_time);
	lvl_status_encode(model->pub->msg, &status, ACTUAL);
	return 0;
}

static int bt_mesh_lightness_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	int err;

	srv->lightness_model = model;

	lightness_srv_reset(srv);
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

#if CONFIG_BT_SETTINGS
	k_work_init_delayable(&srv->store_timer, store_timeout);
#endif

	err = bt_mesh_model_extend(model, srv->ponoff.ponoff_model);
	if (err) {
		return err;
	}

	return bt_mesh_model_extend(model, srv->lvl.model);
}

#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_lightness_srv_settings_set(struct bt_mesh_model *model,
					      const char *name, size_t len_rd,
					      settings_read_cb read_cb,
					      void *cb_arg)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	struct bt_mesh_lightness_srv_settings_data data;
	ssize_t result;

	if (name) {
		return -ENOENT;
	}

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
#endif

int lightness_on_power_up(struct bt_mesh_lightness_srv *srv)
{
	struct bt_mesh_lightness_status dummy = { 0 };
	struct bt_mesh_model_transition transition = {
		.time = srv->ponoff.dtt.transition_time,
	};
	struct bt_mesh_lightness_set set = { .transition = &transition };

	switch (srv->ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_OFF:
		break;
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

	lightness_srv_change_lvl(srv, NULL, &set, &dummy, true);
	return 0;
}

#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_lightness_srv_start(struct bt_mesh_model *model)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;

	/* When Light Lightness server is extended by Light LC server, Light LC server will execute
	 * power-up sequence of Light Lightness server according to section 6.5.1.2. Otherwise,
	 * Light Lightness will execute power-up sequence behavior.
	 */
	if (atomic_test_bit(&srv->flags, LIGHTNESS_SRV_FLAG_EXTENDED_BY_LIGHT_CTRL)) {
		return 0;
	}

	return lightness_on_power_up(srv);
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

static int bt_mesh_lightness_setup_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_lightness_srv *srv = model->user_data;
	int err;

	srv->lightness_setup_model = model;

	err = bt_mesh_model_extend(model, srv->lightness_model);
	if (err) {
		return err;
	}

	return bt_mesh_model_extend(model, srv->ponoff.ponoff_setup_model);
}

const struct bt_mesh_model_cb _bt_mesh_lightness_setup_srv_cb = {
	.init = bt_mesh_lightness_setup_srv_init,
};

int bt_mesh_lightness_srv_pub(struct bt_mesh_lightness_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_lightness_status *status)
{
	return pub(srv, ctx, status, LIGHT_USER_REPR);
}
