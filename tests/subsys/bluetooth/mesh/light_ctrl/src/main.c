/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/light_ctrl_srv.h>
#include <bluetooth/mesh/light_ctrl_reg_spec.h>
#include "light_ctrl_internal.h"

#define FLAGS_CONFIGURATION (BIT(FLAG_STARTED) | BIT(FLAG_OCC_MODE))

#define REG_ACCURACY(state_luxlevel) \
	(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_ACCURACY / 100.0 * (state_luxlevel) / 2)

/* Difference between In and In-1 in PI Regulator for the given Ki coefficient and U = 1. */
#define SUMMATION_STEP(ki) ((ki) * CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL / 1000)

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

static struct {
	bool enabled;
	uint16_t lightness;
	bool lightness_changed;
} pi_reg_test_ctx;

/** Mocks ******************************************/

static struct bt_mesh_model mock_lightness_model = { .elem_idx = 0 };

static struct bt_mesh_lightness_srv lightness_srv = {
	.lightness_model = &mock_lightness_model
};

static struct bt_mesh_light_ctrl_srv light_ctrl_srv =
	BT_MESH_LIGHT_CTRL_SRV_INIT(&lightness_srv);

static struct bt_mesh_model mock_ligth_ctrl_model = {
	.user_data = &light_ctrl_srv,
	.elem_idx = 1,
};

enum light_ctrl_timer {
	STATE_TIMER,
	ACTION_DELAY_TIMER,
	REG_TIMER,
};

static struct {
	k_work_handler_t handler;
	struct k_work_delayable *dwork;
} mock_timers[] = {
	[STATE_TIMER] = { .dwork = &light_ctrl_srv.timer },
	[ACTION_DELAY_TIMER] = { .dwork = &light_ctrl_srv.action_delay },
	[REG_TIMER] = { .dwork = NULL }
};

static int64_t mock_uptime;

void lightness_srv_change_lvl(struct bt_mesh_lightness_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_lightness_set *set,
			      struct bt_mesh_lightness_status *status,
			      bool publish)
{
	if (pi_reg_test_ctx.enabled) {
		pi_reg_test_ctx.lightness = set->lvl;
		pi_reg_test_ctx.lightness_changed = true;
		return;
	}

	ztest_check_expected_value(srv);
	zassert_is_null(ctx, "Context was not null");

	ztest_check_expected_value(set->lvl);
	ztest_check_expected_data(set->transition,
				  sizeof(struct bt_mesh_model_transition));
	/* status can not be tested as it is a dummy value for one call that can be anything. */
	zassert_true(publish, "Publication not done");
}

int lightness_on_power_up(struct bt_mesh_lightness_srv *srv)
{
	zassert_not_null(srv, "Context was null");
	return 0;
}

void bt_mesh_lightness_srv_set(struct bt_mesh_lightness_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_lightness_set *set,
			       struct bt_mesh_lightness_status *status)
{
	if (pi_reg_test_ctx.enabled) {
		pi_reg_test_ctx.lightness = set->lvl;
		pi_reg_test_ctx.lightness_changed = true;
		return;
	}

	/* Not implemented. */
	ztest_test_fail();
}

uint8_t model_transition_encode(int32_t transition_time)
{
	return 0;
}

int32_t model_transition_decode(uint8_t encoded_transition)
{
	return 0;
}

int32_t model_delay_decode(uint8_t encoded_delay)
{
	return 0;
}

struct bt_mesh_elem *bt_mesh_model_elem(struct bt_mesh_model *mod)
{
	return NULL;
}

struct bt_mesh_model *bt_mesh_model_find(const struct bt_mesh_elem *elem,
					 uint16_t id)
{
	return NULL;
}

int tid_check_and_update(struct bt_mesh_tid_ctx *prev_transaction, uint8_t tid,
			 const struct bt_mesh_msg_ctx *ctx)
{
	return 0;
}

int bt_mesh_msg_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
	       struct net_buf_simple *buf)
{
	ztest_check_expected_value(model);
	ztest_check_expected_value(ctx);
	ztest_check_expected_value(buf->len);
	ztest_check_expected_data(buf->data, buf->len);
	return 0;
}

int bt_mesh_onoff_srv_pub(struct bt_mesh_onoff_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_onoff_status *status)
{
	ztest_check_expected_value(srv);
	zassert_is_null(ctx, "Context was not null");
	ztest_check_expected_value(status->present_on_off);
	ztest_check_expected_value(status->target_on_off);
	ztest_check_expected_value(status->remaining_time);
	return 0;
}

void bt_mesh_model_msg_init(struct net_buf_simple *msg, uint32_t opcode)
{
	ztest_check_expected_value(opcode);
	net_buf_simple_init(msg, 0);
}

int bt_mesh_model_extend(struct bt_mesh_model *mod,
			 struct bt_mesh_model *base_mod)
{
	return 0;
}

void k_work_init_delayable(struct k_work_delayable *dwork,
			   k_work_handler_t handler)
{
	for (size_t i = 0; i < ARRAY_SIZE(mock_timers); i++) {
		if (mock_timers[i].dwork == dwork) {
			mock_timers[i].handler = handler;
			return;
		}
	}

	ztest_test_fail();
}

