/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <drivers/sensor_sim.h>

#include "sensor_sim_ctrl_def.h"

#include <caf/events/button_event.h>

#define MODULE sensor_sim_ctrl
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_SENSOR_SIM_CTRL_LOG_LEVEL);

#if CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_TIMEOUT
	BUILD_ASSERT(CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_TIMEOUT_MS > 0);
	#define WAVE_SWAP_PERIOD	K_MSEC(CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_TIMEOUT_MS)
#else
	#define WAVE_SWAP_PERIOD	K_MSEC(0)
#endif /* CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_TIMEOUT */

#if CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_BUTTON
	#define WAVE_SWAP_BUTTON_ID	CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_BUTTON_ID
#else
	#define WAVE_SWAP_BUTTON_ID	0xffff
#endif /* CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_BUTTON */

enum state {
	STATE_DISABLED,
	STATE_ACTIVE,
	STATE_ERROR
};

static enum state state;
static uint8_t cur_wave_idx;
static struct k_delayed_work change_wave;


static void report_error(void)
{
	state = STATE_ERROR;
	module_set_state(MODULE_STATE_ERROR);
}

static void set_wave(void)
{
	int err = sensor_sim_set_wave_param(sim_signal_params.chan,
					    &sim_signal_params.waves[cur_wave_idx]);

	if (err) {
		LOG_ERR("Cannot set simulated accel params (err %d)", err);
		report_error();
	}
}

static void select_next_wave(void)
{
	if (state != STATE_ACTIVE) {
		return;
	}

	cur_wave_idx++;
	if (cur_wave_idx >= sim_signal_params.waves_cnt) {
		cur_wave_idx = 0;
	}

	set_wave();
}

static void change_wave_fn(struct k_work *w)
{
	select_next_wave();

	k_delayed_work_submit(&change_wave, WAVE_SWAP_PERIOD);
}

static bool handle_button_event(const struct button_event *event)
{
	if ((event->key_id == WAVE_SWAP_BUTTON_ID) && event->pressed) {
		select_next_wave();
	}

	return false;
}

static void verify_wave_params(void)
{
	for (size_t i = 0; i < sim_signal_params.waves_cnt; i++) {
		const struct wave_gen_param *w = &sim_signal_params.waves[i];

		__ASSERT_NO_MSG(w->type < WAVE_GEN_TYPE_COUNT);
		__ASSERT_NO_MSG((w->period_ms > 0) || (w->type == WAVE_GEN_TYPE_NONE));
	}
}

static int init(void)
{
	if (IS_ENABLED(CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_TIMEOUT)) {
		k_delayed_work_init(&change_wave, change_wave_fn);
		k_delayed_work_submit(&change_wave, WAVE_SWAP_PERIOD);
	}

	if (IS_ENABLED(CONFIG_ASSERT)) {
		verify_wave_params();
	}

	set_wave();

	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	if (IS_ENABLED(CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_BUTTON) &&
	    is_button_event(eh)) {
		return handle_button_event(cast_button_event(eh));
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);

			if (!init()) {
				module_set_state(MODULE_STATE_READY);
				state = STATE_ACTIVE;
			} else {
				report_error();
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_BUTTON
EVENT_SUBSCRIBE(MODULE, button_event);
#endif /* CONFIG_ML_APP_SENSOR_SIM_CTRL_TRIG_BUTTON */
