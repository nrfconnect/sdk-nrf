/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <bluetooth/mesh/light_ctrl_srv.h>
#include <bluetooth/mesh/light_ctrl_reg.h>
#include <bluetooth/mesh/sensor_types.h>
#include <bluetooth/mesh/properties.h>
#include "lightness_internal.h"
#include "light_ctrl_internal.h"
#include "gen_onoff_internal.h"
#include "sensor.h"
#include "model_utils.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_light_ctrl_srv);

#define FLAGS_CONFIGURATION (BIT(FLAG_STARTED) | BIT(FLAG_OCC_MODE))

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
	FLAG_CTRL_SRV_MANUALLY_ENABLED,
	FLAG_STARTED,
	FLAG_RESUME_TIMER,
	FLAG_AMBIENT_LUXLEVEL_SET,
};

enum stored_flags {
	STORED_FLAG_ON = FLAG_ON,
	STORED_FLAG_OCC_MODE = FLAG_OCC_MODE,
	STORED_FLAG_ENABLED,
};

struct setup_srv_storage_data {
	struct bt_mesh_light_ctrl_srv_cfg cfg;
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	struct bt_mesh_light_ctrl_reg_cfg reg_cfg;
#endif
};

static void restart_timer(struct bt_mesh_light_ctrl_srv *srv, uint32_t delay)
{
	k_work_reschedule(&srv->timer, K_MSEC(delay));
}

static void reg_start(struct bt_mesh_light_ctrl_srv *srv)
{
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (srv->reg && srv->reg->start) {
		srv->reg->start(srv->reg);
	}
#endif
}

static void reg_stop(struct bt_mesh_light_ctrl_srv *srv)
{
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (srv->reg && srv->reg->stop) {
		srv->reg->stop(srv->reg);
	}
#endif
}


static inline uint32_t to_centi_lux(const struct sensor_value *lux)
{
	return lux->val1 * 100L + lux->val2 / 10000L;
}

static inline void from_centi_lux(uint32_t centi_lux, struct sensor_value *lux)
{
	lux->val1 = centi_lux / 100L;
	lux->val2 = centi_lux % 10000L;
}

static void store(struct bt_mesh_light_ctrl_srv *srv, enum flags kind)
{
#if CONFIG_BT_SETTINGS
	atomic_set_bit(&srv->flags, kind);

	bt_mesh_model_data_store_schedule(srv->model);
#endif
}

static bool is_enabled(const struct bt_mesh_light_ctrl_srv *srv)
{
	return srv->lightness->ctrl == srv;
}

static void schedule_resume_timer(struct bt_mesh_light_ctrl_srv *srv)
{
	if (srv->resume) {
		k_work_reschedule(&srv->timer, K_SECONDS(srv->resume));
		atomic_set_bit(&srv->flags, FLAG_RESUME_TIMER);
	}
}

static uint32_t delay_remaining(struct bt_mesh_light_ctrl_srv *srv)
{
	return k_ticks_to_ms_ceil32(
		k_work_delayable_remaining_get(&srv->action_delay));
}

static int delayed_change(struct bt_mesh_light_ctrl_srv *srv, bool value,
			  uint32_t delay)
{
	if (!is_enabled(srv)) {
		return -EBUSY;
	}

	if (((value && (atomic_test_bit(&srv->flags, FLAG_ON_PENDING) ||
			atomic_test_bit(&srv->flags, FLAG_OCC_PENDING))) ||
	     (!value && atomic_test_bit(&srv->flags, FLAG_OFF_PENDING))) &&
	    (delay_remaining(srv) < delay)) {
		/* Trying to do a second delayed change that will finish later
		 * with the same result. Can be safely ignored.
		 */
		return -EALREADY;
	}

	/* Clear all pending transitions - the last one triggered should be the
	 * one that gets executed. Note that the check above prevents this from
	 * delaying transitions.
	 */
	atomic_clear_bit(&srv->flags, FLAG_ON_PENDING);
	atomic_clear_bit(&srv->flags, FLAG_OFF_PENDING);
	atomic_clear_bit(&srv->flags, FLAG_OCC_PENDING);

	k_work_reschedule(&srv->action_delay, K_MSEC(delay));

	return 0;
}

static void delayed_occupancy(struct bt_mesh_light_ctrl_srv *srv,
			      const struct sensor_value *time_since_motion)
{
	int err;

	int32_t ms_since_motion = time_since_motion->val1 * MSEC_PER_SEC;

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

static void light_set(struct bt_mesh_light_ctrl_srv *srv, uint16_t lvl,
		      uint32_t transition_time)
{
	struct bt_mesh_model_transition transition = { transition_time };
	struct bt_mesh_lightness_set set = {
		.lvl = lvl,
		.transition = &transition,
	};
	struct bt_mesh_lightness_status dummy;

	lightness_srv_change_lvl(srv->lightness, NULL, &set, &dummy, true);
}

static uint32_t remaining_fade_time(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!atomic_test_bit(&srv->flags, FLAG_TRANSITION)) {
		return 0;
	}

	return k_ticks_to_ms_ceil32(k_work_delayable_remaining_get(&srv->timer));
}

static uint32_t curr_fade_time(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!atomic_test_bit(&srv->flags, FLAG_TRANSITION)) {
		return 0;
	}

	uint32_t remaining = remaining_fade_time(srv);

	return srv->fade.duration > remaining ? srv->fade.duration - remaining : 0;
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