int k_work_cancel_delayable(struct k_work_delayable *dwork)
{
	ztest_check_expected_value(dwork);
	return 0;
}

/*
 * This is mocked, as k_work_reschedule is inline and can't be, but calls this
 * underneath
 */
int k_work_reschedule_for_queue(struct k_work_q *queue,
				struct k_work_delayable *dwork,
				k_timeout_t delay)
{
	zassert_equal(queue, &k_sys_work_q, "Not rescheduled on k_sys_work_q");
	ztest_check_expected_value(dwork);
	ztest_check_expected_data(&delay, sizeof(delay));
	return 0;
}

int k_work_schedule(struct k_work_delayable *dwork,
		    k_timeout_t delay)
{
	ztest_check_expected_value(dwork);
	ztest_check_expected_data(&delay, sizeof(delay));
	return 0;
}

int64_t z_impl_k_uptime_ticks(void)
{
	return mock_uptime;
}

k_ticks_t z_timeout_remaining(const struct _timeout *timeout)
{
	k_ticks_t ticks;

	ztest_copy_return_data(&ticks, sizeof(k_ticks_t));
	return ticks;
}

/** End Mocks **************************************/
static struct bt_mesh_model_transition expected_transition;
static struct bt_mesh_lightness_set expected_set = {
	.transition = &expected_transition
};
static uint8_t expected_msg[10];
static struct bt_mesh_onoff_status expected_onoff_status = { 0 };

static void schedule_dwork_timeout(enum light_ctrl_timer timer_idx)
{
	zassert_not_null(mock_timers[timer_idx].handler, "No timer handler");
	if (timer_idx == REG_TIMER) {
		mock_uptime += k_ms_to_ticks_ceil64(CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL);
	}
	mock_timers[timer_idx].handler(&mock_timers[timer_idx].dwork->work);
}

static void expect_timer_reschedule(enum light_ctrl_timer timer_idx, k_timeout_t *wait_timeout)
{
	ztest_expect_value(k_work_reschedule_for_queue, dwork, mock_timers[timer_idx].dwork);
	ztest_expect_data(k_work_reschedule_for_queue, &delay, wait_timeout);
}

static void expect_light_onoff_pub(uint8_t *expected_msg, size_t len, k_timeout_t *timeout)
{
	ztest_expect_value(bt_mesh_model_msg_init, opcode,
			   BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS);
	ztest_expect_value(bt_mesh_msg_send, model, &mock_ligth_ctrl_model);
	ztest_expect_value(bt_mesh_msg_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_send, buf->len, len);
	ztest_expect_data(bt_mesh_msg_send, buf->data, expected_msg);
	ztest_expect_value(bt_mesh_onoff_srv_pub, srv, &light_ctrl_srv.onoff);
	ztest_expect_value(bt_mesh_onoff_srv_pub, status->present_on_off,
			   expected_onoff_status.present_on_off);
	ztest_expect_value(bt_mesh_onoff_srv_pub, status->target_on_off,
			   expected_onoff_status.target_on_off);
	ztest_expect_value(bt_mesh_onoff_srv_pub, status->remaining_time,
			   expected_onoff_status.remaining_time);

	if (timeout != NULL) {
		ztest_return_data(z_timeout_remaining, &ticks, &timeout->ticks);
		/* Generic OnOff state publishing */
		ztest_return_data(z_timeout_remaining, &ticks, &timeout->ticks);
	}
}

static void expect_transition_start(k_timeout_t *timeout)
{
	expect_timer_reschedule(STATE_TIMER, timeout);

	if (pi_reg_test_ctx.enabled) {
		return;
	}

	ztest_expect_value(lightness_srv_change_lvl, srv, &lightness_srv);
	ztest_expect_value(lightness_srv_change_lvl, set->lvl,
			   expected_set.lvl);
	ztest_expect_data(lightness_srv_change_lvl, set->transition,
			  expected_set.transition);
}

static void expect_ctrl_disable(void)
{
	expected_msg[0] = 0;
	expected_onoff_status.present_on_off = false;
	expected_onoff_status.target_on_off = false;
	expected_onoff_status.remaining_time = 0;
	expect_light_onoff_pub(expected_msg, 1, NULL);

	ztest_expect_value(k_work_cancel_delayable, dwork, mock_timers[ACTION_DELAY_TIMER].dwork);
	ztest_expect_value(k_work_cancel_delayable, dwork, mock_timers[STATE_TIMER].dwork);
	ztest_expect_value(k_work_cancel_delayable, dwork, mock_timers[REG_TIMER].dwork);
}

static void expect_ctrl_enable(void)
{
	static k_timeout_t state_timeout;
	static k_timeout_t reg_start_timeout;

	expected_transition.time = 0;
	expected_set.lvl = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_STANDBY];

	state_timeout = K_MSEC(0);
	expect_transition_start(&state_timeout);

	reg_start_timeout = K_MSEC(CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL);
	ztest_expect_value(k_work_schedule, dwork, mock_timers[REG_TIMER].dwork);
	ztest_expect_data(k_work_schedule, &delay, &reg_start_timeout);
}

