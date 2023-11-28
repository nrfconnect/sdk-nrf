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

static const struct bt_mesh_model mock_lightness_model = {
	.rt = &(struct bt_mesh_model_rt_ctx){.elem_idx = 0}};

static struct bt_mesh_lightness_srv lightness_srv = {.lightness_model = &mock_lightness_model};

static struct bt_mesh_light_ctrl_srv light_ctrl_srv = BT_MESH_LIGHT_CTRL_SRV_INIT(&lightness_srv);

static const struct bt_mesh_model mock_ligth_ctrl_model = {
	.rt = &(struct bt_mesh_model_rt_ctx){.user_data = &light_ctrl_srv, .elem_idx = 1}};

enum light_ctrl_timer {
	STATE_TIMER,
	ACTION_DELAY_TIMER,
	REG_TIMER,
};

static struct {
	k_work_handler_t handler;
	struct k_work_delayable *dwork;
	struct k_sem sem;
} mock_timers[] = {
	[STATE_TIMER] = { .dwork = &light_ctrl_srv.timer, },
	[ACTION_DELAY_TIMER] = { .dwork = &light_ctrl_srv.action_delay },
	[REG_TIMER] = { .dwork = NULL }
};

static void state_timer_handler_wrapper(struct k_work *work)
{
	mock_timers[STATE_TIMER].handler(work);
	k_sem_give(&mock_timers[STATE_TIMER].sem);
}

static void (*reg_updated_cb)(struct bt_mesh_light_ctrl_reg *reg, float output);

static void reg_updated_handler_wrapper(struct bt_mesh_light_ctrl_reg *reg, float value)
{
	reg_updated_cb(reg, value);
	k_sem_give(&mock_timers[REG_TIMER].sem);
}

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

const struct bt_mesh_elem *bt_mesh_model_elem(const struct bt_mesh_model *mod)
{
	return NULL;
}

const struct bt_mesh_model *bt_mesh_model_find(const struct bt_mesh_elem *elem,
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

int bt_mesh_model_extend(const struct bt_mesh_model *mod,
			 const struct bt_mesh_model *base_mod)
{
	return 0;
}

/** End Mocks **************************************/
static struct bt_mesh_model_transition expected_transition;
static struct bt_mesh_lightness_set expected_set = {
	.transition = &expected_transition
};
static uint8_t expected_msg[10];
static struct bt_mesh_onoff_status expected_onoff_status = { 0 };

static void wait_for_timeout(enum light_ctrl_timer timer_idx, k_timeout_t timeout)
{
	int err;

	/* Action delay timer is not supported in the test. */
	zassert_true(timer_idx != ACTION_DELAY_TIMER);

	err = k_sem_take(&mock_timers[timer_idx].sem, timeout);
	zassert_equal(err, 0);
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
}

static void expect_transition_start(k_timeout_t *timeout)
{
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
}

static void expect_ctrl_enable(void)
{
	static k_timeout_t state_timeout;

	expected_transition.time = 0;
	expected_set.lvl = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_STANDBY];

	state_timeout = K_MSEC(0);
	expect_transition_start(&state_timeout);
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
	/* z_add_timeout() adds one more tick which results in additional 10ms because of
	 * CONFIG_SYS_CLOCK_TICKS_PER_SEC value.
	 */
	expected_onoff_status.remaining_time = light_ctrl_srv.cfg.fade_standby_manual + 10;
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
	/* z_add_timeout() adds one more tick which results in additional 10ms because of
	 * CONFIG_SYS_CLOCK_TICKS_PER_SEC value.
	 */
	expected_onoff_status.remaining_time = light_ctrl_srv.cfg.fade_on + 10;

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
	k_sem_reset(&mock_timers[REG_TIMER].sem);

	while (steps-- > 0) {
		/* Refresh ambient light level timeout so that measured is not reset to zero, when
		 * pi regulator is scheduled for a long time.
		 */
		if (refresh) {
			refresh_amb_light_level_timeout();
		}

		wait_for_timeout(REG_TIMER,
				 K_MSEC(CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL + 1));
	}
}

