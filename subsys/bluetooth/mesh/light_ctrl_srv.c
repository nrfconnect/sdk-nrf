/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <bluetooth/mesh/light_ctrl_srv.h>
#include <bluetooth/mesh/sensor_types.h>
#include <bluetooth/mesh/properties.h>
#include "lightness_internal.h"
#include "light_ctrl_internal.h"
#include "sensor.h"
#include "model_utils.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_light_ctrl
#include "common/log.h"

#define REG_INT CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_INTERVAL

#define FLAGS_CONFIGURATION (BIT(FLAG_OCC_MODE))

enum flags {
	FLAG_ON,
	FLAG_OCC_MODE,
	FLAG_MANUAL,
	FLAG_REGULATOR,
	FLAG_OCC_PENDING,
	FLAG_ON_PENDING,
	FLAG_OFF_PENDING,
	FLAG_TRANSITION,
	FLAG_STORE_CFG,
	FLAG_STORE_STATE,
};

enum stored_flags {
	STORED_FLAG_ON = FLAG_ON,
	STORED_FLAG_OCC_MODE = FLAG_OCC_MODE,
	STORED_FLAG_ENABLED,
};

struct setup_srv_storage_data {
	struct bt_mesh_light_ctrl_srv_cfg cfg;
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	struct bt_mesh_light_ctrl_srv_reg_cfg reg_cfg;
#endif
};

static void restart_timer(struct bt_mesh_light_ctrl_srv *srv, u32_t delay)
{
	k_delayed_work_submit(&srv->timer, K_MSEC(delay));
}

static void reg_start(struct bt_mesh_light_ctrl_srv *srv)
{
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	k_delayed_work_submit(&srv->reg.timer, K_MSEC(REG_INT));
#endif
}

static inline u32_t to_centi_lux(const struct sensor_value *lux)
{
	return lux->val1 * 100L + lux->val2 / 10000L;
}

static inline void from_centi_lux(u32_t centi_lux, struct sensor_value *lux)
{
	lux->val1 = centi_lux / 100L;
	lux->val2 = centi_lux % 10000L;
}

static void store(struct bt_mesh_light_ctrl_srv *srv, enum flags kind)
{
#if CONFIG_BT_SETTINGS
	bool pending = atomic_test_bit(&srv->flags, FLAG_STORE_CFG) ||
		       atomic_test_bit(&srv->flags, FLAG_STORE_STATE);

	atomic_set_bit(&srv->flags, kind);

	if (!pending) {
		k_delayed_work_submit(
			&srv->store_timer,
			K_SECONDS(CONFIG_BT_MESH_LIGHT_CTRL_SRV_STORE_TIMEOUT));
	}
#endif
}

static bool is_enabled(const struct bt_mesh_light_ctrl_srv *srv)
{
	return atomic_test_bit(&srv->lightness->flags,
			       LIGHTNESS_SRV_FLAG_CONTROLLED);
}

static int delayed_change(struct bt_mesh_light_ctrl_srv *srv, bool value,
			  u32_t delay)
{
	if (((value && (atomic_test_bit(&srv->flags, FLAG_ON_PENDING) ||
			atomic_test_bit(&srv->flags, FLAG_OCC_PENDING))) ||
	     (!value && atomic_test_bit(&srv->flags, FLAG_OFF_PENDING))) &&
	    (k_delayed_work_remaining_get(&srv->action_delay) < delay)) {
		/* Trying to do a second delayed change that will finish later
		 * with the same result. Can be safely ignored.
		 */
		return -EBUSY;
	}

	/* Clear all pending transitions - the last one triggered should be the
	 * one that gets executed. Note that the check above prevents this from
	 * delaying transitions.
	 */
	atomic_clear_bit(&srv->flags, FLAG_ON_PENDING);
	atomic_clear_bit(&srv->flags, FLAG_OFF_PENDING);
	atomic_clear_bit(&srv->flags, FLAG_OCC_PENDING);

	k_delayed_work_submit(&srv->action_delay, K_MSEC(delay));

	return 0;
}

static void delayed_occupancy(struct bt_mesh_light_ctrl_srv *srv,
			      const struct sensor_value *time_since_motion)
{
	int err;

	s32_t ms_since_motion = time_since_motion->val1 * MSEC_PER_SEC;

	if (ms_since_motion > srv->cfg.occupancy_delay) {
		return;
	}

	err = delayed_change(srv, true,
			     srv->cfg.occupancy_delay - ms_since_motion);
	if (err) {
		return;
	}

	atomic_set_bit(&srv->flags, FLAG_OCC_PENDING);
}

static void delayed_set(struct bt_mesh_light_ctrl_srv *srv,
			const struct bt_mesh_model_transition *transition,
			bool value)
{
	int err;

	err = delayed_change(srv, value, transition->delay);
	if (err) {
		return;
	}

	atomic_set_bit(&srv->flags, value ? FLAG_ON_PENDING : FLAG_OFF_PENDING);

	srv->fade.duration = transition->time;
}

static void light_set(struct bt_mesh_light_ctrl_srv *srv, u16_t lvl,
		      u32_t transition_time)
{
	struct bt_mesh_model_transition transition = { transition_time };
	struct bt_mesh_lightness_set set = {
		.lvl = lvl,
		.transition = &transition,
	};
	struct bt_mesh_lightness_status dummy;

	lightness_srv_change_lvl(srv->lightness, NULL, &set, &dummy);
}