static void expect_turn_off_state_change(void)
{
	static k_timeout_t state_timeout;

	expected_transition.time = light_ctrl_srv.cfg.fade_standby_manual;
	expected_set.lvl = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_STANDBY];

	state_timeout = K_MSEC(expected_transition.time);
	expect_transition_start(&state_timeout);

	expected_msg[0] = 1;
	expected_msg[1] = 0;
	expected_msg[2] = 0;
	expected_onoff_status.present_on_off = true;
	expected_onoff_status.target_on_off = false;
	expected_onoff_status.remaining_time = light_ctrl_srv.cfg.fade_standby_manual;
	expect_light_onoff_pub(expected_msg, 3, &state_timeout);
}

static void expect_turn_on_state_change(void)
{
	static k_timeout_t state_timeout;

	expected_transition.time = light_ctrl_srv.cfg.fade_on;
	expected_set.lvl = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON];

	state_timeout = K_MSEC(expected_transition.time);
	expect_transition_start(&state_timeout);

	expected_msg[0] = 1;
	expected_msg[1] = 1;
	expected_msg[2] = 0;
	expected_onoff_status.present_on_off = true;
	expected_onoff_status.target_on_off = true;
	expected_onoff_status.remaining_time = light_ctrl_srv.cfg.fade_on;

	expect_light_onoff_pub(expected_msg, 3, &state_timeout);
}

static void
expected_statemachine_cond(bool enabled,
			   enum bt_mesh_light_ctrl_srv_state expected_state,
			   atomic_t expected_flags)
{
	if (enabled) {
		zassert_not_null(light_ctrl_srv.lightness->ctrl,
				 "Is not enabled.");
	} else {
		zassert_is_null(light_ctrl_srv.lightness->ctrl,
				"Is not disabled");
	}
	zassert_equal(light_ctrl_srv.state, expected_state, "Wrong state, expected: %d, got: %d",
		      expected_state, light_ctrl_srv.state);
	zassert_equal(light_ctrl_srv.flags, expected_flags,
		      "Wrong Flags: exp 0x%X : got 0x%X", expected_flags,
		      light_ctrl_srv.flags);
}

static struct sensor_value float_to_sensor_value(float fvalue)
{
	return (struct sensor_value) {
		       .val1 = (int32_t)(fvalue),
		       .val2 = ((int64_t)((fvalue) * 1000000.0f)) % 1000000L
	};
}

static float sensor_to_float(struct sensor_value *val)
{
	return val->val1 + val->val2 / 1000000.0f;
}

static void send_amb_lux_level(float amb)
{
	struct bt_mesh_msg_ctx ctx;
	struct sensor_value value;

	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_SENSOR_OP_STATUS);
	net_buf_simple_init(&buf, 0);

	value = float_to_sensor_value(amb);

	zassert_ok(sensor_status_id_encode(&buf, 3, BT_MESH_PROP_ID_PRESENT_AMB_LIGHT_LEVEL));
	zassert_ok(sensor_value_encode(&buf, &bt_mesh_sensor_present_amb_light_level, &value));

	_bt_mesh_light_ctrl_srv_op[9].func(&mock_ligth_ctrl_model, &ctx, &buf);
}

static void refresh_amb_light_level_timeout(void)
{
	int64_t timestamp_temp;

	/* Refresh ambient light level before the timer times out. */
	timestamp_temp = light_ctrl_srv.amb_light_level_timestamp;
	if (k_uptime_delta(&timestamp_temp) + CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL >=
	    CONFIG_BT_MESH_LIGHT_CTRL_AMB_LIGHT_LEVEL_TIMEOUT * MSEC_PER_SEC) {
		send_amb_lux_level(light_ctrl_srv.reg->measured);
	}
}

static void trigger_pi_reg(uint32_t steps, bool refresh)
{
	while (steps-- > 0) {
		k_timeout_t reg_timeout;

		/* Refresh ambient light level timeout so that measured is not reset to zero, when
		 * pi regulator is scheduled for a long time.
		 */
		if (refresh) {
			refresh_amb_light_level_timeout();
		}

		reg_timeout = K_MSEC(CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL);
		expect_timer_reschedule(REG_TIMER, &reg_timeout);
		schedule_dwork_timeout(REG_TIMER);
	}
}

static void setup(void *f)
{
	mock_timers[REG_TIMER].dwork = &CONTAINER_OF(
		light_ctrl_srv.reg, struct bt_mesh_light_ctrl_reg_spec, reg)->timer;
	zassert_not_null(_bt_mesh_light_ctrl_srv_cb.init, "Init cb is null");
	zassert_not_null(_bt_mesh_light_ctrl_srv_cb.start, "Start cb is null");

	zassert_ok(_bt_mesh_light_ctrl_srv_cb.init(&mock_ligth_ctrl_model),
		   "Init failed");
	expect_ctrl_disable();
	zassert_ok(_bt_mesh_light_ctrl_srv_cb.start(&mock_ligth_ctrl_model),
		   "Start failed");
}