static void light_onoff_encode(struct bt_mesh_light_ctrl_srv *srv,
			       struct net_buf_simple *buf,
			       enum bt_mesh_light_ctrl_srv_state prev_state)
{
	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS);
	net_buf_simple_add_u8(buf, state_is_on(srv, prev_state));

	uint32_t remaining_fade = remaining_fade_time(srv);

	if (remaining_fade) {
		net_buf_simple_add_u8(buf,
				      srv->state != LIGHT_CTRL_STATE_STANDBY);
		net_buf_simple_add_u8(buf,
				      model_transition_encode(remaining_fade));
		return;
	}

	if (atomic_test_bit(&srv->flags, FLAG_ON_PENDING) ||
	    atomic_test_bit(&srv->flags, FLAG_OFF_PENDING)) {
		remaining_fade = srv->fade.duration;
		net_buf_simple_add_u8(buf, atomic_test_bit(&srv->flags,
							   FLAG_ON_PENDING));
		net_buf_simple_add_u8(buf,
				      model_transition_encode(remaining_fade));
		return;
	}
}

static int light_onoff_status_send(struct bt_mesh_light_ctrl_srv *srv,
				   struct bt_mesh_msg_ctx *ctx,
				   enum bt_mesh_light_ctrl_srv_state prev_state)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS,
				 3);
	light_onoff_encode(srv, &buf, prev_state);

	return bt_mesh_msg_send(srv->model, ctx, &buf);
}

static void onoff_encode(struct bt_mesh_light_ctrl_srv *srv,
			 enum bt_mesh_light_ctrl_srv_state prev_state,
			 struct bt_mesh_onoff_status *status)
{
	uint32_t remaining_fade;

	if (atomic_test_bit(&srv->flags, FLAG_ON_PENDING) ||
	    atomic_test_bit(&srv->flags, FLAG_OFF_PENDING)) {
		remaining_fade = srv->fade.duration;
		status->target_on_off = atomic_test_bit(&srv->flags,
							FLAG_ON_PENDING);
	} else {
		remaining_fade = remaining_fade_time(srv);
		status->target_on_off = srv->state != LIGHT_CTRL_STATE_STANDBY;
	}

	status->present_on_off = state_is_on(srv, prev_state);
	status->remaining_time = remaining_fade;
}

static void light_onoff_pub(struct bt_mesh_light_ctrl_srv *srv,
			    enum bt_mesh_light_ctrl_srv_state prev_state,
			    bool pub_gen_onoff)
{
	struct bt_mesh_onoff_status status;

	(void)light_onoff_status_send(srv, NULL, prev_state);

	if (!pub_gen_onoff) {
		return;
	}

	onoff_encode(srv, prev_state, &status);

	(void)bt_mesh_onoff_srv_pub(&srv->onoff, NULL, &status);
}

static uint16_t light_get(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!is_enabled(srv)) {
		return 0;
	}

	if (!atomic_test_bit(&srv->flags, FLAG_TRANSITION) ||
	    !srv->fade.duration) {
		return srv->cfg.light[srv->state];
	}

	uint32_t curr = curr_fade_time(srv);
	uint16_t start = srv->fade.initial_light;
	uint16_t end = srv->cfg.light[srv->state];

	/* Linear interpolation: */
	return start + ((end - start) * curr) / srv->fade.duration;
}

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG

static float sensor_to_float(struct sensor_value *val)
{
	return val->val1 + val->val2 / 1000000.0f;
}

static void lux_get(struct bt_mesh_light_ctrl_srv *srv,
		    struct sensor_value *lux)
{
	if (!is_enabled(srv)) {
		memset(lux, 0, sizeof(*lux));
		return;
	}

	if (!atomic_test_bit(&srv->flags, FLAG_TRANSITION) ||
	    !srv->fade.duration) {
		*lux = srv->cfg.lux[srv->state];
		return;
	}

	uint32_t delta = curr_fade_time(srv);
	uint32_t init = to_centi_lux(&srv->fade.initial_lux);
	uint32_t cfg = to_centi_lux(&srv->cfg.lux[srv->state]);
	uint32_t centi_lux = init + ((cfg - init) * delta) / srv->fade.duration;

	from_centi_lux(centi_lux, lux);
}

static float lux_getf(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!is_enabled(srv)) {
		return 0.0f;
	}

	return to_centi_lux(&srv->cfg.lux[srv->state]) / 100.0f;
}

static void reg_updated(struct bt_mesh_light_ctrl_reg *reg, float value)
{
	struct bt_mesh_light_ctrl_srv *srv = (struct bt_mesh_light_ctrl_srv *)(reg->user_data);
	uint16_t output = CLAMP(value, 0, UINT16_MAX);
	/* The regulator output is always in linear format. We'll convert to
	 * the configured representation again before calling the Lightness
	 * server.
	 */
	uint16_t lvl = to_linear(light_get(srv));

	/* Output value is max out of regulator and configured level. */
	if (output <= lvl) {
		output = lvl;
	}

	if (!atomic_test_and_clear_bit(&srv->flags, FLAG_REGULATOR) &&
	    output == srv->reg_prev) {
		return;
	}
	srv->reg_prev = output;

	struct bt_mesh_lightness_set set = {
		/* Regulator output is linear, but lightness works in the configured representation.
		 */
		.lvl = from_linear(output),
	};

	bt_mesh_lightness_srv_set(srv->lightness, NULL, &set,
				  &(struct bt_mesh_lightness_status){});
}

#else

static void lux_get(struct bt_mesh_light_ctrl_srv *srv,
		    struct sensor_value *lux)
{
	memset(lux, 0, sizeof(*lux));
}

#endif

static void transition_start(struct bt_mesh_light_ctrl_srv *srv,
			     enum bt_mesh_light_ctrl_srv_state state,
			     uint32_t fade_time)
{
	srv->state = state;
	srv->fade.initial_light = light_get(srv);
	srv->fade.duration = fade_time;
	lux_get(srv, &srv->fade.initial_lux);

	atomic_set_bit(&srv->flags, FLAG_TRANSITION);
	if (!IS_ENABLED(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG) ||
	    !atomic_test_bit(&srv->flags, FLAG_AMBIENT_LUXLEVEL_SET)) {
		/* If the regulator is enabled and Ambient LuxLevel value has been provided, the
		 * regulator will provide a light value to an application according to new state
		 * and target value.
		 */
		light_set(srv, srv->cfg.light[state], fade_time);
	}
	restart_timer(srv, fade_time);
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (srv->reg) {
		atomic_set_bit(&srv->flags, FLAG_REGULATOR);
		bt_mesh_light_ctrl_reg_target_set(srv->reg, lux_getf(srv), fade_time);
	}
#endif
}

