/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdlib.h>
#include <zephyr/sys/byteorder.h>
#include <bluetooth/mesh/gen_plvl_srv.h>
#include "model_utils.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_gen_lvl_srv);

#define LVL_TO_POWER(_lvl) ((_lvl) + 32768)
#define POWER_TO_LVL(_power) ((_power)-32768)

/** Persistent storage handling */
struct bt_mesh_plvl_srv_settings_data {
	struct bt_mesh_plvl_range range;
	uint16_t default_power;
#if !IS_ENABLED(CONFIG_EMDS)
	uint16_t last;
	bool is_on;
#endif
} __packed;

#if CONFIG_BT_SETTINGS
static void bt_mesh_plvl_srv_pending_store(struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;

	struct bt_mesh_plvl_srv_settings_data data = {
		.default_power = srv->default_power,
		.range = srv->range,
#if !IS_ENABLED(CONFIG_EMDS)
		.last = srv->transient.last,
		.is_on = srv->transient.is_on,
#endif
	};

	(void)bt_mesh_model_data_store(srv->plvl_model, false, NULL, &data,
				       sizeof(data));
}
#endif

static void store_state(struct bt_mesh_plvl_srv *srv)
{
#if CONFIG_BT_SETTINGS
	bt_mesh_model_data_store_schedule(srv->plvl_model);
#endif
}

static void lvl_status_encode(struct net_buf_simple *buf,
			      const struct bt_mesh_plvl_status *status)
{
	bt_mesh_model_msg_init(buf, BT_MESH_PLVL_OP_LEVEL_STATUS);

	net_buf_simple_add_le16(buf, status->current);

	if (status->remaining_time == 0) {
		return;
	}

	net_buf_simple_add_le16(buf, status->target);
	net_buf_simple_add_u8(buf,
			      model_transition_encode(status->remaining_time));
}

static int pub(struct bt_mesh_plvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
	       const struct bt_mesh_plvl_status *status)
{
	if (ctx == NULL) {
		/* We'll always make an attempt to publish to the underlying
		 * models as well, since a publish message typically indicates
		 * a status change, which should always be published.
		 * Ignoring the status codes, so that we're able to publish
		 * in the highest model even if the underlying models don't have
		 * publishing configured.
		 */
		struct bt_mesh_lvl_status lvl_status;
		struct bt_mesh_onoff_status onoff_status;

		lvl_status.current = POWER_TO_LVL(status->current);
		lvl_status.target = POWER_TO_LVL(status->target);
		lvl_status.remaining_time = status->remaining_time;
		(void)bt_mesh_lvl_srv_pub(&srv->lvl, NULL, &lvl_status);

		onoff_status.present_on_off = status->current > 0;
		onoff_status.target_on_off = status->target > 0;
		onoff_status.remaining_time = status->remaining_time;
		(void)bt_mesh_onoff_srv_pub(&srv->ponoff.onoff, NULL,
					    &onoff_status);
	}

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PLVL_OP_LEVEL_STATUS,
				 BT_MESH_PLVL_MSG_MAXLEN_LEVEL_STATUS);
	lvl_status_encode(&msg, status);

	return bt_mesh_msg_send(srv->plvl_model, ctx, &msg);
}

static void rsp_plvl_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct bt_mesh_plvl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_LEVEL_STATUS,
				 BT_MESH_PLVL_MSG_MAXLEN_LEVEL_STATUS);
	lvl_status_encode(&rsp, status);

	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);
}

static int handle_lvl_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_plvl_status status = { 0 };

	srv->handlers->power_get(srv, ctx, &status);

	rsp_plvl_status(model, ctx, &status);

	return 0;
}

static int change_lvl(struct bt_mesh_plvl_srv *srv,
		       struct bt_mesh_msg_ctx *ctx,
		       struct bt_mesh_plvl_set *set,
		       struct bt_mesh_plvl_status *status)
{
	bool state_change = (srv->transient.is_on == (set->power_lvl == 0));

	if (set->power_lvl != 0) {
		set->power_lvl =
			CLAMP(set->power_lvl, srv->range.min, srv->range.max);
		state_change |= (srv->transient.last != set->power_lvl);
		srv->transient.last = set->power_lvl;
	}

	srv->transient.is_on = (set->power_lvl > 0);

	if (!IS_ENABLED(CONFIG_EMDS) && state_change) {
		store_state(srv);
	}