static void teardown(void *f)
{
	zassert_not_null(_bt_mesh_light_ctrl_srv_cb.reset, "Reset cb is null");
	expect_ctrl_disable();
	_bt_mesh_light_ctrl_srv_cb.reset(&mock_ligth_ctrl_model);
}

static void enable_ctrl(void)
{
	enum bt_mesh_light_ctrl_srv_state expected_state = LIGHT_CTRL_STATE_STANDBY;
	atomic_t expected_flags = FLAGS_CONFIGURATION | BIT(FLAG_CTRL_SRV_MANUALLY_ENABLED)
				  | BIT(FLAG_REGULATOR);

	expect_ctrl_enable();
	bt_mesh_light_ctrl_srv_enable(&light_ctrl_srv);
	schedule_dwork_timeout(STATE_TIMER);
	expected_statemachine_cond(true, expected_state, expected_flags);
}

static void turn_on_ctrl(void)
{
	enum bt_mesh_light_ctrl_srv_state expected_state = LIGHT_CTRL_STATE_ON;
	atomic_t expected_flags = FLAGS_CONFIGURATION | BIT(FLAG_CTRL_SRV_MANUALLY_ENABLED)
				  | BIT(FLAG_ON) | BIT(FLAG_TRANSITION) | BIT(FLAG_REGULATOR);

	/* Start transition to On state. */
	expect_turn_on_state_change();
	bt_mesh_light_ctrl_srv_on(&light_ctrl_srv);
	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Finish transition to On state. */
	expected_msg[0] = 1;
	expected_onoff_status.present_on_off = true;
	expected_onoff_status.target_on_off = true;
	expected_onoff_status.remaining_time = 0;
	expect_light_onoff_pub(expected_msg, 1, NULL);

	k_timeout_t timeout = K_MSEC(light_ctrl_srv.cfg.on);

	expect_timer_reschedule(STATE_TIMER, &timeout);
	schedule_dwork_timeout(STATE_TIMER);
	expected_flags &= ~BIT(FLAG_TRANSITION);
	expected_statemachine_cond(true, expected_state, expected_flags);
	mock_uptime += timeout.ticks;
}

static void start_reg(float amb)
{
	atomic_t expected_flags = FLAGS_CONFIGURATION | BIT(FLAG_CTRL_SRV_MANUALLY_ENABLED)
				  | BIT(FLAG_ON) | BIT(FLAG_REGULATOR)
				  | BIT(FLAG_AMBIENT_LUXLEVEL_SET);

	/* Start the regulator by sending Sensor Status message. */
	send_amb_lux_level(amb);
	expected_statemachine_cond(true, LIGHT_CTRL_STATE_ON, expected_flags);
}

static void setup_pi_reg(void *f)
{
	pi_reg_test_ctx.enabled = true;
	pi_reg_test_ctx.lightness = 0;
	pi_reg_test_ctx.lightness_changed = false;

	/* Set Lightness Out value for each state to 0 so that PI Regulator output is always
	 * greater than Lightness Out state.
	 */
	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_STANDBY] = 0;
	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON] = 0;
	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_PROLONG] = 0;

	setup(f);
	enable_ctrl();
	turn_on_ctrl();
}

static void teardown_pi_reg(void *f)
{
	pi_reg_test_ctx.enabled = false;
	pi_reg_test_ctx.lightness = 0;
	pi_reg_test_ctx.lightness_changed = false;

	teardown(f);
}

ZTEST(light_ctrl_test, test_fsm_no_change_by_light_onoff)
{
	/* The light ctrl server is disable after init, and in OFF state */
	enum bt_mesh_light_ctrl_srv_state expected_state =
		LIGHT_CTRL_STATE_STANDBY;
	atomic_t expected_flags = FLAGS_CONFIGURATION;

	expected_statemachine_cond(false, expected_state, expected_flags);

	bt_mesh_light_ctrl_srv_on(&light_ctrl_srv);
	expected_statemachine_cond(false, expected_state, expected_flags);

	bt_mesh_light_ctrl_srv_off(&light_ctrl_srv);
	expected_statemachine_cond(false, expected_state, expected_flags);

	/* Enable light ctrl server, to enter STANDBY state */
	expected_flags = expected_flags | BIT(FLAG_CTRL_SRV_MANUALLY_ENABLED);
	expected_flags = expected_flags | BIT(FLAG_TRANSITION) | BIT(FLAG_REGULATOR);
	expect_ctrl_enable();
	bt_mesh_light_ctrl_srv_enable(&light_ctrl_srv);
	/* Start regulator manually to allow the test to check operation */
	light_ctrl_srv.reg->start(light_ctrl_srv.reg);

	/* Wait for transition to completed. */
	expected_flags = expected_flags & ~BIT(FLAG_TRANSITION);
	schedule_dwork_timeout(STATE_TIMER);

	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Light_Off should not change state or flags */
	bt_mesh_light_ctrl_srv_off(&light_ctrl_srv);
	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Light_On should change state to FADE_ON state */
	expected_state = LIGHT_CTRL_STATE_ON;
	expected_flags = expected_flags | BIT(FLAG_ON);
	expected_flags = expected_flags | BIT(FLAG_TRANSITION);
	expected_flags = expected_flags & ~BIT(FLAG_MANUAL);
	expect_turn_on_state_change();
	bt_mesh_light_ctrl_srv_on(&light_ctrl_srv);
	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Light_On should not change state or flags */
	bt_mesh_light_ctrl_srv_on(&light_ctrl_srv);
	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Light_Off should change state to FADE_STANDBY_MANUAL state */
	expected_state = LIGHT_CTRL_STATE_STANDBY;
	expected_flags = expected_flags | BIT(FLAG_MANUAL);
	expected_flags = expected_flags & ~BIT(FLAG_ON);
	/* Transition is not finished yet, Light Controller will calc remaining time. */
	k_ticks_t ticks = 50;

	/* Remaining time till target Lightness value. */
	ztest_return_data(z_timeout_remaining, &ticks, &ticks);
	/* Remaining time till target LuxLevel value. */
	ztest_return_data(z_timeout_remaining, &ticks, &ticks);
	expect_turn_off_state_change();
	bt_mesh_light_ctrl_srv_off(&light_ctrl_srv);
	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Light_Off should not change state or flags */
	bt_mesh_light_ctrl_srv_off(&light_ctrl_srv);
	expected_statemachine_cond(true, expected_state, expected_flags);
}

