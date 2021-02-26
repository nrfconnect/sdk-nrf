/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/sensor.h>
#include <event_manager.h>

#if defined(CONFIG_EXTERNAL_SENSORS)
#include "ext_sensors.h"
#endif

#define MODULE sensor_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/sensor_module_event.h"
#include "events/util_module_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(sensor_module, CONFIG_SENSOR_MODULE_LOG_LEVEL);

struct sensor_msg_data {
	union {
		struct app_module_event app;
		struct data_module_event data;
		struct util_module_event util;
	} module;
};

/* Sensor module super states. */
static enum state_type {
	STATE_INIT,
	STATE_RUNNING,
	STATE_SHUTDOWN
} state;

/* Sensor module message queue. */
#define SENSOR_QUEUE_ENTRY_COUNT	10
#define SENSOR_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_sensor, sizeof(struct sensor_msg_data),
	      SENSOR_QUEUE_ENTRY_COUNT, SENSOR_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "sensor",
	.msg_q = &msgq_sensor,
};

/* Forward declarations. */
#if defined(CONFIG_EXTERNAL_SENSORS)
static void movement_data_send(const struct ext_sensor_evt *const acc_data);
#endif

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type new_state)
{
	switch (new_state) {
	case STATE_INIT:
		return "STATE_INIT";
	case STATE_RUNNING:
		return "STATE_RUNNING";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
	default:
		return "Unknown";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

/* Handlers */
static bool event_handler(const struct event_header *eh)
{
	struct sensor_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_app_module_event(eh)) {
		struct app_module_event *event = cast_app_module_event(eh);

		msg.module.app = *event;
		enqueue_msg = true;
	}

	if (is_data_module_event(eh)) {
		struct data_module_event *event = cast_data_module_event(eh);

		msg.module.data = *event;
		enqueue_msg = true;
	}

	if (is_util_module_event(eh)) {
		struct util_module_event *event = cast_util_module_event(eh);

		msg.module.util = *event;
		enqueue_msg = true;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(sensor, SENSOR_EVT_ERROR, err);
		}
	}

	return false;
}

#if defined(CONFIG_EXTERNAL_SENSORS)
static void ext_sensor_handler(const struct ext_sensor_evt *const evt)
{
	switch (evt->type) {
	case EXT_SENSOR_EVT_ACCELEROMETER_TRIGGER:
		movement_data_send(evt);
		break;
	default:
		break;
	}
}
#endif

/* Static module functions. */
#if defined(CONFIG_EXTERNAL_SENSORS)
static void movement_data_send(const struct ext_sensor_evt *const acc_data)
{
	struct sensor_module_event *sensor_module_event =
			new_sensor_module_event();

	sensor_module_event->data.accel.values[0] = acc_data->value_array[0];
	sensor_module_event->data.accel.values[1] = acc_data->value_array[1];
	sensor_module_event->data.accel.values[2] = acc_data->value_array[2];
	sensor_module_event->data.accel.timestamp = k_uptime_get();
	sensor_module_event->type = SENSOR_EVT_MOVEMENT_DATA_READY;

	EVENT_SUBMIT(sensor_module_event);
}
#endif

static int environmental_data_get(void)
{
	struct sensor_module_event *sensor_module_event;
#if defined(CONFIG_EXTERNAL_SENSORS)
	int err;
	double temp, hum;

	/* Request data from external sensors. */
	err = ext_sensors_temperature_get(&temp);
	if (err) {
		LOG_ERR("temperature_get, error: %d", err);
		return err;
	}

	err = ext_sensors_humidity_get(&hum);
	if (err) {
		LOG_ERR("temperature_get, error: %d", err);
		return err;
	}

	sensor_module_event = new_sensor_module_event();
	sensor_module_event->data.sensors.timestamp = k_uptime_get();
	sensor_module_event->data.sensors.temperature = temp;
	sensor_module_event->data.sensors.humidity = hum;
	sensor_module_event->type = SENSOR_EVT_ENVIRONMENTAL_DATA_READY;
#else

	/* This event must be sent even though environmental sensors are not
	 * available on the nRF9160DK. This is because the Data module expects
	 * responses from the different modules within a certain amounf of time
	 * after the APP_EVT_DATA_GET event has been emitted.
	 */
	LOG_DBG("No external sensors, submitting dummy sensor data");

	/* Set this entry to false signifying that the event carries no data.
	 * This makes sure that the entry is not stored in the circular buffer.
	 */
	sensor_module_event = new_sensor_module_event();
	sensor_module_event->type = SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED;
#endif
	EVENT_SUBMIT(sensor_module_event);

	return 0;
}