static void setup(void *f)
{
	zassert_not_null(_bt_mesh_light_ctrl_srv_cb.init, "Init cb is null");
	zassert_not_null(_bt_mesh_light_ctrl_srv_cb.start, "Start cb is null");

	zassert_ok(_bt_mesh_light_ctrl_srv_cb.init(&mock_ligth_ctrl_model),
		   "Init failed");

	/* Mock regulator timer handler. */
	mock_timers[REG_TIMER].dwork = &CONTAINER_OF(
		light_ctrl_srv.reg, struct bt_mesh_light_ctrl_reg_spec, reg)->timer;
	k_sem_init(&mock_timers[REG_TIMER].sem, 0, 1);
	reg_updated_cb = light_ctrl_srv.reg->updated;
	light_ctrl_srv.reg->updated = reg_updated_handler_wrapper;

	/* Mock state timer handler. */
	mock_timers[STATE_TIMER].handler = light_ctrl_srv.timer.work.handler;
	light_ctrl_srv.timer.work.handler = state_timer_handler_wrapper;
	k_sem_init(&mock_timers[STATE_TIMER].sem, 0, 1);

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
	wait_for_timeout(STATE_TIMER, K_MSEC(1));
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

	wait_for_timeout(STATE_TIMER, K_MSEC(light_ctrl_srv.cfg.on + 1));
	expected_flags &= ~BIT(FLAG_TRANSITION);
	expected_statemachine_cond(true, expected_state, expected_flags);
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
	light_ctrl_srv.reg->start(light_ctrl_srv.reg, 0);

	/* Wait for transition to completed. */
	expected_flags = expected_flags & ~BIT(FLAG_TRANSITION);
	wait_for_timeout(STATE_TIMER, K_MSEC(light_ctrl_srv.cfg.fade_on + 1));

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
	/* Tweak internal sum of the regulator to avoid long execution time.*/
	((struct bt_mesh_light_ctrl_reg_spec *)light_ctrl_srv.reg)->i = 65528 *
		SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up);
	trigger_pi_reg(1, true);
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
	/* Tweak internal sum of the regulator to avoid long execution time.*/
	((struct bt_mesh_light_ctrl_reg_spec *)light_ctrl_srv.reg)->i = 65535 - 65528 *
		SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up);
	trigger_pi_reg(1, true);
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
	/* Decremt recalculated value at start of the regulator. */
	expected_lightness -= SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up);
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	teardown_pi_reg(NULL);
	light_ctrl_srv.reg->cfg.ki.up = 200;
	light_ctrl_srv.reg->cfg.kp.up = 80;
	setup_pi_reg(NULL);

	/* Set different ambient light level than it was before the restart. The regulator should
	 * calculate the output value with internal sum set to zero.
	 */
	start_reg(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		  REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 2);
	trigger_pi_reg(6, true);
	expected_lightness = (SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 6 +
			      light_ctrl_srv.reg->cfg.kp.up) * 2;
	/* Decremt recalculated value at start of the regulator. */
	expected_lightness -= SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 2;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Test that reg.prev is also reset when the controller is disabled. */
	teardown_pi_reg(NULL);
	light_ctrl_srv.reg->cfg.ki.up = 200;
	light_ctrl_srv.reg->cfg.kp.up = 80;
	setup_pi_reg(NULL);

	/* setup_pi_reg() starts the regulator with the zero ambient light level. The regulator
	 * will drive the lightness high up.
	 *
	 * Because it is the first step after starting the regulator, only proportional part will
	 * be represented in the output value.
	 */
	start_reg(0);
	trigger_pi_reg(1, true);
	float input = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		      REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON);
	expected_lightness = input * light_ctrl_srv.reg->cfg.kp.up;
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
	wait_for_timeout(STATE_TIMER, K_MSEC(light_ctrl_srv.cfg.on + 1));
	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Finish transition. */
	expected_flags &= ~BIT(FLAG_TRANSITION);
	wait_for_timeout(STATE_TIMER, K_MSEC(light_ctrl_srv.cfg.fade_prolong + 1));
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
	wait_for_timeout(STATE_TIMER, K_MSEC(light_ctrl_srv.cfg.prolong + 1));
	expected_statemachine_cond(true, expected_state, expected_flags);

	/* Finish transition. */
	expected_flags &= ~BIT(FLAG_TRANSITION);
	wait_for_timeout(STATE_TIMER, K_MSEC(light_ctrl_srv.cfg.fade_standby_auto + 1));
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
	expected_lightness = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON] +
			     SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 10 +
			     light_ctrl_srv.reg->cfg.kp.up;
	/* Decremt recalculated value at start of the regulator. */
	expected_lightness -= SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up);
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Set Ambient LuxLevel to target LuxLevel to stop PI Regulator changing lightness value. */
	send_amb_lux_level(sensor_to_float(&light_ctrl_srv.cfg.lux[LIGHT_CTRL_STATE_ON]));
	/* It will take long time to finish the test so that the regulator will drop the last
	 * measured ambient value. Don't let it do that.
	 */
	light_ctrl_srv.amb_light_level_timestamp = (uint32_t)-1;
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
	/* Decremt recalculated value at start of the regulator. */
	expected_lightness -= SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up);
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