static void check_default_cfg(void)
{
	zassert_equal(light_ctrl_srv.cfg.occupancy_delay,
		      CONFIG_BT_MESH_LIGHT_CTRL_SRV_OCCUPANCY_DELAY, "Invalid default value");
	zassert_equal(light_ctrl_srv.cfg.fade_on, CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_FADE_ON,
		      "Invalid default value");
	zassert_equal(light_ctrl_srv.cfg.on, (MSEC_PER_SEC * CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_ON),
		      "Invalid default value");
	zassert_equal(light_ctrl_srv.cfg.fade_prolong,
		      CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_FADE_PROLONG, "Invalid default value");
	zassert_equal(light_ctrl_srv.cfg.prolong,
		      (MSEC_PER_SEC * CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_PROLONG),
		      "Invalid default value");
	zassert_equal(light_ctrl_srv.cfg.fade_standby_auto,
		      CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_FADE_STANDBY_AUTO,
		      "Invalid default value");
	zassert_equal(light_ctrl_srv.cfg.fade_standby_manual,
		      CONFIG_BT_MESH_LIGHT_CTRL_SRV_TIME_FADE_STANDBY_MANUAL,
		      "Invalid default value");

	zassert_equal(light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_STANDBY],
		      CONFIG_BT_MESH_LIGHT_CTRL_SRV_LVL_STANDBY, "Invalid default value");
	zassert_equal(light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON],
		      CONFIG_BT_MESH_LIGHT_CTRL_SRV_LVL_ON, "Invalid default value");
	zassert_equal(light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_PROLONG],
		      CONFIG_BT_MESH_LIGHT_CTRL_SRV_LVL_PROLONG, "Invalid default value");

	zassert_equal(light_ctrl_srv.reg->cfg.ki.up, CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_KIU,
		      "Invalid default value");
	zassert_equal(light_ctrl_srv.reg->cfg.ki.down, CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_KID,
		      "Invalid default value");
	zassert_equal(light_ctrl_srv.reg->cfg.kp.up, CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_KPU,
		      "Invalid default value");
	zassert_equal(light_ctrl_srv.reg->cfg.kp.down, CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_KPD,
		      "Invalid default value");

	zassert_equal(light_ctrl_srv.reg->cfg.accuracy, CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_ACCURACY,
		      "Invalid default value");

	zassert_mem_equal(&light_ctrl_srv.cfg.lux[LIGHT_CTRL_STATE_STANDBY],
			  &(struct sensor_value)
			  { .val1 = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_STANDBY },
			  sizeof(struct sensor_value), "Invalid default value");
	zassert_mem_equal(&light_ctrl_srv.cfg.lux[LIGHT_CTRL_STATE_ON],
			  &(struct sensor_value)
			  { .val1 = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON },
			  sizeof(struct sensor_value), "Invalid default value");
	zassert_mem_equal(&light_ctrl_srv.cfg.lux[LIGHT_CTRL_STATE_PROLONG],
			  &(struct sensor_value)
			  { .val1 = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_PROLONG },
			  sizeof(struct sensor_value), "Invalid default value");
}