static int turn_on(struct bt_mesh_light_ctrl_srv *srv,
		   const struct bt_mesh_model_transition *transition,
		   bool pub_gen_onoff)
{
	if (!is_enabled(srv)) {
		return -EBUSY;
	}

	LOG_DBG("Light Turned On");

	uint32_t fade_time;

	if (transition) {
		fade_time = transition->time;
	} else {
		fade_time = srv->cfg.fade_on;
	}

	if (srv->state != LIGHT_CTRL_STATE_ON) {
		enum bt_mesh_light_ctrl_srv_state prev_state = srv->state;

		if (prev_state == LIGHT_CTRL_STATE_STANDBY) {
			atomic_set_bit(&srv->flags, FLAG_ON);
			atomic_clear_bit(&srv->flags, FLAG_MANUAL);
			store(srv, FLAG_STORE_STATE);
		}

		transition_start(srv, LIGHT_CTRL_STATE_ON, fade_time);
		light_onoff_pub(srv, prev_state, pub_gen_onoff);
	} else if (!atomic_test_bit(&srv->flags, FLAG_TRANSITION)) {
		/* Delay the move to prolong */
		restart_timer(srv, srv->cfg.on);
	}

	return 0;
}

static void turn_off_auto(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!is_enabled(srv) || srv->state != LIGHT_CTRL_STATE_PROLONG) {
		return;
	}

	LOG_DBG("Light Automatically Turn Off");

	transition_start(srv, LIGHT_CTRL_STATE_STANDBY,
			 srv->cfg.fade_standby_auto);

	atomic_clear_bit(&srv->flags, FLAG_ON);
	store(srv, FLAG_STORE_STATE);
	light_onoff_pub(srv, LIGHT_CTRL_STATE_PROLONG, true);
}

static int turn_off(struct bt_mesh_light_ctrl_srv *srv,
		    const struct bt_mesh_model_transition *transition,
		    bool pub_gen_onoff)
{
	if (!is_enabled(srv)) {
		return -EBUSY;
	}

	LOG_DBG("Light Turned Off");

	uint32_t fade_time =
		transition ? transition->time : srv->cfg.fade_standby_manual;
	enum bt_mesh_light_ctrl_srv_state prev_state = srv->state;

	if (prev_state != LIGHT_CTRL_STATE_STANDBY) {
		atomic_set_bit(&srv->flags, FLAG_MANUAL);
		transition_start(srv, LIGHT_CTRL_STATE_STANDBY, fade_time);
		atomic_clear_bit(&srv->flags, FLAG_ON);
		store(srv, FLAG_STORE_STATE);
		light_onoff_pub(srv, prev_state, pub_gen_onoff);
	} else if (atomic_test_bit(&srv->flags, FLAG_TRANSITION) &&
		   !atomic_test_bit(&srv->flags, FLAG_MANUAL)) {
		atomic_set_bit(&srv->flags, FLAG_MANUAL);
		transition_start(srv, LIGHT_CTRL_STATE_STANDBY, fade_time);
	}

	return 0;
}

static void prolong(struct bt_mesh_light_ctrl_srv *srv)
{
	if (!is_enabled(srv) || srv->state != LIGHT_CTRL_STATE_ON) {
		return;
	}

	LOG_DBG("Light Prolonged");

	transition_start(srv, LIGHT_CTRL_STATE_PROLONG, srv->cfg.fade_prolong);
}

static void ctrl_enable(struct bt_mesh_light_ctrl_srv *srv)
{
	atomic_clear_bit(&srv->flags, FLAG_RESUME_TIMER);
	atomic_clear_bit(&srv->flags, FLAG_AMBIENT_LUXLEVEL_SET);
	srv->lightness->ctrl = srv;
	LOG_DBG("Enable Light Control");
	transition_start(srv, LIGHT_CTRL_STATE_STANDBY, 0);
	/* Regulator remains stopped until fresh LuxLevel is received. */
}

static void ctrl_disable(struct bt_mesh_light_ctrl_srv *srv)
{
	/* Do not change the light level, disabling the control server just
	 * disengages it from the Lightness Server.
	 */

	/* Clear transient state: */
	atomic_and(&srv->flags, FLAGS_CONFIGURATION);
	srv->lightness->ctrl = NULL;
	srv->state = LIGHT_CTRL_STATE_STANDBY;
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	srv->reg_prev = 0;
#endif

	LOG_DBG("Disable Light Control");

	/* If any of these cancel calls fail, their handler will exit early on
	 * their is_enabled() checks:
	 */
	k_work_cancel_delayable(&srv->action_delay);
	k_work_cancel_delayable(&srv->timer);

	reg_stop(srv);

	light_onoff_pub(srv, srv->state, true);
}

/*******************************************************************************
 * Timeouts
 ******************************************************************************/