static u32_t curr_fade_time(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!atomic_test_bit(&srv->flags, FLAG_TRANSITION)) {
		return 0;
	}

	return srv->fade.duration - k_delayed_work_remaining_get(&srv->timer);
}

static u32_t remaining_fade_time(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!atomic_test_bit(&srv->flags, FLAG_TRANSITION)) {
		return 0;
	}

	return k_delayed_work_remaining_get(&srv->timer);
}

static bool state_is_on(const struct bt_mesh_light_ctrl_srv *srv,
			enum bt_mesh_light_ctrl_srv_state state)
{
	/* Only the stable Standby state is considered Off: */
	if (srv->fade.duration > 0) {
		return (state != LIGHT_CTRL_STATE_STANDBY ||
			atomic_test_bit(&srv->flags, FLAG_TRANSITION));
	}

	return srv->state != LIGHT_CTRL_STATE_STANDBY;
}

static void onoff_encode(struct bt_mesh_light_ctrl_srv *srv,
			 struct net_buf_simple *buf,
			 enum bt_mesh_light_ctrl_srv_state prev_state)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS);
	net_buf_simple_add_u8(buf, state_is_on(srv, prev_state));

	u32_t remaining_fade = remaining_fade_time(srv);

	if (remaining_fade) {
		net_buf_simple_add_u8(buf,
				      srv->state != LIGHT_CTRL_STATE_STANDBY);
		net_buf_simple_add_u8(buf,
				      model_transition_encode(remaining_fade));
		return;
	}

	if (atomic_test_bit(&srv->flags, FLAG_ON_PENDING) ||
	    atomic_test_bit(&srv->flags, FLAG_OFF_PENDING)) {
		remaining_fade =
			k_delayed_work_remaining_get(&srv->action_delay) +
			srv->fade.duration;
		net_buf_simple_add_u8(buf, atomic_test_bit(&srv->flags,
							   FLAG_ON_PENDING));
		net_buf_simple_add_u8(buf,
				      model_transition_encode(remaining_fade));
		return;
	}
}

static int onoff_status_send(struct bt_mesh_light_ctrl_srv *srv,
			     struct bt_mesh_msg_ctx *ctx,
			     enum bt_mesh_light_ctrl_srv_state prev_state)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS,
				 3);
	onoff_encode(srv, &buf, prev_state);

	return model_send(srv->model, ctx, &buf);
}

static void onoff_pub(struct bt_mesh_light_ctrl_srv *srv,
		      enum bt_mesh_light_ctrl_srv_state prev_state,
		      bool pub_gen_onoff)
{
	onoff_status_send(srv, NULL, prev_state);

	if (!pub_gen_onoff) {
		return;
	}

	u32_t remaining_fade;

	if (atomic_test_bit(&srv->flags, FLAG_ON_PENDING) ||
	    atomic_test_bit(&srv->flags, FLAG_OFF_PENDING)) {
		remaining_fade =
			k_delayed_work_remaining_get(&srv->action_delay) +
			srv->fade.duration;
	} else {
		remaining_fade = remaining_fade_time(srv);
	}

	const struct bt_mesh_onoff_status status = {
		state_is_on(srv, prev_state),
		state_is_on(srv, srv->state),
		remaining_fade,
	};

	bt_mesh_onoff_srv_pub(&srv->onoff, NULL, &status);
}

static u16_t light_get(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!is_enabled(srv)) {
		return 0;
	}

	if (!atomic_test_bit(&srv->flags, FLAG_TRANSITION) ||
	    !srv->fade.duration) {
		return srv->cfg.light[srv->state];
	}

	u32_t curr = curr_fade_time(srv);
	u16_t start = srv->fade.initial_light;
	u16_t end = srv->cfg.light[srv->state];

	/* Linear interpolation: */
	return start + ((end - start) * curr) / srv->fade.duration;
}

static void lux_get(struct bt_mesh_light_ctrl_srv *srv,
		    struct sensor_value *lux)
{
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (!is_enabled(srv)) {
		memset(lux, 0, sizeof(*lux));
		return;
	}

	if (!atomic_test_bit(&srv->flags, FLAG_TRANSITION) ||
	    !srv->fade.duration) {
		*lux = srv->reg.cfg.lux[srv->state];
		return;
	}

	u32_t delta = curr_fade_time(srv);
	u32_t init = to_centi_lux(&srv->fade.initial_lux);
	u32_t cfg = to_centi_lux(&srv->reg.cfg.lux[srv->state]);
	u32_t centi_lux = init + ((cfg - init) * delta) / srv->fade.duration;

	from_centi_lux(centi_lux, lux);
#else
	memset(lux, 0, sizeof(*lux));
#endif
}

static void transition_start(struct bt_mesh_light_ctrl_srv *srv,
			     enum bt_mesh_light_ctrl_srv_state state,
			     u32_t fade_time)
{
	srv->state = state;
	srv->fade.initial_light = light_get(srv);
	srv->fade.duration = fade_time;
	lux_get(srv, &srv->fade.initial_lux);

	atomic_set_bit(&srv->flags, FLAG_TRANSITION);
	light_set(srv, srv->cfg.light[state], fade_time);
	restart_timer(srv, fade_time);
}

