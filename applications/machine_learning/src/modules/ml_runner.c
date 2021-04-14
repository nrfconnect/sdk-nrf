/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <ei_wrapper.h>

#include <caf/events/sensor_event.h>
#include "ml_state_event.h"
#include "ml_result_event.h"

#define MODULE ml_runner
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_ML_RUNNER_LOG_LEVEL);

#define SHIFT_WINDOWS		CONFIG_ML_APP_ML_RUNNER_WINDOW_SHIFT
#define SHIFT_FRAMES		CONFIG_ML_APP_ML_RUNNER_FRAME_SHIFT

#define APP_CONTROLS_ML_STATE	IS_ENABLED(CONFIG_ML_APP_ML_STATE_EVENTS)

enum state {
	STATE_DISABLED,
	STATE_ACTIVE,
	STATE_SUSPENDED,
	STATE_ERROR
};

enum {
	ML_DROP_RESULT			= BIT(0),
	ML_CLEANUP_REQUIRED		= BIT(1),
	ML_FIRST_PREDICTION		= BIT(2),
	ML_RUNNING			= BIT(3),
};

static uint8_t ml_control;
static enum state state;


static void result_ready_cb(int err)
{
	if (err) {
		LOG_ERR("Result ready callback returned error (err: %d)", err);
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	struct ml_result_event *evt = new_ml_result_event();

	err = ei_wrapper_get_classification_results(&evt->label, &evt->value, &evt->anomaly);

	if (err) {
		LOG_ERR("Cannot get classification results (err: %d)", err);
		module_set_state(MODULE_STATE_ERROR);
		k_free(evt);
	} else {
		EVENT_SUBMIT(evt);
	}
}

static void report_error(void)
{
	state = STATE_ERROR;
	module_set_state(MODULE_STATE_ERROR);
}

static int buf_cleanup(void)
{
	bool cancelled = false;
	int err = ei_wrapper_clear_data(&cancelled);

	if (!err) {
		if (cancelled) {
			ml_control &= ~ML_RUNNING;
		}

		if (ml_control & ML_RUNNING) {
			ml_control |= ML_DROP_RESULT;
		}

		ml_control &= ~ML_CLEANUP_REQUIRED;
		ml_control |= ML_FIRST_PREDICTION;
	} else if (err == -EBUSY) {
		__ASSERT_NO_MSG(ml_control & ML_RUNNING);
		ml_control |= ML_DROP_RESULT;
		ml_control |= ML_CLEANUP_REQUIRED;
	} else {
		LOG_ERR("Cannot cleanup buffer (err: %d)", err);
		report_error();
	}

	return err;
}

static void start_prediction(void)
{
	int err;
	size_t window_shift;
	size_t frame_shift;

	if (ml_control & ML_RUNNING) {
		return;
	}

	if (ml_control & ML_CLEANUP_REQUIRED) {
		err = buf_cleanup();
		if (err) {
			return;
		}
	}

	if (ml_control & ML_FIRST_PREDICTION) {
		window_shift = 0;
		frame_shift = 0;
	} else {
		window_shift = SHIFT_WINDOWS;
		frame_shift = SHIFT_FRAMES;
	}

	err = ei_wrapper_start_prediction(window_shift, frame_shift);

	if (!err) {
		ml_control |= ML_RUNNING;
		ml_control &= ~ML_FIRST_PREDICTION;
	} else {
		LOG_ERR("Cannot start prediction (err: %d)", err);
		report_error();
	}
}

static int init(void)
{
	ml_control |= ML_FIRST_PREDICTION;

	int err = ei_wrapper_init(result_ready_cb);

	if (err) {
		LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)", err);
	} else if (!APP_CONTROLS_ML_STATE) {
		start_prediction();
	}

	return err;
}

static bool handle_sensor_event(const struct sensor_event *event)
{
	if (state != STATE_ACTIVE) {
		return false;
	}

	int err = ei_wrapper_add_data(sensor_event_get_data_ptr(event),
				      sensor_event_get_data_cnt(event));

	if (err) {
		LOG_ERR("Cannot add data for EI wrapper (err %d)", err);
		report_error();
	}

	return false;
}

static bool handle_ml_result_event(const struct ml_result_event *event)
{
	bool consume = ((ml_control & ML_DROP_RESULT) || (state == STATE_ERROR));

	ml_control &= ~ML_DROP_RESULT;
	ml_control &= ~ML_RUNNING;

	if (state == STATE_ACTIVE) {
		start_prediction();
	}

	return consume;
}

static bool handle_ml_state_event(const struct ml_state_event *event)
{
	if ((event->state == ML_STATE_MODEL_RUNNING) && (state == STATE_SUSPENDED)) {
		start_prediction();
		state = STATE_ACTIVE;
	} else if ((event->state != ML_STATE_MODEL_RUNNING) && (state == STATE_ACTIVE)) {
		int err = buf_cleanup();

		if (!err || (err == -EBUSY)) {
			state = STATE_SUSPENDED;

		}
	}

	return false;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		__ASSERT_NO_MSG(state == STATE_DISABLED);

		if (!init()) {
			state = APP_CONTROLS_ML_STATE ? STATE_SUSPENDED : STATE_ACTIVE;
			module_set_state(MODULE_STATE_READY);
		} else {
			report_error();
		}
	} else if (check_state(event, MODULE_ID(MODULE), MODULE_STATE_ERROR)) {
		state = STATE_ERROR;
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_sensor_event(eh)) {
		return handle_sensor_event(cast_sensor_event(eh));
	}

	if (is_ml_result_event(eh)) {
		return handle_ml_result_event(cast_ml_result_event(eh));
	}

	if (APP_CONTROLS_ML_STATE &&
	    is_ml_state_event(eh)) {
		return handle_ml_state_event(cast_ml_state_event(eh));
	}

	if (is_module_state_event(eh)) {
		return handle_module_state_event(cast_module_state_event(eh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, sensor_event);
EVENT_SUBSCRIBE_EARLY(MODULE, ml_result_event);
#if APP_CONTROLS_ML_STATE
EVENT_SUBSCRIBE(MODULE, ml_state_event);
#endif /* APP_CONTROLS_ML_STATE */