ZTEST(light_ctrl_test, test_default_cfg)
{
	check_default_cfg();

	light_ctrl_srv.cfg.occupancy_delay = 3;
	light_ctrl_srv.cfg.fade_on = 5;
	light_ctrl_srv.cfg.on = 7;
	light_ctrl_srv.cfg.fade_prolong = 9;
	light_ctrl_srv.cfg.prolong = 11;
	light_ctrl_srv.cfg.fade_standby_auto = 13;
	light_ctrl_srv.cfg.fade_standby_manual = 15;

	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_STANDBY] = 17;
	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON] = 19;
	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_PROLONG] = 21;

	light_ctrl_srv.reg->cfg.ki.up = 3;
	light_ctrl_srv.reg->cfg.ki.down = 5;
	light_ctrl_srv.reg->cfg.kp.up = 7;
	light_ctrl_srv.reg->cfg.kp.down = 9;

	light_ctrl_srv.reg->cfg.accuracy = 11;

	light_ctrl_srv.cfg.lux[LIGHT_CTRL_STATE_STANDBY] = float_to_sensor_value(13);
	light_ctrl_srv.cfg.lux[LIGHT_CTRL_STATE_ON] = float_to_sensor_value(15);
	light_ctrl_srv.cfg.lux[LIGHT_CTRL_STATE_PROLONG] = float_to_sensor_value(17);

	expect_ctrl_disable();
	_bt_mesh_light_ctrl_srv_cb.reset(&mock_ligth_ctrl_model);
	expect_ctrl_disable();
	zassert_ok(_bt_mesh_light_ctrl_srv_cb.start(&mock_ligth_ctrl_model), "Start failed");

	check_default_cfg();
}

/**
 * Verify that PI Regulator keeps internal sum and its output within the range defined in
 * MeshMDLv1.0.1, table 6.53.
 */
ZTEST(light_ctrl_pi_reg_test, test_pi_regulator_shall_not_wrap_around)
{
	light_ctrl_srv.reg->cfg.ki.up = 10;
	light_ctrl_srv.reg->cfg.ki.down = 10;
	light_ctrl_srv.reg->cfg.kp.up = 5;
	light_ctrl_srv.reg->cfg.kp.down = 5;

	/* For test simplification, set Ambient LuxLevel to the highest possible value before
	 * LuxLevel On value so that U = E - D == 1.
	 */
	start_reg(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		  REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);
	trigger_pi_reg(65529, true);
	/* Expected lightness = 65529 * U * T * Kiu + U * Kpu = 65529 + 5 = 65534. */
	zassert_equal(pi_reg_test_ctx.lightness, 65534, "Incorrect lightness value: %d",
		      pi_reg_test_ctx.lightness);
	trigger_pi_reg(1, true);
	zassert_equal(pi_reg_test_ctx.lightness, 65535, "Incorrect lightness value: %d",
		      pi_reg_test_ctx.lightness);

	/* PI Regulator should stop changing lightness value. */
	pi_reg_test_ctx.lightness_changed = false;
	trigger_pi_reg(10, true);
	zassert_true(!pi_reg_test_ctx.lightness_changed, "Lightness value has been changed");

	/* For test simplification set Ambient LuxLevel to the lowest possible value after
	 * LuxLevel On value so that U = E + D == -1.
	 */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON +
			   REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) + 1);
	/* Trigger PI Regulator and check that internal sum didn't wrap around. */
	trigger_pi_reg(1, true);
	/* Expected lightness = 65535 + U * T * Kid + U * Kpd = 65535 - 1 - 5 = 65529. */
	zassert_equal(pi_reg_test_ctx.lightness, 65529, "Incorrect lightness value: %d",
		      pi_reg_test_ctx.lightness);

	/* Drive lightness value to 0 and check that it stops changing. */
	trigger_pi_reg(65528, true);
	/* Expected lightness = 65535 - 65529 * U * T * Kid - U * Kpd = 1. */
	zassert_equal(pi_reg_test_ctx.lightness, 1, "Incorrect lightness value: %d",
		      pi_reg_test_ctx.lightness);
	trigger_pi_reg(1, true);
	zassert_equal(pi_reg_test_ctx.lightness, 0, "Incorrect lightness value: %d",
		      pi_reg_test_ctx.lightness);

	/* PI Regulator should stop changing lightness value. */
	pi_reg_test_ctx.lightness_changed = false;
	trigger_pi_reg(10, true);
	zassert_true(!pi_reg_test_ctx.lightness_changed, "Lightness value has been changed");

	/* Trigger PI Regulator and check that internal sum didn't wrap around. */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
			   REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);
	trigger_pi_reg(1, true);
	zassert_equal(pi_reg_test_ctx.lightness, 6, "Incorrect lightness value: %d",
		      pi_reg_test_ctx.lightness);
}

/**
 * Verify that accumulated values are reset after model reset.
 */
ZTEST(light_ctrl_pi_reg_test, test_pi_regulator_after_reset)
{
	uint16_t expected_lightness;

	light_ctrl_srv.reg->cfg.ki.up = 200;
	light_ctrl_srv.reg->cfg.kp.up = 80;

	/* Test that reg.i is reset when the controller is disabled. */
	start_reg(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		  REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);
	trigger_pi_reg(14, true);
	expected_lightness = SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 14 +
			     light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	teardown_pi_reg(NULL);
	setup_pi_reg(NULL);
	start_reg(0);

	/* Set different ambient light level than it was before the restart. The regulator should
	 * calculate the output value with internal sum set to zero.
	 */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
			   REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 2);
	trigger_pi_reg(6, true);
	expected_lightness = (SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 6 +
			      light_ctrl_srv.reg->cfg.kp.up) * 2;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Test that reg.prev is also reset when the controller is disabled. */
	teardown_pi_reg(NULL);
	setup_pi_reg(NULL);
	start_reg(0);

	/* setup_pi_reg() starts the regulator with the zero ambient light level. The regulator
	 * will drive the lightness high up.
	 */
	trigger_pi_reg(1, true);
	float input = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		      REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON);
	expected_lightness = SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up * input) +
			     input * light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	teardown_pi_reg(NULL);
	setup_pi_reg(NULL);
	start_reg(0);

	/* reg.prev should have been reset when controller was stopped. The controller should set
	 * the same lightness value as it was before the restart.
	 */
	pi_reg_test_ctx.lightness_changed = false;
	trigger_pi_reg(1, true);
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);
	zassert_true(pi_reg_test_ctx.lightness_changed, "Lightness value has not changed");
}