static int turn_on(struct bt_mesh_light_ctrl_srv *srv,
		   const struct bt_mesh_model_transition *transition,
		   bool pub_gen_onoff)
{
	if (!is_enabled(srv)) {
		return -EBUSY;
	}

	BT_DBG("");

	u32_t fade_time;

	if (transition) {
		fade_time = transition->time;
	} else {
		fade_time = srv->cfg.fade_on;
	}

	if (srv->state == LIGHT_CTRL_STATE_ON) {
		restart_timer(srv, remaining_fade_time(srv) + srv->cfg.on);
	} else {
		enum bt_mesh_light_ctrl_srv_state prev_state = srv->state;

		if (prev_state == LIGHT_CTRL_STATE_STANDBY) {
			atomic_set_bit(&srv->flags, FLAG_ON);
			atomic_clear_bit(&srv->flags, FLAG_MANUAL);
			store(srv, FLAG_STORE_STATE);
		}

		transition_start(srv, LIGHT_CTRL_STATE_ON, fade_time);
		onoff_pub(srv, prev_state, pub_gen_onoff);
	}

	return 0;
}

static void turn_off_auto(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!is_enabled(srv) || srv->state != LIGHT_CTRL_STATE_PROLONG) {
		return;
	}

	BT_DBG("");

	transition_start(srv, LIGHT_CTRL_STATE_STANDBY,
			 srv->cfg.fade_standby_auto);

	atomic_clear_bit(&srv->flags, FLAG_ON);
	store(srv, FLAG_STORE_STATE);
	onoff_pub(srv, LIGHT_CTRL_STATE_PROLONG, true);
}

static int turn_off(struct bt_mesh_light_ctrl_srv *srv,
		    const struct bt_mesh_model_transition *transition,
		    bool pub_gen_onoff)
{
	if (!is_enabled(srv)) {
		return -EBUSY;
	}

	BT_DBG("");

	atomic_set_bit(&srv->flags, FLAG_MANUAL);

	u32_t fade_time =
		transition ? transition->time : srv->cfg.fade_standby_manual;
	enum bt_mesh_light_ctrl_srv_state prev_state = srv->state;

	if (prev_state != LIGHT_CTRL_STATE_STANDBY) {
		transition_start(srv, LIGHT_CTRL_STATE_STANDBY, fade_time);
		atomic_clear_bit(&srv->flags, FLAG_ON);
		store(srv, FLAG_STORE_STATE);
		onoff_pub(srv, prev_state, pub_gen_onoff);
	} else if (fade_time < remaining_fade_time(srv)) {
		/* Replacing current transition with a manual transition if it's
		 * faster. This ensures that the manual override looks
		 * responsive:
		 */
		transition_start(srv, LIGHT_CTRL_STATE_STANDBY, fade_time);
	}

	return 0;
}

static void prolong(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!is_enabled(srv) || srv->state != LIGHT_CTRL_STATE_ON) {
		return;
	}

	BT_DBG("");

	transition_start(srv, LIGHT_CTRL_STATE_PROLONG, srv->cfg.fade_prolong);
}

static void ctrl_enable(struct bt_mesh_light_ctrl_srv *srv)
{
	atomic_set_bit(&srv->lightness->flags, LIGHTNESS_SRV_FLAG_CONTROLLED);

	transition_start(srv, LIGHT_CTRL_STATE_STANDBY, 0);
	reg_start(srv);
}

static void ctrl_disable(struct bt_mesh_light_ctrl_srv *srv)
{
	/* Do not change the light level, disabling the control server just
	 * disengages it from the Lightness Server.
	 */

	/* Clear transient state: */
	atomic_and(&srv->flags, FLAGS_CONFIGURATION);
	atomic_clear_bit(&srv->lightness->flags, LIGHTNESS_SRV_FLAG_CONTROLLED);
	srv->state = LIGHT_CTRL_STATE_STANDBY;

	k_delayed_work_cancel(&srv->action_delay);
	k_delayed_work_cancel(&srv->timer);
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	k_delayed_work_cancel(&srv->reg.timer);
#endif
}

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
static void reg_step(struct k_work *work)
{
	struct bt_mesh_light_ctrl_srv *srv = CONTAINER_OF(
		work, struct bt_mesh_light_ctrl_srv, reg.timer.work);

	if (!is_enabled(srv)) {
		/* The server might be disabled asynchronously. */
		return;
	}

	struct sensor_value raw_lux;

	lux_get(srv, &raw_lux);

	u32_t lux = raw_lux.val1;
	u32_t ambient = srv->ambient_lux.val1;

	s32_t error = lux - ambient;
	/* Accuracy should be in percent and both up and down: */
	u32_t accuracy = (srv->reg.cfg.accuracy * lux) / (2 * 100);
	s16_t input;
	u16_t output;

	if (error > accuracy) {
		input = error - accuracy;
	} else if (error < -accuracy) {
		input = error + accuracy;
	} else {
		return;
	}

	if (input >= 0) {
		s32_t p = input * srv->reg.cfg.kpu;
		s32_t i = (input * REG_INT * srv->reg.cfg.kiu) / MSEC_PER_SEC;

		srv->reg.i = MIN(UINT16_MAX, srv->reg.i + i);
		output = MIN(UINT16_MAX, srv->reg.i + p);
	} else {
		s32_t p = input * srv->reg.cfg.kpd;
		s32_t i = (input * REG_INT * srv->reg.cfg.kid) / MSEC_PER_SEC;

		srv->reg.i = MAX(0, srv->reg.i + i);
		output = MAX(0, srv->reg.i + p);
	}

	/* The regulator output is always in linear format. We'll convert to
	 * the configured representation again before calling the Lightness
	 * server.
	 */
	u16_t lvl = light_get(srv);

	/* Output value is max out of regulator and configured level. */
	if (output > lvl) {
		atomic_set_bit(&srv->flags, FLAG_REGULATOR);
		light_set(srv, light_to_repr(output, LINEAR), REG_INT);
	} else if (atomic_test_and_clear_bit(&srv->flags, FLAG_REGULATOR)) {
		light_set(srv, light_to_repr(lvl, LINEAR), REG_INT);
	}

	k_delayed_work_submit(&srv->reg.timer, K_MSEC(REG_INT));
}
#endif