	memset(status, 0, sizeof(*status));
	srv->handlers->power_set(srv, ctx, set, status);

	return 0;
}

static int plvl_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	if (buf->len != BT_MESH_PLVL_MSG_MINLEN_LEVEL_SET &&
	    buf->len != BT_MESH_PLVL_MSG_MAXLEN_LEVEL_SET) {
		return -EMSGSIZE;
	}

	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_plvl_status status;
	struct bt_mesh_plvl_set set;
	uint8_t tid;

	set.power_lvl = net_buf_simple_pull_le16(buf);
	tid = net_buf_simple_pull_u8(buf);
	set.transition = model_transition_get(srv->plvl_model, &transition, buf);

	if (!tid_check_and_update(&srv->tid, tid, ctx)) {
		change_lvl(srv, ctx, &set, &status);
		(void)bt_mesh_plvl_srv_pub(srv, NULL, &status);

		if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
			bt_mesh_scene_invalidate(srv->plvl_model);
		}
	} else if (ack) {
		srv->handlers->power_get(srv, NULL, &status);
	}

	if (ack) {
		rsp_plvl_status(model, ctx, &status);
	}

	return 0;
}

static int handle_plvl_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	return plvl_set(model, ctx, buf, true);
}

static int handle_plvl_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	return plvl_set(model, ctx, buf, false);
}