static void timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_light_ctrl_srv *srv =
		CONTAINER_OF(dwork, struct bt_mesh_light_ctrl_srv, timer);

	if (!is_enabled(srv)) {
		if (srv->resume && atomic_test_and_clear_bit(&srv->flags, FLAG_RESUME_TIMER)) {
			LOG_DBG("Resuming LC server");
			ctrl_enable(srv);
			store(srv, FLAG_STORE_STATE);
		}
		return;
	}

	/* According to test spec, we should publish the OnOff state at the end
	 * of the transition:
	 */
	if (atomic_test_and_clear_bit(&srv->flags, FLAG_TRANSITION)) {
		LOG_DBG("Transition complete");

		/* If the fade wasn't instant, we've already published the
		 * steady state in the state change function.
		 */
		if (srv->fade.duration > 0) {
			light_onoff_pub(srv, srv->state, true);
		}

		if (srv->state == LIGHT_CTRL_STATE_ON) {
			restart_timer(srv, srv->cfg.on);
			return;
		}

		if (srv->state == LIGHT_CTRL_STATE_PROLONG) {
			restart_timer(srv, srv->cfg.prolong);
			return;
		}

		/* If we're in manual mode, wait until the end of the cooldown
		 * period before disabling it:
		 */
		if (!atomic_test_bit(&srv->flags, FLAG_MANUAL)) {
			return;
		}

		uint32_t cooldown = MSEC_PER_SEC *
				    CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_MANUAL;

		if (srv->fade.duration >= cooldown) {
			atomic_clear_bit(&srv->flags, FLAG_MANUAL);
			return;
		}

		restart_timer(srv, cooldown - srv->fade.duration);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	if (srv->state == LIGHT_CTRL_STATE_ON) {
		prolong(srv);
		return;
	}

	if (srv->state == LIGHT_CTRL_STATE_PROLONG) {
		turn_off_auto(srv);
		return;
	}

	/* Ends the sensor cooldown period: */
	atomic_clear_bit(&srv->flags, FLAG_MANUAL);
}

static void delayed_action_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_light_ctrl_srv *srv = CONTAINER_OF(
		dwork, struct bt_mesh_light_ctrl_srv, action_delay);
	struct bt_mesh_model_transition transition = {
		.time = srv->fade.duration
	};

	if (!is_enabled(srv)) {
		return;
	}

	LOG_DBG("Delayed Action Timeout");

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
static void store_cfg_data(struct bt_mesh_light_ctrl_srv *srv)
{
	if (atomic_test_and_clear_bit(&srv->flags, FLAG_STORE_CFG)) {
		struct setup_srv_storage_data data = {
			.cfg = srv->cfg,
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
			.reg_cfg = srv->reg->cfg,
#endif
		};

		(void)bt_mesh_model_data_store(srv->setup_srv, false, NULL,
					       &data, sizeof(data));
	}
}

static void store_state_data(struct bt_mesh_light_ctrl_srv *srv)
{
	if (atomic_test_and_clear_bit(&srv->flags, FLAG_STORE_STATE)) {
		atomic_t data = 0;

		atomic_set_bit_to(&data, STORED_FLAG_ENABLED, is_enabled(srv));
		atomic_set_bit_to(&data, STORED_FLAG_ON,
				  atomic_test_bit(&srv->flags, FLAG_ON));
		atomic_set_bit_to(&data, STORED_FLAG_OCC_MODE,
				  atomic_test_bit(&srv->flags, FLAG_OCC_MODE));

		(void)bt_mesh_model_data_store(srv->model, false, NULL, &data,
					       sizeof(data));
	}

}

static void ligth_ctrl_srv_pending_store(struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	LOG_DBG("Store Timeout");

	store_cfg_data(srv);
	store_state_data(srv);
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

	bt_mesh_msg_send(srv->model, ctx, &rsp);
}

static int handle_mode_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	mode_rsp(srv, ctx);

	return 0;
}

static int mode_set(struct bt_mesh_light_ctrl_srv *srv,
		    struct net_buf_simple *buf)
{
	uint8_t mode = net_buf_simple_pull_u8(buf);

	if (mode > 1) {
		return -EINVAL;
	}

	LOG_DBG("Set Mode: %s", mode ? "enable" : "disable");

	if (mode) {
		bt_mesh_light_ctrl_srv_enable(srv);
	} else {
		bt_mesh_light_ctrl_srv_disable(srv);
	}

	return 0;
}

static int handle_mode_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	int err;

	err = mode_set(srv, buf);
	if (err) {
		return err;
	}

	mode_rsp(srv, ctx);

	return 0;
}

static int handle_mode_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	return mode_set(srv, buf);
}

static void om_rsp(struct bt_mesh_light_ctrl_srv *srv,
		   struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHT_CTRL_OP_OM_STATUS, 1);
	bt_mesh_model_msg_init(&rsp, BT_MESH_LIGHT_CTRL_OP_OM_STATUS);
	net_buf_simple_add_u8(&rsp,
			      atomic_test_bit(&srv->flags, FLAG_OCC_MODE));

	bt_mesh_msg_send(srv->model, ctx, &rsp);
}

static int handle_om_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	om_rsp(srv, ctx);

	return 0;
}

static int om_set(struct bt_mesh_light_ctrl_srv *srv,
		  struct net_buf_simple *buf)
{
	uint8_t mode = net_buf_simple_pull_u8(buf);

	if (mode > 1) {
		return -EINVAL;
	}

	LOG_DBG("Set OM: %s", mode ? "on" : "off");

	atomic_set_bit_to(&srv->flags, FLAG_OCC_MODE, mode);
	store(srv, FLAG_STORE_STATE);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	return 0;
}

static int handle_om_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	int err;

	err = om_set(srv, buf);
	if (err) {
		return err;
	}

	om_rsp(srv, ctx);

	return 0;
}

static int handle_om_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	return om_set(srv, buf);
}

static int handle_light_onoff_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	LOG_DBG("Get Light OnOff");

	light_onoff_status_send(srv, ctx, srv->state);

	return 0;
}

