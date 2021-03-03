/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <ei_wrapper.h>

#include "sensor_event.h"
#include "ml_result_event.h"

#define MODULE ml_runner
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_ML_RUNNER_LOG_LEVEL);

#define SHIFT_WINDOWS		CONFIG_ML_APP_ML_RUNNER_WINDOW_SHIFT
#define SHIFT_FRAMES		CONFIG_ML_APP_ML_RUNNER_FRAME_SHIFT

enum state {
	STATE_DISABLED,
	STATE_ACTIVE,
	STATE_ERROR
};

static enum state state;


static void report_error(void)
{
	state = STATE_ERROR;
	module_set_state(MODULE_STATE_ERROR);
}

static void result_ready_cb(int err)
{
	if (err) {
		LOG_ERR("Result ready callback returned error (err: %d)", err);
		report_error();
		return;
	}

	struct ml_result_event *evt = new_ml_result_event();

	err = ei_wrapper_get_classification_results(&evt->label, &evt->value, &evt->anomaly);

	if (err) {
		LOG_ERR("Cannot get classification results (err: %d)", err);
		k_free(evt);
		report_error();
	} else {
		EVENT_SUBMIT(evt);

		err = ei_wrapper_start_prediction(SHIFT_WINDOWS, SHIFT_FRAMES);

		if (err) {
			LOG_ERR("Cannot restart prediction (err: %d)", err);
			report_error();
		}
	}
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

static int init(void)
{
	int err = ei_wrapper_init(result_ready_cb);

	if (err) {
		LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)", err);
	} else {
		err = ei_wrapper_start_prediction(0, 0);

		if (err) {
			LOG_ERR("Cannot start prediction (err: %d)", err);
		}
	}

	return err;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_sensor_event(eh)) {
		return handle_sensor_event(cast_sensor_event(eh));
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
EVENT_SUBSCRIBE(MODULE, sensor_event);