/*******************************************************************************
 * Timeouts
 ******************************************************************************/

static void timeout(struct k_work *work)
{
	struct bt_mesh_light_ctrl_srv *srv =
		CONTAINER_OF(work, struct bt_mesh_light_ctrl_srv, timer.work);

	/* According to test spec, we should publish the OnOff state at the end
	 * of the transition:
	 */
	if (atomic_test_and_clear_bit(&srv->flags, FLAG_TRANSITION)) {
		BT_DBG("Transition complete");

		/* If the fade wasn't instant, we've already published the
		 * steady state in the state change function.
		 */
		if (srv->fade.duration > 0) {
			onoff_pub(srv, srv->state, true);
		}

		if (srv->state == LIGHT_CTRL_STATE_ON) {
			restart_timer(srv, srv->cfg.on);
		} else if (srv->state == LIGHT_CTRL_STATE_PROLONG) {
			restart_timer(srv, srv->cfg.prolong);
		} else if (atomic_test_bit(&srv->flags, FLAG_MANUAL)) {
			restart_timer(
				srv, CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_MANUAL -
					     srv->cfg.fade_standby_manual);
		}
	} else if (srv->state == LIGHT_CTRL_STATE_ON) {
		prolong(srv);
	} else if (srv->state == LIGHT_CTRL_STATE_PROLONG) {
		turn_off_auto(srv);
	} else {
		/* Ends the sensor cooldown period: */
		atomic_clear_bit(&srv->flags, FLAG_MANUAL);
	}
}

static void delayed_action_timeout(struct k_work *work)
{
	struct bt_mesh_light_ctrl_srv *srv = CONTAINER_OF(
		work, struct bt_mesh_light_ctrl_srv, action_delay.work);
	struct bt_mesh_model_transition transition = {
		.time = srv->fade.duration
	};

	BT_DBG("");

	if (atomic_test_and_clear_bit(&srv->flags, FLAG_ON_PENDING)) {
		turn_on(srv, &transition, true);
	} else if (atomic_test_and_clear_bit(&srv->flags, FLAG_OFF_PENDING)) {
		turn_off(srv, &transition, true);
	} else if (atomic_test_and_clear_bit(&srv->flags, FLAG_OCC_PENDING) &&
		   (srv->state == LIGHT_CTRL_STATE_ON ||
		    atomic_test_bit(&srv->flags, FLAG_OCC_MODE))) {
		turn_on(srv, NULL, true);
	}
}

#if CONFIG_BT_SETTINGS
static void store_timeout(struct k_work *work)
{
	struct bt_mesh_light_ctrl_srv *srv = CONTAINER_OF(
		work, struct bt_mesh_light_ctrl_srv, store_timer.work);
	int err;

	if (atomic_test_and_clear_bit(&srv->flags, FLAG_STORE_CFG)) {
		struct setup_srv_storage_data data = {
			.cfg = srv->cfg,
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
			.reg_cfg = srv->reg.cfg,
#endif
		};

		err = bt_mesh_model_data_store(srv->setup_srv, false, &data,
					       sizeof(data));
		if (err) {
			BT_ERR("Failed storing config: %d", err);
		}
	}

	if (atomic_test_and_clear_bit(&srv->flags, FLAG_STORE_STATE)) {
		atomic_t data = 0;

		atomic_set_bit_to(&data, STORED_FLAG_ENABLED, is_enabled(srv));
		atomic_set_bit_to(&data, STORED_FLAG_ON,
				  atomic_test_bit(&srv->flags, FLAG_ON));
		atomic_set_bit_to(&data, STORED_FLAG_OCC_MODE,
				  atomic_test_bit(&srv->flags, FLAG_OCC_MODE));

		err = bt_mesh_model_data_store(srv->model, false, &data,
					 sizeof(data));
		if (err) {
			BT_ERR("Failed storing state: %d", err);
		}
	}

	BT_DBG("");
}
#endif
/*******************************************************************************
 * Handlers
 ******************************************************************************/

static void mode_rsp(struct bt_mesh_light_ctrl_srv *srv,
		     struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHT_CTRL_OP_MODE_STATUS, 1);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHT_CTRL_OP_MODE_STATUS);
	net_buf_simple_add_u8(&rsp, is_enabled(srv));

	model_send(srv->model, ctx, &rsp);
}

static void handle_mode_get(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	if (buf->len != 0) {
		return;
	}

	mode_rsp(srv, ctx);
}

static int mode_set(struct bt_mesh_light_ctrl_srv *srv,
		    struct net_buf_simple *buf)
{
	if (buf->len != 1) {
		return -EINVAL;
	}

	u8_t mode = net_buf_simple_pull_u8(buf);

	if (mode > 1) {
		return -EINVAL;
	}

	BT_DBG("%s", mode ? "enable" : "disable");