static int handle_last_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_LAST_STATUS,
				 BT_MESH_PLVL_MSG_LEN_LAST_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_LAST_STATUS);

	net_buf_simple_add_le16(&rsp, srv->transient.last);
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_default_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS,
				 BT_MESH_PLVL_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS);

	net_buf_simple_add_le16(&rsp, srv->default_power);
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int set_default(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	uint16_t new = net_buf_simple_pull_le16(buf);

	if (new != srv->default_power) {
		uint16_t old = srv->default_power;

		srv->default_power = new;
		if (srv->handlers->default_update) {
			srv->handlers->default_update(srv, ctx, old, new);
		}

		store_state(srv);
	}

	if (!ack) {
		return 0;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS,
				 BT_MESH_PLVL_MSG_LEN_DEFAULT_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_DEFAULT_STATUS);
	net_buf_simple_add_le16(&rsp, srv->default_power);

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
	struct bt_mesh_plvl_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_RANGE_STATUS,
				 BT_MESH_PLVL_MSG_LEN_RANGE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_RANGE_STATUS);

	net_buf_simple_add_u8(&rsp, BT_MESH_MODEL_SUCCESS);
	net_buf_simple_add_le16(&rsp, srv->range.min);
	net_buf_simple_add_le16(&rsp, srv->range.max);

	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int set_range(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_plvl_range new;

	new.min = net_buf_simple_pull_le16(buf);
	new.max = net_buf_simple_pull_le16(buf);

	/* The test specification doesn't accept changes to the status, even if
	 * the parameters are wrong. Simply ignore messages with wrong
	 * parameters.
	 */
	if (new.min == 0 || new.max == 0 || new.min > new.max) {
		return -EINVAL;
	}

	if (new.min != srv->range.min || new.max != srv->range.max) {
		struct bt_mesh_plvl_range old = srv->range;

		srv->range = new;
		if (srv->handlers->range_update) {
			srv->handlers->range_update(srv, ctx, &old, &new);
		}

		store_state(srv);
	}

	if (!ack) {
		return 0;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PLVL_OP_RANGE_STATUS,
				 BT_MESH_PLVL_MSG_LEN_RANGE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_PLVL_OP_RANGE_STATUS);

	net_buf_simple_add_u8(&rsp, BT_MESH_MODEL_SUCCESS);
	net_buf_simple_add_le16(&rsp, srv->range.min);
	net_buf_simple_add_le16(&rsp, srv->range.max);

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

const struct bt_mesh_model_op _bt_mesh_plvl_srv_op[] = {
	{
		BT_MESH_PLVL_OP_LEVEL_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_LEVEL_GET),
		handle_lvl_get,
	},
	{
		BT_MESH_PLVL_OP_LEVEL_SET,
		BT_MESH_LEN_MIN(BT_MESH_PLVL_MSG_MINLEN_LEVEL_SET),
		handle_plvl_set,
	},
	{
		BT_MESH_PLVL_OP_LEVEL_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_PLVL_MSG_MINLEN_LEVEL_SET),
		handle_plvl_set_unack,
	},
	{
		BT_MESH_PLVL_OP_LAST_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_LAST_GET),
		handle_last_get,
	},
	{
		BT_MESH_PLVL_OP_DEFAULT_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_DEFAULT_GET),
		handle_default_get,
	},
	{
		BT_MESH_PLVL_OP_RANGE_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_RANGE_GET),
		handle_range_get,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_plvl_setup_srv_op[] = {
	{
		BT_MESH_PLVL_OP_DEFAULT_SET,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_DEFAULT_SET),
		handle_default_set,
	},
	{
		BT_MESH_PLVL_OP_DEFAULT_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_DEFAULT_SET),
		handle_default_set_unack,
	},
	{
		BT_MESH_PLVL_OP_RANGE_SET,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_RANGE_SET),
		handle_range_set,
	},
	{
		BT_MESH_PLVL_OP_RANGE_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_PLVL_MSG_LEN_RANGE_SET),
		handle_range_set_unack,
	},
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
	struct bt_mesh_plvl_status status = { 0 };

	if (lvl_set->new_transaction) {
		change_lvl(srv, ctx, &set, &status);
		(void)bt_mesh_plvl_srv_pub(srv, NULL, &status);
	} else if (rsp) {
		srv->handlers->power_get(srv, NULL, &status);
	}

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
	uint16_t start_lvl;

	if (delta_set->new_transaction) {
		srv->handlers->power_get(srv, NULL, &status);
		start_lvl = status.current;
	} else {
		start_lvl = srv->transient.last;
	}

	struct bt_mesh_plvl_set set = {
		/* Clamp the value to the power level range to prevent it be moved back to zero in
		 * binding with Generic Level state.
		 */
		.power_lvl = CLAMP(start_lvl + delta_set->delta, srv->range.min, srv->range.max),
		.transition = delta_set->transition,
	};

	change_lvl(srv, ctx, &set, &status);
	(void)bt_mesh_plvl_srv_pub(srv, NULL, &status);

	/* Override "last" value to be able to make corrective deltas when
	 * new_transaction is false. Note that the "last" value in persistent
	 * storage will still be the target value, allowing us to recover
	 * correctly on power loss.
	 */
	srv->transient.last = start_lvl;

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
	uint16_t target;

	srv->handlers->power_get(srv, NULL, &status);

	if (move_set->delta > 0) {
		target = srv->range.max;
	} else if (move_set->delta < 0) {
		target = srv->range.min;
	} else {
		target = status.current;
	}

	struct bt_mesh_model_transition transition;
	struct bt_mesh_plvl_set set = {
		.power_lvl = target,
		.transition = NULL,
	};

	if (move_set->delta != 0 && move_set->transition) {
		int32_t distance = abs(target - status.current);

		int32_t time_to_edge =
			((uint64_t)distance * (uint64_t)move_set->transition->time) /
			abs(move_set->delta);

		if (time_to_edge > 0) {
			transition.delay = move_set->transition->delay;
			transition.time = time_to_edge;
			set.transition = &transition;
		}
	}

	change_lvl(srv, ctx, &set, &status);
	(void)bt_mesh_plvl_srv_pub(srv, NULL, &status);

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
			(srv->default_power ? srv->default_power : srv->transient.last);
	} else {
		set.power_lvl = 0;
	}

	change_lvl(srv, ctx, &set, &status);
	(void)bt_mesh_plvl_srv_pub(srv, NULL, &status);

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

static ssize_t scene_store(struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_plvl_status status = { 0 };

	srv->handlers->power_get(srv, NULL, &status);
	sys_put_le16(status.remaining_time ? status.target : status.current, &data[0]);

	return 2;
}

static void scene_recall(struct bt_mesh_model *model, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_plvl_status status = { 0 };
	struct bt_mesh_plvl_set set = {
		.power_lvl = sys_get_le16(data),
		.transition = transition,
	};

	change_lvl(srv, NULL, &set, &status);
}

static void scene_recall_complete(struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_plvl_status status = { 0 };

	srv->handlers->power_get(srv, NULL, &status);

	(void)bt_mesh_plvl_srv_pub(srv, NULL, &status);
}

/*  MeshMDL1.0.1, section 5.1.3.1.1:
 *  If a model is extending another model, the extending model shall determine
 *  the Stored with Scene behavior of that model.
 *
 *  Use Setup Model to handle Scene Store/Recall as it is not extended
 *  by other models.
 */