static int light_onoff_set(struct bt_mesh_light_ctrl_srv *srv, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf, bool ack)
{
	uint8_t onoff = net_buf_simple_pull_u8(buf);

	if (onoff > 1) {
		return -EINVAL;
	}

	uint8_t tid = net_buf_simple_pull_u8(buf);

	struct bt_mesh_model_transition transition;
	bool has_trans = !!model_transition_get(srv->model, &transition, buf);

	enum bt_mesh_light_ctrl_srv_state prev_state = srv->state;

	if (!tid_check_and_update(&srv->tid, tid, ctx)) {
		LOG_DBG("Set Light OnOff: %s (%u + %u)", onoff ? "on" : "off",
		       has_trans ? transition.time : 0,
		       has_trans ? transition.delay : 0);

		if (has_trans && transition.delay > 0) {
			delayed_set(srv, &transition, onoff);
		} else if (onoff) {
			turn_on(srv, has_trans ? &transition : NULL, true);
		} else {
			turn_off(srv, has_trans ? &transition : NULL, true);
		}

		if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
			bt_mesh_scene_invalidate(srv->model);
		}
	}

	if (ack) {
		light_onoff_status_send(srv, ctx, prev_state);
	}

	return 0;
}

static int handle_light_onoff_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	return light_onoff_set(srv, ctx, buf, true);
}

static int handle_light_onoff_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	return light_onoff_set(srv, ctx, buf, false);
}

static int handle_sensor_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	int err;

	if (!is_enabled(srv)) {
		return 0;
	}

	while (buf->len >= 3) {
		uint8_t len;
		uint16_t id;
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
			return -ENOENT;
		}

		LOG_DBG("Sensor 0x%04x: %s", id, bt_mesh_sensor_ch_str(&value));

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
		if (id == BT_MESH_PROP_ID_PRESENT_AMB_LIGHT_LEVEL && srv->reg) {
			srv->reg->measured = sensor_to_float(&value);
			if (!atomic_test_and_set_bit(&srv->flags, FLAG_AMBIENT_LUXLEVEL_SET)) {
				reg_start(srv);
			}
			continue;
		}
#endif

		/* Occupancy sensor */

		/* OCC_MODE must be enabled for the occupancy sensors to be
		 * able to turn on the light:
		 */
		if (srv->state == LIGHT_CTRL_STATE_STANDBY &&
		    !atomic_test_bit(&srv->flags, FLAG_OCC_MODE) &&
		    !(atomic_test_bit(&srv->flags, FLAG_TRANSITION) &&
		      !atomic_test_bit(&srv->flags, FLAG_MANUAL))) {
			continue;
		}

		if (id == BT_MESH_PROP_ID_TIME_SINCE_MOTION_SENSED) {
			delayed_occupancy(srv, &value);
		} else if (value.val1 > 0) {
			turn_on(srv, NULL, true);
		}
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_light_ctrl_srv_op[] = {
	{
		BT_MESH_LIGHT_CTRL_OP_MODE_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_MODE_GET),
		handle_mode_get,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_MODE_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_MODE_SET),
		handle_mode_set,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_MODE_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_MODE_SET),
		handle_mode_set_unack,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_OM_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_OM_GET),
		handle_om_get,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_OM_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_OM_SET),
		handle_om_set,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_OM_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_OM_SET),
		handle_om_set_unack,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_LIGHT_ONOFF_GET),
		handle_light_onoff_get,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTRL_MSG_MINLEN_LIGHT_ONOFF_SET),
		handle_light_onoff_set,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTRL_MSG_MINLEN_LIGHT_ONOFF_SET),
		handle_light_onoff_set_unack,
	},
	{
		BT_MESH_SENSOR_OP_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_SENSOR_MSG_MINLEN_STATUS),
		handle_sensor_status,
	},
	BT_MESH_MODEL_OP_END,
};

static void to_prop_time(uint32_t time, struct sensor_value *prop)
{
	prop->val1 = time / MSEC_PER_SEC;
	prop->val2 = (time % MSEC_PER_SEC) * 1000;
}

static uint32_t from_prop_time(const struct sensor_value *prop)
{
	return prop->val1 * MSEC_PER_SEC + prop->val2 / 1000;
}

