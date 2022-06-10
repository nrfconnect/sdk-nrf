/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <ei_wrapper.h>

#include <caf/events/sensor_event.h>
#include "ml_app_mode_event.h"
#include "ml_result_event.h"

#define MODULE ml_runner
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_ML_RUNNER_LOG_LEVEL);

#define SHIFT_WINDOWS		CONFIG_ML_APP_ML_RUNNER_WINDOW_SHIFT
#define SHIFT_FRAMES		CONFIG_ML_APP_ML_RUNNER_FRAME_SHIFT

#define APP_CONTROLS_ML_MODE	IS_ENABLED(CONFIG_ML_APP_MODE_EVENTS)

/* Make sure that event handlers will not be preempted by the EI wrapper's callback. */
BUILD_ASSERT(CONFIG_SYSTEM_WORKQUEUE_PRIORITY < CONFIG_EI_WRAPPER_THREAD_PRIORITY);

/**
 * @brief Enumeration of possible current module states
 */
enum state {
	/** Module is disabled.
	 *  The state is used only before module is initialized.
	 */
	STATE_DISABLED,
	/** Module is ready to work but no module is
	 *  listening so data would not be processed.
	 */
	STATE_READY,
	/** Module is currently processing the data. */
	STATE_ACTIVE,
	/** The module was suspended by user. */
	STATE_SUSPENDED,
	/** Error */
	STATE_ERROR
};

enum {
	ML_DROP_RESULT			= BIT(0),
	ML_CLEANUP_REQUIRED		= BIT(1),
	ML_FIRST_PREDICTION		= BIT(2),
	ML_RUNNING			= BIT(3),
};

BUILD_ASSERT(ARRAY_SIZE(CONFIG_ML_APP_ML_RUNNER_SENSOR_EVENT_DESCR) > 1);
static const char *handled_sensor_event_descr = CONFIG_ML_APP_ML_RUNNER_SENSOR_EVENT_DESCR;

static uint8_t ml_control;
static enum state state;
static struct module_flags active_listeners;


static void report_error(void)
{
	state = STATE_ERROR;
	module_set_state(MODULE_STATE_ERROR);
}

static enum state current_active_state(void)
{
	return module_flags_check_zero(&active_listeners) ? STATE_READY : STATE_ACTIVE;
}

static void submit_result(void)
{
	struct ml_result_event *evt = new_ml_result_event();

	int err = ei_wrapper_get_next_classification_result(&evt->label, &evt->value, NULL);

	if (!err) {
		err = ei_wrapper_get_anomaly(&evt->anomaly);
	}

	__ASSERT_NO_MSG(!err);
	ARG_UNUSED(err);

	APP_EVENT_SUBMIT(evt);
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

static void result_ready_cb(int err)
{
	k_sched_lock();

	bool drop_result = (err) || (ml_control & ML_DROP_RESULT) || (state == STATE_ERROR);

	if (err) {
		LOG_ERR("Result ready callback returned error (err: %d)", err);
		report_error();
	} else {
		ml_control &= ~ML_DROP_RESULT;
		ml_control &= ~ML_RUNNING;

		if (state == STATE_ACTIVE) {
			start_prediction();
		}
	}

	k_sched_unlock();

	if (!drop_result) {
		submit_result();
	}
}

static int init(void)
{
	ml_control |= ML_FIRST_PREDICTION;

	int err = ei_wrapper_init(result_ready_cb);

	if (err) {
		LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)", err);
	} else if (!APP_CONTROLS_ML_MODE) {
		start_prediction();
	}

	return err;
}

static bool handle_sensor_event(const struct sensor_event *event)
{
	if ((event->descr != handled_sensor_event_descr) &&
	    strcmp(event->descr, handled_sensor_event_descr)) {
		return false;
	}

	if (state != STATE_ACTIVE) {
		return false;
	}

	size_t data_cnt = sensor_event_get_data_cnt(event);
	float float_data[data_cnt];
	const struct sensor_value *data_ptr = sensor_event_get_data_ptr(event);

	for (size_t i = 0; i < data_cnt; i++) {
		float_data[i] = sensor_value_to_double(&data_ptr[i]);
	}

	int err = ei_wrapper_add_data(float_data, data_cnt);

	if (err) {
		LOG_ERR("Cannot add data for EI wrapper (err %d)", err);
		report_error();
		return false;
	}

	return false;
}

static bool handle_ml_app_mode_event(const struct ml_app_mode_event *event)
{
	if ((event->mode == ML_APP_MODE_MODEL_RUNNING) && (state == STATE_SUSPENDED)) {
		state = current_active_state();
		if (state == STATE_ACTIVE) {
			start_prediction();
		}
	} else if ((event->mode != ML_APP_MODE_MODEL_RUNNING) &&
		   ((state == STATE_ACTIVE) || (state == STATE_READY))) {
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
			state = APP_CONTROLS_ML_MODE ? STATE_SUSPENDED : current_active_state();
			module_set_state(MODULE_STATE_READY);
		} else {
			report_error();
		}
	}

	return false;
}

static bool handle_ml_result_signin_event(const struct ml_result_signin_event *event)
{
	__ASSERT_NO_MSG(module_check_id_valid(event->module_idx));
	module_flags_set_bit_to(&active_listeners, event->module_idx, event->state);

	if ((state == STATE_READY) || (state == STATE_ACTIVE)) {
		enum state new_state = current_active_state();

		if (state != new_state) {
			LOG_INF("State changed because of active liseners, new state: %s",
				new_state == STATE_READY ? "READY" : "ACTIVE");

			if (new_state == STATE_ACTIVE) {
				start_prediction();
			} else {
				(void)buf_cleanup();
			}
			state = new_state;
		}
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_event(aeh)) {
		return handle_sensor_event(cast_sensor_event(aeh));
	}

	if (APP_CONTROLS_ML_MODE &&
	    is_ml_app_mode_event(aeh)) {
		return handle_ml_app_mode_event(cast_ml_app_mode_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	if (is_ml_result_signin_event(aeh)) {
		return handle_ml_result_signin_event(cast_ml_result_signin_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_event);
APP_EVENT_SUBSCRIBE(MODULE, ml_result_signin_event);
#if APP_CONTROLS_ML_MODE
APP_EVENT_SUBSCRIBE(MODULE, ml_app_mode_event);
#endif /* APP_CONTROLS_ML_MODE */