/**
 * Verify that PI Regulator doesn't change lightness value when the adjustment error within the
 * accuracy.
 */
ZTEST(light_ctrl_pi_reg_test, test_pi_regulator_accuracy)
{
	uint32_t expected_lightness;

	/* Make PI Regulator accumulate internal sum. */
	start_reg(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		  REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);
	trigger_pi_reg(10, true);

	/* Set Ambient LuxLevel within the accuracy range. */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
			     REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) + 0.01);

	/* Now PI Regulator will exclude Kp coefficient from lightness value and will return
	 * what was already accumulated.
	 */
	expected_lightness = pi_reg_test_ctx.lightness - light_ctrl_srv.reg->cfg.kp.up;
	trigger_pi_reg(1, true);
	zassert_equal(expected_lightness, pi_reg_test_ctx.lightness,
		      "Got incorrect lightness, expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Now PI Regulator shall not change lightness value since Ambient LuxLevel within the
	 * accuracy range.
	 */
	pi_reg_test_ctx.lightness_changed = false;
	trigger_pi_reg(100, true);
	zassert_true(!pi_reg_test_ctx.lightness_changed, "Lightness value has been changed");

	/* Repeat the same with another boundary within the range. */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON +
			     REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 0.01);
	pi_reg_test_ctx.lightness_changed = false;
	trigger_pi_reg(100, true);
	zassert_true(!pi_reg_test_ctx.lightness_changed, "Lightness value has been changed");
}

/**
 * Verify that PI Regulator correctly uses Kiu, Kid, Kpu and Kpd coffeicients.
 */
ZTEST(light_ctrl_pi_reg_test, test_pi_regulator_coefficients)
{
	uint16_t expected_lightness;

	/* Check that PI Regulator doesn't accumulate error when only proportional part is used. */
	light_ctrl_srv.reg->cfg.ki.up = 0;
	light_ctrl_srv.reg->cfg.kp.up = 80;

	/* For test simplification, set Ambient LuxLevel to the highest possible value before
	 * LuxLevel On value so that U = E - D == 1.
	 */
	start_reg(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		  REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);

	/* Trigger PI Regulator multiple times to check that it doesn't accumulate error. */
	trigger_pi_reg(10, true);

	/* Expected lightness value when only proportional part is used:
	 * Lightness = Kpu * U = Kpu * (E - D); E - D == 1 => Lightness = Kpu.
	 */
	expected_lightness = light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Check that PI Regulator accumulates error when only integral part is used. */
	light_ctrl_srv.reg->cfg.ki.up = 200;
	light_ctrl_srv.reg->cfg.kp.up = 0;

	trigger_pi_reg(5, true);

	expected_lightness = SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 5;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Check that PI Regulator doesn't decrement accumulated error when only proportional part
	 * is used.
	 */
	light_ctrl_srv.reg->cfg.ki.down = 0;
	light_ctrl_srv.reg->cfg.kp.down = 8;

	/* For test simplification set Ambient LuxLevel to the lowest possible value after
	 * LuxLevel On value so that U = E + D == -1.
	 */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON +
			     REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) + 1);

	/* Trigger PI Regulator multiple times to check that it doesn't accumulate error. */
	trigger_pi_reg(5, true);

	/* Expected lightness value when integral part is not used:
	 * Lightness = In + Kpd * U = In + Kpd * (E + D); E + D == -1 => Lightness = In - Kpd.
	 */
	expected_lightness = SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 5 -
			     light_ctrl_srv.reg->cfg.kp.down;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Check that PI Regulator decrements accumulated value when only integral part is used. */
	light_ctrl_srv.reg->cfg.ki.down = 100;
	light_ctrl_srv.reg->cfg.kp.down = 0;

	trigger_pi_reg(5, true);

	expected_lightness = SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 5 -
			     SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.down) * 5;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);
}