	if (mode) {
		bt_mesh_light_ctrl_srv_enable(srv);
	} else {
		bt_mesh_light_ctrl_srv_disable(srv);
	}

	return 0;
}

static void handle_mode_set(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;
	int err;

	err = mode_set(srv, buf);
	if (err) {
		return;
	}

	mode_rsp(srv, ctx);
	mode_rsp(srv, NULL); /* publish */
}

static void handle_mode_set_unack(struct bt_mesh_model *mod,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;
	int err;

	err = mode_set(srv, buf);
	if (err) {
		return;
	}

	mode_rsp(srv, NULL); /* publish */
}

static void om_rsp(struct bt_mesh_light_ctrl_srv *srv,
		   struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHT_CTRL_OP_OM_STATUS, 1);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHT_CTRL_OP_OM_STATUS);
	net_buf_simple_add_u8(&rsp,
			      atomic_test_bit(&srv->flags, FLAG_OCC_MODE));

	model_send(srv->model, ctx, &rsp);
}

static void handle_om_get(struct bt_mesh_model *mod,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	if (buf->len != 0) {
		return;
	}

	om_rsp(srv, ctx);
}

static int om_set(struct bt_mesh_light_ctrl_srv *srv,
		  struct net_buf_simple *buf)
{
	if (buf->len != 1) {
		return -EINVAL;
	}

	u8_t mode = net_buf_simple_pull_u8(buf);

	if (mode > 1) {
		return -EINVAL;
	}

	BT_DBG("%s", mode ? "on" : "off");

	atomic_set_bit_to(&srv->flags, FLAG_OCC_MODE, mode);
	store(srv, FLAG_STORE_STATE);

	return 0;
}

static void handle_om_set(struct bt_mesh_model *mod,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;
	int err;

	err = om_set(srv, buf);
	if (err) {
		return;
	}

	om_rsp(srv, ctx);
	om_rsp(srv, NULL); /* publish */
}

static void handle_om_set_unack(struct bt_mesh_model *mod,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;
	int err;

	err = om_set(srv, buf);
	if (err) {
		return;
	}

	om_rsp(srv, NULL); /* publish */
}

static void handle_light_onoff_get(struct bt_mesh_model *mod,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	if (buf->len != 0) {
		return;
	}

	BT_DBG("");

	onoff_status_send(srv, ctx, srv->state);
}

static void light_onoff_set(struct bt_mesh_light_ctrl_srv *srv,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf, bool ack)
{
	u8_t onoff = net_buf_simple_pull_u8(buf);

	if (onoff > 1) {
		return;
	}

	u8_t tid = net_buf_simple_pull_u8(buf);

	bool has_trans = (buf->len == 2);
	struct bt_mesh_model_transition transition;

	if (has_trans) {
		model_transition_buf_pull(buf, &transition);
	} else if (buf->len != 0) {
		return;
	}

	enum bt_mesh_light_ctrl_srv_state prev_state = srv->state;

	if (!tid_check_and_update(&srv->tid, tid, ctx)) {
		BT_DBG("%s (%u + %u)", onoff ? "on" : "off",
		       has_trans ? transition.time : 0,
		       has_trans ? transition.delay : 0);

		if (has_trans && transition.delay > 0) {
			delayed_set(srv, &transition, onoff);
		} else if (onoff) {
			turn_on(srv, has_trans ? &transition : NULL, true);
		} else {
			turn_off(srv, has_trans ? &transition : NULL, true);
		}
	}

	if (ack) {
		onoff_status_send(srv, ctx, prev_state);
	}
}

static void handle_light_onoff_set(struct bt_mesh_model *mod,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	light_onoff_set(srv, ctx, buf, true);
}

static void handle_light_onoff_set_unack(struct bt_mesh_model *mod,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	light_onoff_set(srv, ctx, buf, false);
}

static void handle_sensor_status(struct bt_mesh_model *mod,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;
	int err;

	while (buf->len >= 3) {
		u8_t len;
		u16_t id;
		struct sensor_value value;

		sensor_status_id_decode(buf, &len, &id);

		const struct bt_mesh_sensor_type *type;

		switch (id) {
		case BT_MESH_PROP_ID_MOTION_SENSED:
			type = &bt_mesh_sensor_motion_sensed;
			break;
		case BT_MESH_PROP_ID_PEOPLE_COUNT:
			type = &bt_mesh_sensor_people_count;
			break;
		case BT_MESH_PROP_ID_PRESENCE_DETECTED:
			type = &bt_mesh_sensor_presence_detected;
			break;
		case BT_MESH_PROP_ID_TIME_SINCE_MOTION_SENSED:
			type = &bt_mesh_sensor_time_since_motion_sensed;
			break;
		case BT_MESH_PROP_ID_PRESENT_AMB_LIGHT_LEVEL:
			type = &bt_mesh_sensor_present_amb_light_level;
			break;
		default:
			type = NULL;
			break;
		}

		if (!type) {
			net_buf_simple_pull(buf, len);
			continue;
		}

		err = sensor_value_decode(buf, type, &value);
		if (err) {
			return;
		}

		BT_DBG("Sensor 0x%04x: %s", id, bt_mesh_sensor_ch_str(&value));

		if (id == BT_MESH_PROP_ID_PRESENT_AMB_LIGHT_LEVEL) {
			srv->ambient_lux = value;
			continue;
		}

		/* Occupancy sensor */

		/* OCC_MODE must be enabled for the occupancy sensors to be
		 * able to turn on the light:
		 */
		if (srv->state == LIGHT_CTRL_STATE_STANDBY &&
		    !atomic_test_bit(&srv->flags, FLAG_OCC_MODE)) {
			continue;
		}

		if (id == BT_MESH_PROP_ID_TIME_SINCE_MOTION_SENSED) {
			delayed_occupancy(srv, &value);
		} else if (value.val1 > 0) {
			turn_on(srv, NULL, true);
		}
	}
}

