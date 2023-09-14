/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <caf/events/sensor_event.h>
#include <caf/events/power_event.h>

#include "app_sensor_event.h"

#define MODULE app_control
#include <caf/events/module_state_event.h>

#define BURST_INTERVAL		CONFIG_APP_CONTROL_BURST_INTERVAL_S
#define BURST_MEAS_CNT		CONFIG_APP_CONTROL_BURST_MEAS_CNT
#define BURST_MEAS_INTERVAL	CONFIG_APP_CONTROL_BURST_MEAS_INTERVAL_MS

static size_t meas_cnt;
static struct k_work_delayable burst_trigger;


static void burst_trigger_fn(struct k_work *w)
{
	APP_EVENT_SUBMIT(new_wake_up_event());

	k_work_reschedule(&burst_trigger, K_SECONDS(BURST_INTERVAL));
}

static void init(void)
{
	k_work_init_delayable(&burst_trigger, burst_trigger_fn);
	k_work_schedule(&burst_trigger, K_NO_WAIT);
	module_set_state(MODULE_STATE_READY);
};

/**
 * @brief Process the wake_up_event.
 *
 * @param[in] event	Wake up event.
 * @return true		Consume the event.
 * @return false	Do not consume the event.
 */
static bool handle_wake_up_event(const struct wake_up_event *event)
{
	/* Cancel cannot fail if executed from another work's context. */
	(void)k_work_cancel_delayable(&burst_trigger);
	meas_cnt = 0;

	struct sensor_start_event *start_env = new_sensor_start_event();

	start_env->descr = "env";
	start_env->delay = 0;
	start_env->period = BURST_MEAS_INTERVAL;

	struct sensor_start_event *start_bmi = new_sensor_start_event();

	start_bmi->descr = "imu";
	start_bmi->delay = 0;
	start_bmi->period = BURST_MEAS_INTERVAL;

	APP_EVENT_SUBMIT(start_bmi);
	APP_EVENT_SUBMIT(start_env);

	return false;
}

/**
 * @brief Process the power_down_event.
 *
 * @param[in] event	Power down event.
 * @return true		Consume the event.
 * @return false	Do not consume the event.
 */
static bool handle_power_down_event(const struct power_down_event *event)
{
	k_work_reschedule(&burst_trigger, K_SECONDS(BURST_INTERVAL));

	struct sensor_stop_event *stop_env = new_sensor_stop_event();
	struct sensor_stop_event *stop_imu = new_sensor_stop_event();

	stop_env->descr = "env";
	stop_imu->descr = "imu";

	APP_EVENT_SUBMIT(stop_env);
	APP_EVENT_SUBMIT(stop_imu);

	return false;
}

/**
 * @brief Process the sensor_event.
 *
 * @param event		Sensor event.
 * @return true		Consume the event.
 * @return false	Do not consume the event.
 */
static bool handle_sensor_event(const struct sensor_event *event)
{
	if (!strcmp(event->descr, "env")) {
		if (++meas_cnt == BURST_MEAS_CNT) {
			meas_cnt = 0;

			struct power_down_event *evt = new_power_down_event();

			evt->error = false;
			APP_EVENT_SUBMIT(evt);
		}
	}

	return false;
}

/**
 * @brief Handler for application event manager events.
 *
 * @param[in] aeh	Application event header.
 * @return true		Consume the event.
 * @return false	Do not consume the event.
 */
static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_wake_up_event(aeh)) {
		return handle_wake_up_event(cast_wake_up_event(aeh));
	}

	if (is_power_down_event(aeh)) {
		return handle_power_down_event(cast_power_down_event(aeh));
	}

	if (is_sensor_event(aeh)) {
		return handle_sensor_event(cast_sensor_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(app_sensors), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			init();
			initialized = true;
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_event);