static void switch_to_prolong_state(void)
{
	enum bt_mesh_light_ctrl_srv_state expected_state = LIGHT_CTRL_STATE_PROLONG;
	atomic_t expected_flags = FLAGS_CONFIGURATION | BIT(FLAG_CTRL_SRV_MANUALLY_ENABLED)
				  | BIT(FLAG_ON) | BIT(FLAG_TRANSITION) | BIT(FLAG_REGULATOR)
				  | BIT(FLAG_AMBIENT_LUXLEVEL_SET);

	light_ctrl_srv.cfg.fade_prolong = 0;

	expected_transition.time = 0;
	expected_set.lvl = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_PROLONG];

	/* Start transition to Prolong state. */
	k_timeout_t timeout = K_MSEC(0);

	expect_timer_reschedule(STATE_TIMER, &timeout);
	schedule_dwork_timeout(STATE_TIMER);
	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Finish transition. */
	expected_flags &= ~BIT(FLAG_TRANSITION);
	timeout = K_MSEC(light_ctrl_srv.cfg.prolong);
	expect_timer_reschedule(STATE_TIMER, &timeout);
	schedule_dwork_timeout(STATE_TIMER);
	expected_statemachine_cond(true, expected_state, expected_flags);
}

static void switch_to_standby_state(void)
{
	enum bt_mesh_light_ctrl_srv_state expected_state = LIGHT_CTRL_STATE_STANDBY;
	atomic_t expected_flags = FLAGS_CONFIGURATION | BIT(FLAG_CTRL_SRV_MANUALLY_ENABLED)
				  | BIT(FLAG_TRANSITION) | BIT(FLAG_REGULATOR)
				  | BIT(FLAG_AMBIENT_LUXLEVEL_SET);

	light_ctrl_srv.cfg.fade_standby_auto = 0;

	expected_transition.time = 0;
	expected_set.lvl = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_STANDBY];

	k_timeout_t timeout = K_MSEC(0);

	expect_transition_start(&timeout);

	expected_msg[0] = 0;
	expected_onoff_status.present_on_off = false;
	expected_onoff_status.target_on_off = false;
	expected_onoff_status.remaining_time = 0;
	expect_light_onoff_pub(expected_msg, 1, &timeout);

	/* Start transition to Standby state. */
	schedule_dwork_timeout(STATE_TIMER);
	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Finish transition. */
	expected_flags &= ~BIT(FLAG_TRANSITION);
	schedule_dwork_timeout(STATE_TIMER);
	expected_statemachine_cond(true, expected_state, expected_flags);
}

/**
 * Verify that PI Regulator uses correct target LuxLevel values for On, Prolong and Standby states.
 */
ZTEST(light_ctrl_pi_reg_test, test_target_luxlevel_for_pi_regulator)
{
	uint16_t expected_lightness;

	/* Preconfigure test. */
	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_STANDBY] = 50;
	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON] = 200;
	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_PROLONG] = 100;

	/* Light controller is On state already. */

	/* Set Ambient LuxLevel to the highest possible value before LuxLevel On value and
	 * check that PI Regulator changes lightness value according to coefficients.
	 */
	start_reg(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		  REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);
	trigger_pi_reg(10, true);
	expected_lightness = SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 10 +
			     light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Set Ambient LuxLevel to target LuxLevel to stop PI Regulator changing lightness value. */
	send_amb_lux_level(sensor_to_float(&light_ctrl_srv.cfg.lux[LIGHT_CTRL_STATE_ON]));
	expected_lightness -= light_ctrl_srv.reg->cfg.kp.up;
	trigger_pi_reg(1, true);
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Test Prolong state. */
	switch_to_prolong_state();

	/* Set Ambient LuxLevel to the highest possible value before LuxLevel Prolong value and
	 * check that PI Regulator changes lightness value according to coefficients.
	 */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_PROLONG -
			     REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_PROLONG) - 1);
	trigger_pi_reg(4, true);
	expected_lightness += SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 4 +
			      light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Set Ambient LuxLevel to target LuxLevel to stop PI Regulator changing lightness value. */
	send_amb_lux_level(sensor_to_float(&light_ctrl_srv.cfg.lux[LIGHT_CTRL_STATE_PROLONG]));
	expected_lightness -= light_ctrl_srv.reg->cfg.kp.up;
	trigger_pi_reg(1, true);
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Test Standby state. */
	switch_to_standby_state();

	/* Set Ambient LuxLevel to the highest possible value before LuxLevel Prolong value and
	 * check that PI Regulator changes lightness value according to coefficients.
	 */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_STANDBY -
			     REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_STANDBY) - 1);
	trigger_pi_reg(10, true);
	expected_lightness += SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 10 +
			      light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);
}

/**
 * Verify that Light LC Linear Output = max((Lightness Out ^ 2) / 65535, Regulator Output)
 */
ZTEST(light_ctrl_pi_reg_test, test_linear_output_state)
{
	uint16_t expected_lightness;

	/* For test simplification, set Ambient LuxLevel to the highest possible value before
	 * LuxLevel On value so that U = E - D == 1.
	 */
	start_reg(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		  REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);
	trigger_pi_reg(10, true);
	expected_lightness = SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 10 +
			     light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Set Lightness On state to a value higher than PI Regulator output,
	 * trigger PI Regulator and check that the Lightness values is set to
	 * Lightness On state.
	 */
	light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON] = 10000;
	trigger_pi_reg(1, true);
	expected_lightness = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON];
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);
}

ZTEST_SUITE(light_ctrl_test, NULL, NULL, setup, teardown, NULL);
ZTEST_SUITE(light_ctrl_pi_reg_test, NULL, NULL, setup_pi_reg, teardown_pi_reg, NULL);