static int prop_get(struct net_buf_simple *buf,
		    const struct bt_mesh_light_ctrl_srv *srv, uint16_t id)
{
	struct sensor_value val = { 0 };
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (srv->reg) {
		switch (id) {
		/* Regulator coefficients are raw IEEE-754 floats, push them straight
		 * to the buffer instead of using sensor to encode them:
		 */
		case BT_MESH_LIGHT_CTRL_COEFF_KID:
			net_buf_simple_add_mem(buf, &srv->reg->cfg.ki.down, sizeof(float));
			return 0;
		case BT_MESH_LIGHT_CTRL_COEFF_KIU:
			net_buf_simple_add_mem(buf, &srv->reg->cfg.ki.up, sizeof(float));
			return 0;
		case BT_MESH_LIGHT_CTRL_COEFF_KPD:
			net_buf_simple_add_mem(buf, &srv->reg->cfg.kp.down, sizeof(float));
			return 0;
		case BT_MESH_LIGHT_CTRL_COEFF_KPU:
			net_buf_simple_add_mem(buf, &srv->reg->cfg.kp.up, sizeof(float));
			return 0;
		case BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY:
			val.val1 = srv->reg->cfg.accuracy;
			break;
		}
	}
	switch (id) {
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON:
		val = srv->cfg.lux[LIGHT_CTRL_STATE_ON];
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG:
		val = srv->cfg.lux[LIGHT_CTRL_STATE_PROLONG];
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY:
		val = srv->cfg.lux[LIGHT_CTRL_STATE_STANDBY];
		break;
#else
	switch (id) {
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY:
#endif
	case BT_MESH_LIGHT_CTRL_COEFF_KID:
	case BT_MESH_LIGHT_CTRL_COEFF_KIU:
	case BT_MESH_LIGHT_CTRL_COEFF_KPD:
	case BT_MESH_LIGHT_CTRL_COEFF_KPU:
	case BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY:
		break; /* Prevent returning -ENOENT */
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_ON:
		val.val1 = to_actual(srv->cfg.light[LIGHT_CTRL_STATE_ON]);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_PROLONG:
		val.val1 = to_actual(srv->cfg.light[LIGHT_CTRL_STATE_PROLONG]);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_STANDBY:
		val.val1 = to_actual(srv->cfg.light[LIGHT_CTRL_STATE_STANDBY]);
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
		    struct bt_mesh_light_ctrl_srv *srv, uint16_t id)
{
	struct sensor_value val;
	int err;

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	/* Regulator coefficients are raw IEEE-754 floats, pull them straight
	 * from the buffer instead of using sensor to decode them:
	 */
	if (srv->reg) {
		switch (id) {
		case BT_MESH_LIGHT_CTRL_COEFF_KID:
			memcpy(&srv->reg->cfg.ki.down,
			net_buf_simple_pull_mem(buf, sizeof(float)),
			sizeof(float));
			return 0;
		case BT_MESH_LIGHT_CTRL_COEFF_KIU:
			memcpy(&srv->reg->cfg.ki.up,
			net_buf_simple_pull_mem(buf, sizeof(float)),
			sizeof(float));
			return 0;
		case BT_MESH_LIGHT_CTRL_COEFF_KPD:
			memcpy(&srv->reg->cfg.kp.down,
			net_buf_simple_pull_mem(buf, sizeof(float)),
			sizeof(float));
			return 0;
		case BT_MESH_LIGHT_CTRL_COEFF_KPU:
			memcpy(&srv->reg->cfg.kp.up,
			net_buf_simple_pull_mem(buf, sizeof(float)),
			sizeof(float));
			return 0;
		}
	}
#endif

	err = prop_decode(buf, id, &val);
	if (err) {
		return err;
	}

	if (buf->len > 0) {
		LOG_ERR("Invalid message size");
		return -EMSGSIZE;
	}

	LOG_DBG("Set Prop: 0x%04x: %s", id, bt_mesh_sensor_ch_str(&val));

	switch (id) {
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	case BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY:
		if (srv->reg) {
			srv->reg->cfg.accuracy = val.val1;
		}
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON:
		srv->cfg.lux[LIGHT_CTRL_STATE_ON] = val;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG:
		srv->cfg.lux[LIGHT_CTRL_STATE_PROLONG] = val;
		break;
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY:
		srv->cfg.lux[LIGHT_CTRL_STATE_STANDBY] = val;
		break;
#else
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_ON:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_PROLONG:
	case BT_MESH_LIGHT_CTRL_PROP_ILLUMINANCE_STANDBY:
	case BT_MESH_LIGHT_CTRL_PROP_REG_ACCURACY:
#endif
	case BT_MESH_LIGHT_CTRL_COEFF_KID:
	case BT_MESH_LIGHT_CTRL_COEFF_KIU:
	case BT_MESH_LIGHT_CTRL_COEFF_KPD:
	case BT_MESH_LIGHT_CTRL_COEFF_KPU:
		break; /* Prevent returning -ENOENT */
	/* Properties are always set in light actual representation: */
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_ON:
		srv->cfg.light[LIGHT_CTRL_STATE_ON] = from_actual(val.val1);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_PROLONG:
		srv->cfg.light[LIGHT_CTRL_STATE_PROLONG] = from_actual(val.val1);
		break;
	case BT_MESH_LIGHT_CTRL_PROP_LIGHTNESS_STANDBY:
		srv->cfg.light[LIGHT_CTRL_STATE_STANDBY] = from_actual(val.val1);
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

static int prop_tx(struct bt_mesh_light_ctrl_srv *srv,
		    struct bt_mesh_msg_ctx *ctx, uint16_t id)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(
		buf, BT_MESH_LIGHT_CTRL_OP_PROP_STATUS,
		2 + CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_PROP_STATUS);
	net_buf_simple_add_le16(&buf, id);

	err = prop_get(&buf, srv, id);
	if (err) {
		return -ENOENT;
	}

	bt_mesh_msg_send(srv->setup_srv, ctx, &buf);

	return 0;
}

static int handle_prop_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	uint16_t id = net_buf_simple_pull_le16(buf);

	return prop_tx(srv, ctx, id);
}

static int handle_prop_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	uint16_t id = net_buf_simple_pull_le16(buf);
	int err;

	err = prop_set(buf, srv, id);
	if (err) {
		return err;
	}

	(void)prop_tx(srv, ctx, id);
	(void)prop_tx(srv, NULL, id);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	store(srv, FLAG_STORE_CFG);

	return 0;
}

static int handle_prop_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	int err;

	uint16_t id = net_buf_simple_pull_le16(buf);

	err = prop_set(buf, srv, id);
	if (err) {
		return err;
	}

	(void)prop_tx(srv, NULL, id);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	store(srv, FLAG_STORE_CFG);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_light_ctrl_setup_srv_op[] = {
	{
		BT_MESH_LIGHT_CTRL_OP_PROP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_PROP_GET),
		handle_prop_get,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_PROP_SET,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTRL_MSG_MINLEN_PROP_SET),
		handle_prop_set,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_PROP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTRL_MSG_MINLEN_PROP_SET),
		handle_prop_set_unack,
	},
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
	enum bt_mesh_light_ctrl_srv_state prev_state = srv->state;

	if (set->transition && set->transition->delay > 0) {
		delayed_set(srv, set->transition, set->on_off);
	} else if (set->on_off) {
		turn_on(srv, set->transition, false);
	} else {
		turn_off(srv, set->transition, false);
	}

	if (!rsp) {
		return;
	}

	onoff_encode(srv, prev_state, rsp);
}