BT_MESH_SCENE_ENTRY_SIG(plvl) = {
	.id.sig = BT_MESH_MODEL_ID_GEN_POWER_LEVEL_SETUP_SRV,
	.maxlen = 2,
	.store = scene_store,
	.recall = scene_recall,
	.recall_complete = scene_recall_complete,
};

static void plvl_srv_reset(struct bt_mesh_plvl_srv *srv)
{
	srv->range.min = 0;
	srv->range.max = UINT16_MAX;
	srv->default_power = 0;
	srv->transient.last = UINT16_MAX;
	srv->transient.is_on = false;
}

static void bt_mesh_plvl_srv_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;

	plvl_srv_reset(srv);
	net_buf_simple_reset(model->pub->msg);
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->plvl_model, false, NULL,
					       NULL, 0);
	}
}

static int update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_plvl_status status = { 0 };

	srv->handlers->power_get(srv, NULL, &status);
	lvl_status_encode(model->pub->msg, &status);
	return 0;
}

static int bt_mesh_plvl_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	int err;

	srv->plvl_model = model;

	plvl_srv_reset(srv);
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

#if IS_ENABLED(CONFIG_BT_SETTINGS) && IS_ENABLED(CONFIG_EMDS)
	srv->emds_entry.entry.id = EMDS_MODEL_ID(model);
	srv->emds_entry.entry.data = (uint8_t *)&srv->transient;
	srv->emds_entry.entry.len = sizeof(srv->transient);
	err = emds_entry_add(&srv->emds_entry);
	if (err) {
		return err;
	}
#endif

	err = bt_mesh_model_extend(model, srv->ponoff.ponoff_model);
	if (err) {
		return err;
	}

	return bt_mesh_model_extend(model, srv->lvl.model);
}

#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_plvl_srv_settings_set(struct bt_mesh_model *model,
					 const char *name, size_t len_rd,
					 settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_plvl_srv_settings_data data;

	if (name) {
		return -ENOENT;
	}

	if (read_cb(cb_arg, &data, sizeof(data)) != sizeof(data)) {
		return -EINVAL;
	}

	srv->default_power = data.default_power;
	srv->range = data.range;
#if !IS_ENABLED(CONFIG_EMDS)
	srv->transient.last = data.last;
	srv->transient.is_on = data.is_on;
#endif

	return 0;
}

static int bt_mesh_plvl_srv_start(struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	struct bt_mesh_plvl_status status = { 0 };
	struct bt_mesh_model_transition transition = {
		.time = srv->ponoff.dtt.transition_time,
	};
	struct bt_mesh_plvl_set set = { .transition = &transition };

	switch (srv->ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_OFF:
		set.power_lvl = 0;
		break;
	case BT_MESH_ON_POWER_UP_ON:
		set.power_lvl =
			(srv->default_power ? srv->default_power : srv->transient.last);
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		set.power_lvl = srv->transient.is_on ? srv->transient.last : 0;
		break;
	default:
		return -EINVAL;
	}

	change_lvl(srv, NULL, &set, &status);
	(void)bt_mesh_plvl_srv_pub(srv, NULL, &status);
	return 0;
}
#endif

const struct bt_mesh_model_cb _bt_mesh_plvl_srv_cb = {
	.init = bt_mesh_plvl_srv_init,
	.reset = bt_mesh_plvl_srv_reset,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = bt_mesh_plvl_srv_settings_set,
	.start = bt_mesh_plvl_srv_start,
	.pending_store = bt_mesh_plvl_srv_pending_store,
#endif
};

static int bt_mesh_plvl_setup_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_plvl_srv *srv = model->user_data;
	int err;

	srv->plvl_setup_model = model;

	err = bt_mesh_model_extend(model, srv->ponoff.ponoff_setup_model);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_MESH_COMP_PAGE_1)
	err = bt_mesh_model_correspond(model, srv->plvl_model);
	if (err) {
		return err;
	}
#endif

	return bt_mesh_model_extend(model, srv->plvl_model);
}


const struct bt_mesh_model_cb _bt_mesh_plvl_setup_srv_cb = {
	.init = bt_mesh_plvl_setup_srv_init,
};

int bt_mesh_plvl_srv_pub(struct bt_mesh_plvl_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_plvl_status *status)
{
	return pub(srv, ctx, status);
}