const struct bt_mesh_model_op _bt_mesh_light_ctrl_srv_op[] = {
	{ BT_MESH_LIGHT_CTRL_OP_MODE_GET, 0, handle_mode_get },
	{ BT_MESH_LIGHT_CTRL_OP_MODE_SET, 1, handle_mode_set },
	{ BT_MESH_LIGHT_CTRL_OP_MODE_SET_UNACK, 1, handle_mode_set_unack },
	{ BT_MESH_LIGHT_CTRL_OP_OM_GET, 0, handle_om_get },
	{ BT_MESH_LIGHT_CTRL_OP_OM_SET, 1, handle_om_set },
	{ BT_MESH_LIGHT_CTRL_OP_OM_SET_UNACK, 1, handle_om_set_unack },
	{ BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_GET, 0, handle_light_onoff_get },
	{ BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET, 2, handle_light_onoff_set },
	{ BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET_UNACK, 2,
	  handle_light_onoff_set_unack },
	{ BT_MESH_SENSOR_OP_STATUS, 3, handle_sensor_status },
	BT_MESH_MODEL_OP_END,
};

static void to_prop_time(u32_t time, struct sensor_value *prop)
{
	prop->val1 = time / MSEC_PER_SEC;
	prop->val2 = (time % MSEC_PER_SEC) * 1000;
}

static u32_t from_prop_time(const struct sensor_value *prop)
{
	return prop->val1 * MSEC_PER_SEC + prop->val2 / 1000;
}