static void onoff_get(struct bt_mesh_onoff_srv *onoff,
		      struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_onoff_status *rsp)
{
	struct bt_mesh_light_ctrl_srv *srv =
		CONTAINER_OF(onoff, struct bt_mesh_light_ctrl_srv, onoff);

	onoff_encode(srv, bt_mesh_light_ctrl_srv_is_on(srv), rsp);
}

const struct bt_mesh_onoff_srv_handlers _bt_mesh_light_ctrl_srv_onoff = {
	.set = onoff_set,
	.get = onoff_get,
};

struct __packed scene_data {
	uint8_t enabled:1,
		occ:1,
		light:1;
	struct bt_mesh_light_ctrl_srv_cfg cfg;
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	struct bt_mesh_light_ctrl_reg_cfg reg;
#endif
	uint16_t lightness;
};

static ssize_t scene_store(struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	struct bt_mesh_lightness_status light = { 0 };
	struct scene_data *scene = (struct scene_data *)&data[0];

	scene->enabled = is_enabled(srv);
	scene->occ = atomic_test_bit(&srv->flags, FLAG_OCC_MODE);
	scene->light = atomic_test_bit(&srv->flags, FLAG_ON);
	scene->cfg = srv->cfg;
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (srv->reg) {
		scene->reg = srv->reg->cfg;
	}
#endif

	srv->lightness->handlers->light_get(srv->lightness, NULL, &light);
	scene->lightness = to_actual(light.remaining_time ? light.target : light.current);

	return sizeof(struct scene_data);
}

static void scene_recall(struct bt_mesh_model *model, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	struct scene_data *scene = (struct scene_data *)&data[0];

	atomic_set_bit_to(&srv->flags, FLAG_OCC_MODE, scene->occ);
	srv->cfg = scene->cfg;
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (srv->reg) {
		srv->reg->cfg = scene->reg;
	}
#endif
	if (scene->enabled) {
		if (!is_enabled(srv)) {
			ctrl_enable(srv);
		}

		if (!!scene->light && !atomic_test_bit(&srv->flags, FLAG_ON)) {
			turn_on(srv, transition, true);
		} else if (atomic_test_bit(&srv->flags, FLAG_ON)) {
			transition_start(srv, LIGHT_CTRL_STATE_STANDBY, 0);
			light_onoff_pub(srv, srv->state, true);
		}
	} else {
		struct bt_mesh_lightness_status status;
		struct bt_mesh_lightness_set set = {
			.lvl = from_actual(scene->lightness),
			.transition = transition,
		};

		ctrl_disable(srv);
		if (atomic_test_bit(&srv->flags, FLAG_RESUME_TIMER)) {
			schedule_resume_timer(srv);
		}
		lightness_srv_change_lvl(srv->lightness, NULL, &set, &status, true);
	}
}

/*  MeshMDL1.0.1, section 5.1.3.1.1:
 *  If a model is extending another model, the extending model shall determine
 *  the Stored with Scene behavior of that model.
 *
 *  Use Setup Model to handle Scene Store/Recall as it is not extended
 *  by other models.
 */
BT_MESH_SCENE_ENTRY_SIG(light_ctrl) = {
	.id.sig = BT_MESH_MODEL_ID_LIGHT_LC_SETUPSRV,
	.maxlen = sizeof(struct scene_data),
	.store = scene_store,
	.recall = scene_recall,
};

static int update_handler(struct bt_mesh_model *model)
{
	LOG_DBG("Update Handler");

	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	light_onoff_encode(srv, srv->pub.msg, srv->state);

	return 0;
}

static int light_ctrl_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	int err;

	srv->model = model;

	if (srv->lightness->lightness_model == NULL ||
	    srv->lightness->lightness_model->elem_idx >= model->elem_idx) {
		LOG_ERR("Lightness: Invalid element index");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LIGHT_CTRL_SRV_OCCUPANCY_MODE)) {
		atomic_set_bit(&srv->flags, FLAG_OCC_MODE);
	}

	k_work_init_delayable(&srv->timer, timeout);
	k_work_init_delayable(&srv->action_delay, delayed_action_timeout);

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (srv->reg) {
		struct bt_mesh_light_ctrl_reg_cfg reg_cfg = BT_MESH_LIGHT_CTRL_SRV_REG_CFG_INIT;

		srv->reg->updated = reg_updated;
		srv->reg->user_data = srv;
		srv->reg->cfg = reg_cfg;
		if (srv->reg->init) {
			srv->reg->init(srv->reg);
		}
	}
#endif

	srv->resume = CONFIG_BT_MESH_LIGHT_CTRL_SRV_RESUME_DELAY;

	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	err = bt_mesh_model_extend(model, srv->lightness->lightness_model);
	if (err) {
		return err;
	}

	err = bt_mesh_model_extend(model, srv->onoff.model);
	if (err) {
		return err;
	}

	atomic_set_bit(&srv->lightness->flags, LIGHTNESS_SRV_FLAG_EXTENDED_BY_LIGHT_CTRL);

	atomic_set_bit(&srv->onoff.flags, GEN_ONOFF_SRV_NO_DTT);

	return 0;
}

static int light_ctrl_srv_settings_set(struct bt_mesh_model *model,
				       const char *name, size_t len_rd,
				       settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	atomic_t data;
	ssize_t result;

	if (name) {
		return -EINVAL;
	}

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
		srv->lightness->ctrl = srv;
	}

	return 0;
}