static int setup(void)
{
#if defined(CONFIG_EXTERNAL_SENSORS)
	int err;

	err = ext_sensors_init(ext_sensor_handler);
	if (err) {
		LOG_ERR("ext_sensors_init, error: %d", err);
		return err;
	}
#endif
	return 0;
}

static bool environmental_data_requested(enum app_module_data_type *data_list,
					 size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (data_list[i] == APP_DATA_ENVIRONMENTAL) {
			return true;
		}
	}

	return false;
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct sensor_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) {
#if defined(CONFIG_EXTERNAL_SENSORS)
		int err;
		double accelerometer_threshold =
			msg->module.data.data.cfg.accelerometer_threshold;

		err = ext_sensors_mov_thres_set(accelerometer_threshold);
		if (err == -ENOTSUP) {
			LOG_WRN("Passed in threshold value not valid");
		} else if (err) {
			LOG_ERR("Failed to set threshold, error: %d", err);
			SEND_ERROR(sensor, SENSOR_EVT_ERROR, err);
		}
#endif
		state_set(STATE_RUNNING);
	}
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct sensor_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {

#if defined(CONFIG_EXTERNAL_SENSORS)
		int err;
		double accelerometer_threshold =
			msg->module.data.data.cfg.accelerometer_threshold;

		err = ext_sensors_mov_thres_set(accelerometer_threshold);
		if (err == -ENOTSUP) {
			LOG_WRN("Passed in threshold value not valid");
		} else if (err) {
			LOG_ERR("Failed to set threshold, error: %d", err);
			SEND_ERROR(sensor, SENSOR_EVT_ERROR, err);
		}
#endif
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (!environmental_data_requested(
			msg->module.app.data_list,
			msg->module.app.count)) {
			return;
		}

		int err;

		err = environmental_data_get();
		if (err) {
			LOG_ERR("environmental_data_get, error: %d", err);
			SEND_ERROR(sensor, SENSOR_EVT_ERROR, err);
		}
	}
}

/* Message handler for all states. */
static void on_all_states(struct sensor_msg_data *msg)
{
	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		/* The module doesn't have anything to shut down and can
		 * report back immediately.
		 */
		SEND_EVENT(sensor, SENSOR_EVT_SHUTDOWN_READY);
		state_set(STATE_SHUTDOWN);
	}
}

static void module_thread_fn(void)
{
	int err;
	struct sensor_msg_data msg;

	self.thread_id = k_current_get();

	module_start(&self);
	state_set(STATE_INIT);

	err = setup();
	if (err) {
		LOG_ERR("setup, error: %d", err);
		SEND_ERROR(sensor, SENSOR_EVT_ERROR, err);
	}

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_INIT:
			on_state_init(&msg);
			break;
		case STATE_RUNNING:
			on_state_running(&msg);
			break;
		case STATE_SHUTDOWN:
			/* The shutdown state has no transition. */
			break;
		default:
			LOG_WRN("Unknown sensor module state.");
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(sensor_module_thread, CONFIG_SENSOR_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, app_module_event);
EVENT_SUBSCRIBE(MODULE, data_module_event);
EVENT_SUBSCRIBE(MODULE, util_module_event);