static int prop_get(struct net_buf_simple *buf,
		    const struct bt_mesh_light_ctrl_srv *srv, u16_t id)
{
	struct sensor_value val = { 0 };

	switch (id) {
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	case BT_MESH_LIGHT_CTRL_PROP_REG_KID:
		val.val1 = srv->reg.cfg.kid;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_REG_KIU:
		val.val1 = srv->reg.cfg.kiu;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_REG_KPD:
		val.val1 = srv->reg.cfg.kpd;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_REG_KPU:
		val.val1 = srv->reg.cfg.kpu;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY:
		val.val1 = srv->reg.cfg.accuracy;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON:
		val = srv->reg.cfg.lux[LIGHT_CTRL_STATE_ON];
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG:
		val = srv->reg.cfg.lux[LIGHT_CTRL_STATE_PROLONG];
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY:
		val = srv->reg.cfg.lux[LIGHT_CTRL_STATE_STANDBY];
		break;
#else
	case BT_MESH_LIGHT_CTRL_PROP_REG_KID:
	case BT_MESH_LIGHT_CTRL_PROP_REG_KIU:
	case BT_MESH_LIGHT_CTRL_PROP_REG_KPD:
	case BT_MESH_LIGHT_CTRL_PROP_REG_KPU:
	case BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY:
		break; /* Prevent returning -ENOENT */
#endif
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_ON:
		val.val1 = srv->cfg.light[LIGHT_CTRL_STATE_ON];
		break;
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_PROLONG:
		val.val1 = srv->cfg.light[LIGHT_CTRL_STATE_PROLONG];
		break;
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_STANDBY:
		val.val1 = srv->cfg.light[LIGHT_CTRL_STATE_STANDBY];
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_PROLONG:
		to_prop_time(srv->cfg.fade_prolong, &val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_ON:
		to_prop_time(srv->cfg.fade_on, &val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_STANDBY_AUTO:
		to_prop_time(srv->cfg.fade_standby_auto, &val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_STANDBY_MANUAL:
		to_prop_time(srv->cfg.fade_standby_manual, &val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_OCCUPANCY_DELAY:
		to_prop_time(srv->cfg.occupancy_delay, &val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_PROLONG:
		to_prop_time(srv->cfg.prolong, &val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_ON:
		to_prop_time(srv->cfg.on, &val);
		break;
	default:
		return -ENOENT;
	}

	return prop_encode(buf, id, &val);
}

static int prop_set(struct net_buf_simple *buf,
		    struct bt_mesh_light_ctrl_srv *srv, u16_t id)
{
	struct sensor_value val;
	int err;

	err = prop_decode(buf, id, &val);
	if (err) {
		return err;
	}

	BT_DBG("0x%04x: %s", id, bt_mesh_sensor_ch_str(&val));

	switch (id) {
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	case BT_MESH_LIGHT_CTRL_PROP_REG_KID:
		srv->reg.cfg.kid = val.val1;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_REG_KIU:
		srv->reg.cfg.kiu = val.val1;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_REG_KPD:
		srv->reg.cfg.kpd = val.val1;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_REG_KPU:
		srv->reg.cfg.kpu = val.val1;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY:
		srv->reg.cfg.accuracy = val.val1;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON:
		srv->reg.cfg.lux[LIGHT_CTRL_STATE_ON] = val;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG:
		srv->reg.cfg.lux[LIGHT_CTRL_STATE_PROLONG] = val;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY:
		srv->reg.cfg.lux[LIGHT_CTRL_STATE_STANDBY] = val;
		break;
#else
	case BT_MESH_LIGHT_CTRL_PROP_REG_KID:
	case BT_MESH_LIGHT_CTRL_PROP_REG_KIU:
	case BT_MESH_LIGHT_CTRL_PROP_REG_KPD:
	case BT_MESH_LIGHT_CTRL_PROP_REG_KPU:
	case BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY:
		break; /* Prevent returning -ENOENT */
#endif
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_ON:
		srv->cfg.light[LIGHT_CTRL_STATE_ON] = val.val1;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_PROLONG:
		srv->cfg.light[LIGHT_CTRL_STATE_PROLONG] = val.val1;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_STANDBY:
		srv->cfg.light[LIGHT_CTRL_STATE_STANDBY] = val.val1;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_PROLONG:
		srv->cfg.fade_prolong = from_prop_time(&val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_ON:
		srv->cfg.fade_on = from_prop_time(&val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_STANDBY_AUTO:
		srv->cfg.fade_standby_auto = from_prop_time(&val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_FADE_STANDBY_MANUAL:
		srv->cfg.fade_standby_manual = from_prop_time(&val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_OCCUPANCY_DELAY:
		srv->cfg.occupancy_delay = from_prop_time(&val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_PROLONG:
		srv->cfg.prolong = from_prop_time(&val);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_TIME_ON:
		srv->cfg.on = from_prop_time(&val);
		break;
	default:
		return -ENOENT;
	}

	return 0;
}

static void prop_tx(struct bt_mesh_light_ctrl_srv *srv,
		     struct bt_mesh_msg_ctx *ctx, u16_t id)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(
		buf, BT_MESH_LIGHT_CTRL_OP_PROP_STATUS,
		2 + CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_PROP_STATUS);
	net_buf_simple_add_le16(&buf, id);

	err = prop_get(&buf, srv, id);
	if (err) {
		return;
	}

	model_send(srv->setup_srv, ctx, &buf);
}

static void handle_prop_get(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	if (buf->len != 2) {
		return;
	}

	u16_t id = net_buf_simple_pull_le16(buf);

	prop_tx(srv, ctx, id);
}

static void handle_prop_set(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;
	int err;

	u16_t id = net_buf_simple_pull_le16(buf);

	err = prop_set(buf, srv, id);

	if (!err) {
		prop_tx(srv, ctx, id);
		prop_tx(srv, NULL, id);
	}
}

static void handle_prop_set_unack(struct bt_mesh_model *mod,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	u16_t id = net_buf_simple_pull_le16(buf);

	prop_set(buf, srv, id);
	prop_tx(srv, NULL, id);
}

const struct bt_mesh_model_op _bt_mesh_light_ctrl_setup_srv_op[] = {
	{ BT_MESH_LIGHT_CTRL_OP_PROP_GET, 2, handle_prop_get },
	{ BT_MESH_LIGHT_CTRL_OP_PROP_SET, 3, handle_prop_set },
	{ BT_MESH_LIGHT_CTRL_OP_PROP_SET_UNACK, 3, handle_prop_set_unack },
	BT_MESH_MODEL_OP_END,
};

/*******************************************************************************
 * Callbacks
 ******************************************************************************/

static void onoff_set(struct bt_mesh_onoff_srv *onoff,
		      struct bt_mesh_msg_ctx *ctx,
		      const struct bt_mesh_onoff_set *set,
		      struct bt_mesh_onoff_status *rsp)
{
	struct bt_mesh_light_ctrl_srv *srv =
		CONTAINER_OF(onoff, struct bt_mesh_light_ctrl_srv, onoff);

	const struct bt_mesh_model_transition *transition =
		set->transition->time ? set->transition : NULL;
	enum bt_mesh_light_ctrl_srv_state prev_state = srv->state;

	if (set->transition->delay) {
		delayed_set(srv, set->transition, set->on_off);
	} else if (set->on_off) {
		turn_on(srv, transition, false);
	} else {
		turn_off(srv, transition, false);
	}

	if (!rsp) {
		return;
	}

	rsp->present_on_off = state_is_on(srv, prev_state);
	rsp->target_on_off = set->on_off;
	rsp->remaining_time = remaining_fade_time(srv);
}

static void onoff_get(struct bt_mesh_onoff_srv *onoff,
		      struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_onoff_status *rsp)
{
	struct bt_mesh_light_ctrl_srv *srv =
		CONTAINER_OF(onoff, struct bt_mesh_light_ctrl_srv, onoff);

	rsp->present_on_off = bt_mesh_light_ctrl_srv_is_on(srv);
	rsp->target_on_off = (srv->state != LIGHT_CTRL_STATE_STANDBY);
	rsp->remaining_time = remaining_fade_time(srv);
}

const struct bt_mesh_onoff_srv_handlers _bt_mesh_light_ctrl_srv_onoff = {
	.set = onoff_set,
	.get = onoff_get,
};

static int light_ctrl_srv_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	srv->model = mod;

	if (IS_ENABLED(CONFIG_BT_MESH_LIGHT_CTRL_SRV_OCCUPANCY_MODE)) {
		atomic_set_bit(&srv->flags, FLAG_OCC_MODE);
	}

	/* When the Lightness Server boots up in restore mode, it'll start
	 * changing its state in the start cb. The LC Server should decide the
	 * startup activity of the Lightness Server itself.
	 *
	 * Block the Lightness Server startup behavior to prevent it from moving
	 * before the LC Server gets a chance to take control.
	 */
	atomic_set_bit(&srv->lightness->flags, LIGHTNESS_SRV_FLAG_NO_START);

	k_delayed_work_init(&srv->timer, timeout);
	k_delayed_work_init(&srv->action_delay, delayed_action_timeout);

#if CONFIG_BT_SETTINGS
	k_delayed_work_init(&srv->store_timer, store_timeout);
#endif

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	k_delayed_work_init(&srv->reg.timer, reg_step);
#endif

	net_buf_simple_init(srv->pub.msg, 0);

	return 0;
}

static int light_ctrl_srv_settings_set(struct bt_mesh_model *mod, size_t len_rd,
				       settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;
	atomic_t data;
	ssize_t result;

	result = read_cb(cb_arg, &data, sizeof(data));
	if (result <= 0) {
		return result;
	}

	if (result < sizeof(data)) {
		return -EINVAL;
	}

	if (atomic_test_bit(&data, STORED_FLAG_ON)) {
		atomic_set_bit(&srv->flags, FLAG_ON);
	}

	if (atomic_test_bit(&data, STORED_FLAG_OCC_MODE)) {
		atomic_set_bit(&srv->flags, FLAG_OCC_MODE);
	}

	if (atomic_test_bit(&data, STORED_FLAG_ENABLED)) {
		atomic_set_bit(&srv->lightness->flags,
			       LIGHTNESS_SRV_FLAG_CONTROLLED);
	}

	return 0;
}

static int light_ctrl_srv_start(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	if (srv->lightness->lightness_model->elem_idx == mod->elem_idx) {
		BT_ERR("Lightness: Invalid element index");
		return -EINVAL;
	}

	switch (srv->lightness->ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_OFF:
		ctrl_disable(srv);
		/* PTS Corner case: If the device restarts while in the On state
		 * (with OnPowerUp != RESTORE), we'll end up here with lights
		 * off. If OnPowerUp is then changed to RESTORE, and the device
		 * restarts, we'll restore to On even though we were off in the
		 * previous power cycle, unless we store the Off state here.
		 */
		store(srv, FLAG_STORE_STATE);
		break;
	case BT_MESH_ON_POWER_UP_ON:
		ctrl_disable(srv);
		store(srv, FLAG_STORE_STATE);
		light_set(srv,
			  (srv->lightness->default_light ?
				   srv->lightness->default_light :
				   srv->lightness->last),
			  srv->lightness->ponoff.dtt.transition_time);
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		if (is_enabled(srv)) {
			reg_start(srv);
			if (atomic_test_bit(&srv->flags, FLAG_ON)) {
				turn_on(srv, NULL, true);
			}
		} else if (atomic_test_bit(&srv->lightness->flags,
					   LIGHTNESS_SRV_FLAG_IS_ON)) {
			light_set(srv, srv->lightness->last,
				  srv->lightness->ponoff.dtt.transition_time);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_ctrl_srv_cb = {
	.init = light_ctrl_srv_init,
	.start = light_ctrl_srv_start,
	.settings_set = light_ctrl_srv_settings_set,
};

static int lc_setup_srv_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	srv->setup_srv = mod;
	net_buf_simple_init(srv->setup_pub.msg, 0);
	return 0;
}

static int lc_setup_srv_settings_set(struct bt_mesh_model *mod, size_t len_rd,
				     settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;
	struct setup_srv_storage_data data;
	ssize_t result;

	result = read_cb(cb_arg, &data, sizeof(data));
	if (result <= 0) {
		return result;
	}

	if (result < sizeof(data)) {
		return -EINVAL;
	}

	srv->cfg = data.cfg;

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	srv->reg.cfg = data.reg_cfg;
#endif

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_ctrl_setup_srv_cb = {
	.init = lc_setup_srv_init,
	.settings_set = lc_setup_srv_settings_set,
};

int _bt_mesh_light_ctrl_srv_update(struct bt_mesh_model *mod)
{
	BT_DBG("");

	struct bt_mesh_light_ctrl_srv *srv = mod->user_data;

	onoff_encode(srv, srv->pub.msg, srv->state);

	return 0;
}

/*******************************************************************************
 * Public API
 ******************************************************************************/

int bt_mesh_light_ctrl_srv_on(struct bt_mesh_light_ctrl_srv *srv)
{
	return turn_on(srv, NULL, true);
}

int bt_mesh_light_ctrl_srv_off(struct bt_mesh_light_ctrl_srv *srv)
{
	return turn_off(srv, NULL, true);
}

int bt_mesh_light_ctrl_srv_enable(struct bt_mesh_light_ctrl_srv *srv)
{
	if (is_enabled(srv)) {
		return -EALREADY;
	}

	ctrl_enable(srv);
	store(srv, FLAG_STORE_STATE);

	return 0;
}

int bt_mesh_light_ctrl_srv_disable(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!is_enabled(srv)) {
		return -EALREADY;
	}

	ctrl_disable(srv);
	store(srv, FLAG_STORE_STATE);

	return 0;
}

bool bt_mesh_light_ctrl_srv_is_on(struct bt_mesh_light_ctrl_srv *srv)
{
	return is_enabled(srv) && state_is_on(srv, srv->state);
}

int bt_mesh_light_ctrl_srv_pub(struct bt_mesh_light_ctrl_srv *srv,
			       struct bt_mesh_msg_ctx *ctx)
{
	return onoff_status_send(srv, ctx, srv->state);
}