static int light_ctrl_srv_start(struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;

	atomic_set_bit(&srv->flags, FLAG_STARTED);

	switch (srv->lightness->ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_OFF:
		if (atomic_test_bit(&srv->flags,
				    FLAG_CTRL_SRV_MANUALLY_ENABLED)) {
			ctrl_enable(srv);
		} else {
			lightness_on_power_up(srv->lightness);
			ctrl_disable(srv);
			schedule_resume_timer(srv);
		}

		/* PTS Corner case: If the device restarts while in the On state
		 * (with OnPowerUp != RESTORE), we'll end up here with lights
		 * off. If OnPowerUp is then changed to RESTORE, and the device
		 * restarts, we'll restore to On even though we were off in the
		 * previous power cycle, unless we store the Off state here.
		 */
		store(srv, FLAG_STORE_STATE);
		break;
	case BT_MESH_ON_POWER_UP_ON:
		if (atomic_test_bit(&srv->flags,
				    FLAG_CTRL_SRV_MANUALLY_ENABLED)) {
			ctrl_enable(srv);
		} else {
			lightness_on_power_up(srv->lightness);
			ctrl_disable(srv);
			schedule_resume_timer(srv);
		}
		store(srv, FLAG_STORE_STATE);
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		if (is_enabled(srv)) {
			reg_start(srv);
			if (atomic_test_bit(&srv->flags, FLAG_ON)) {
				turn_on(srv, NULL, true);
			} else {
				light_onoff_pub(srv, srv->state, true);
			}
		} else if (srv->lightness->transient.is_on) {
			lightness_on_power_up(srv->lightness);
			schedule_resume_timer(srv);
		} else {
			schedule_resume_timer(srv);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void light_ctrl_srv_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	struct bt_mesh_light_ctrl_srv_cfg cfg = BT_MESH_LIGHT_CTRL_SRV_CFG_INIT;
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	struct bt_mesh_light_ctrl_reg_cfg reg_cfg = BT_MESH_LIGHT_CTRL_SRV_REG_CFG_INIT;
#endif

	srv->cfg = cfg;
#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (srv->reg) {
		srv->reg->cfg = reg_cfg;
	}
#endif

	ctrl_disable(srv);
	net_buf_simple_reset(srv->pub.msg);
	srv->resume = CONFIG_BT_MESH_LIGHT_CTRL_SRV_RESUME_DELAY;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->setup_srv, false, NULL,
					       NULL, 0);
		(void)bt_mesh_model_data_store(srv->model, false, NULL,
					       NULL, 0);
	}
}

const struct bt_mesh_model_cb _bt_mesh_light_ctrl_srv_cb = {
	.init = light_ctrl_srv_init,
	.start = light_ctrl_srv_start,
	.reset = light_ctrl_srv_reset,
	.settings_set = light_ctrl_srv_settings_set,
#if CONFIG_BT_SETTINGS
	.pending_store = ligth_ctrl_srv_pending_store,
#endif
};

static int lc_setup_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	int err;

	srv->setup_srv = model;

	err = bt_mesh_model_extend(srv->setup_srv, srv->model);
	if (err) {
		return err;
	}

	srv->setup_pub.msg = &srv->setup_pub_buf;
	net_buf_simple_init_with_data(&srv->setup_pub_buf, srv->setup_pub_data,
				      sizeof(srv->setup_pub_data));

	return 0;
}

static int lc_setup_srv_settings_set(struct bt_mesh_model *model,
				     const char *name, size_t len_rd,
				     settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_light_ctrl_srv *srv = model->user_data;
	struct setup_srv_storage_data data;
	ssize_t result;

	if (name) {
		return -ENOENT;
	}

	result = read_cb(cb_arg, &data, sizeof(data));
	if (result <= 0) {
		return result;
	}

	if (result < sizeof(data)) {
		return -EINVAL;
	}

	srv->cfg = data.cfg;

#if CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG
	if (srv->reg) {
		srv->reg->cfg = data.reg_cfg;
	}
#endif

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_ctrl_setup_srv_cb = {
	.init = lc_setup_srv_init,
	.settings_set = lc_setup_srv_settings_set,
};

/*******************************************************************************
 * Public API
 ******************************************************************************/

int bt_mesh_light_ctrl_srv_on(struct bt_mesh_light_ctrl_srv *srv)
{
	int err;

	err = turn_on(srv, NULL, true);
	if (!err && IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	return err;
}

int bt_mesh_light_ctrl_srv_off(struct bt_mesh_light_ctrl_srv *srv)
{
	int err;

	err = turn_off(srv, NULL, true);
	if (!err && IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	return err;
}

int bt_mesh_light_ctrl_srv_enable(struct bt_mesh_light_ctrl_srv *srv)
{
	atomic_set_bit(&srv->flags, FLAG_CTRL_SRV_MANUALLY_ENABLED);
	if (is_enabled(srv)) {
		return -EALREADY;
	}

	if (atomic_test_bit(&srv->flags, FLAG_STARTED)) {
		ctrl_enable(srv);
		store(srv, FLAG_STORE_STATE);
		if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
			bt_mesh_scene_invalidate(srv->model);
		}
	}

	return 0;
}

int bt_mesh_light_ctrl_srv_disable(struct bt_mesh_light_ctrl_srv *srv)
{
	atomic_clear_bit(&srv->flags, FLAG_CTRL_SRV_MANUALLY_ENABLED);

	if (!is_enabled(srv)) {
		if (atomic_test_bit(&srv->flags, FLAG_RESUME_TIMER)) {
			/* Restart resume timer even
			 * if the server has already been disabled:
			 */
			schedule_resume_timer(srv);
		}
		return -EALREADY;
	}

	ctrl_disable(srv);
	schedule_resume_timer(srv);
	store(srv, FLAG_STORE_STATE);
	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	return 0;
}

bool bt_mesh_light_ctrl_srv_is_on(struct bt_mesh_light_ctrl_srv *srv)
{
	return is_enabled(srv) && state_is_on(srv, srv->state);
}

int bt_mesh_light_ctrl_srv_pub(struct bt_mesh_light_ctrl_srv *srv,
			       struct bt_mesh_msg_ctx *ctx)
{
	return light_onoff_status_send(srv, ctx, srv->state);
}