/* Test that ambient light level reset to zero after timeout. */
ZTEST(light_ctrl_pi_reg_test, test_amb_light_level_timeout)
{
	/* int_timeout = <number of interval before timeout> - 1.
	 * z_add_timeout() adds one more tick to regulator interval which results in additional
	 * 10ms because of CONFIG_SYS_CLOCK_TICKS_PER_SEC value.
	 */
	uint32_t int_timeout = CONFIG_BT_MESH_LIGHT_CTRL_AMB_LIGHT_LEVEL_TIMEOUT * MSEC_PER_SEC /
			       (CONFIG_BT_MESH_LIGHT_CTRL_REG_SPEC_INTERVAL + 10);
	uint16_t expected_lightness;

	/* Run the regulator and stop right before the ambient light level timer refresh. */
	start_reg(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		  REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);
	trigger_pi_reg(int_timeout, true);
	expected_lightness = SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * int_timeout +
			     light_ctrl_srv.reg->cfg.kp.up;
	/* Decremt recalculated value at start of the regulator. */
	expected_lightness -= SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up);
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* One more run to trigger the timer refresh. */
	trigger_pi_reg(1, true);

	/* Run regulator and stop right before the ambient light level timer refresh. */
	trigger_pi_reg(int_timeout - 1, false);
	expected_lightness += SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * int_timeout;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);
	zassert_not_equal(light_ctrl_srv.reg->measured, 0, "Ambient Light Level is reset");

	/* Run the regulator once again to reset the ambient light level. */
	trigger_pi_reg(1, false);
	expected_lightness += SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up);
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);
	zassert_equal(light_ctrl_srv.reg->measured, 0, "Ambient Light Level is not reset");

	/* Run the regulator and check that it drove the lightness value up. */
	trigger_pi_reg(1, false);
	float input = CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		      REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON);
	/* Subtract proportional part as it never added to the internal sum. */
	expected_lightness -= light_ctrl_srv.reg->cfg.kp.up;
	expected_lightness += SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up * input) +
			      input * light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);
}

/**
 * Test that the regulator correctly recalculates internal sum based on the provided lightness
 * value. At the first step, the regulator output should contain the provided lightness value plus
 * the proportional part.
 */
ZTEST(light_ctrl_pi_reg_test, test_internal_sum_recalculation)
{
	uint16_t expected_lightness;
	float i;

	start_reg(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
		  REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);
	trigger_pi_reg(1, true);
	/* Since the reported ambient lux level is lower than expected, the regulator output will
	 * be the current On state fsm output value + the proportional part.
	 */
	expected_lightness = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON];
	expected_lightness += light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Trigger  the regulator one more time to add integral part to the output */
	trigger_pi_reg(1, true);
	/* Since the reported ambient lux level is lower than expected, the regulator output will
	 * be the current On state fsm output value + one regulator step.
	 */
	expected_lightness += SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up);
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* When input is 0, the regulator should output the previous internal sum because other
	 * operands (U * T * Ki and U * Kp) evaluates to zero. Therefore, the internal sum should be
	 * the previous regulator output without proportional part.
	 */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON);
	trigger_pi_reg(1, true);
	expected_lightness -= light_ctrl_srv.reg->cfg.kp.up;
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Set the ambient lux level higher than the expected, which results in `input` eq to -1.
	 * The internal sum should decrease. This will result in the lower regulator output, and
	 * the fsm output will beat the regulator output.
	 */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON +
			   REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) + 1);
	trigger_pi_reg(1, true);
	i = expected_lightness - (SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.down) * 1);
	expected_lightness = light_ctrl_srv.cfg.light[LIGHT_CTRL_STATE_ON];
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);

	/* Set the ambient lux level lower than the expected, which results in `input` eq to 1.
	 * The internal sum should increment. Now the regulator output should be higher again than
	 * the fsm output.
	 */
	send_amb_lux_level(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON -
			   REG_ACCURACY(CONFIG_BT_MESH_LIGHT_CTRL_SRV_REG_LUX_ON) - 1);
	expected_lightness = i + (SUMMATION_STEP(light_ctrl_srv.reg->cfg.ki.up) * 1 +
			      light_ctrl_srv.reg->cfg.kp.up);
	trigger_pi_reg(1, true);
	zassert_equal(pi_reg_test_ctx.lightness, expected_lightness, "Expected: %d, got: %d",
		      expected_lightness, pi_reg_test_ctx.lightness);
}

ZTEST_SUITE(light_ctrl_test, NULL, NULL, setup, teardown, NULL);
ZTEST_SUITE(light_ctrl_pi_reg_test, NULL, NULL, setup_pi_reg, teardown_pi_reg, NULL);
