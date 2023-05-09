/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/sensor.h>
#include <app_event_manager.h>

#if defined(CONFIG_EXTERNAL_SENSORS)
#include "ext_sensors.h"
#endif

#if defined(CONFIG_ADP536X)
#include <adp536x.h>
#endif

#define MODULE sensor_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/sensor_module_event.h"
#include "events/util_module_event.h"

#include <zephyr/logging/log.h>
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
	.supports_shutdown = true,
};

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
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct sensor_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_app_module_event(aeh)) {
		struct app_module_event *event = cast_app_module_event(aeh);

		msg.module.app = *event;
		enqueue_msg = true;
	}

	if (is_data_module_event(aeh)) {
		struct data_module_event *event = cast_data_module_event(aeh);

		msg.module.data = *event;
		enqueue_msg = true;
	}

	if (is_util_module_event(aeh)) {
		struct util_module_event *event = cast_util_module_event(aeh);

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

/* Static module functions. */

#if defined(CONFIG_EXTERNAL_SENSORS)
/* Function that enables or disables trigger callbacks from the accelerometer. */
static void accelerometer_callback_set(bool enable)
{
	int err;

	err = ext_sensors_accelerometer_trigger_callback_set(enable);
	if (err) {
		LOG_ERR("ext_sensors_accelerometer_trigger_callback_set, error: %d", err);
	}
}

static void activity_data_send(const struct ext_sensor_evt *const acc_data)
{
	struct sensor_module_event *sensor_module_event = new_sensor_module_event();

	__ASSERT(sensor_module_event, "Not enough heap left to allocate event");

	if (acc_data->type == EXT_SENSOR_EVT_ACCELEROMETER_ACT_TRIGGER)	{
		sensor_module_event->type = SENSOR_EVT_MOVEMENT_ACTIVITY_DETECTED;
	} else {
		__ASSERT_NO_MSG(acc_data->type == EXT_SENSOR_EVT_ACCELEROMETER_INACT_TRIGGER);
		sensor_module_event->type = SENSOR_EVT_MOVEMENT_INACTIVITY_DETECTED;
	}
	APP_EVENT_SUBMIT(sensor_module_event);
}

static void impact_data_send(const struct ext_sensor_evt *const evt)
{
	struct sensor_module_event *sensor_module_event = new_sensor_module_event();

	__ASSERT(sensor_module_event, "Not enough heap left to allocate event");

	sensor_module_event->data.impact.magnitude = evt->value;
	sensor_module_event->data.impact.timestamp = k_uptime_get();
	sensor_module_event->type = SENSOR_EVT_MOVEMENT_IMPACT_DETECTED;

	APP_EVENT_SUBMIT(sensor_module_event);
}

static void ext_sensor_handler(const struct ext_sensor_evt *const evt)
{
	switch (evt->type) {
	case EXT_SENSOR_EVT_ACCELEROMETER_ACT_TRIGGER:
		activity_data_send(evt);
		break;
	case EXT_SENSOR_EVT_ACCELEROMETER_INACT_TRIGGER:
		activity_data_send(evt);
		break;
	case EXT_SENSOR_EVT_ACCELEROMETER_IMPACT_TRIGGER:
		impact_data_send(evt);
		break;
	case EXT_SENSOR_EVT_ACCELEROMETER_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_ACCELEROMETER_ERROR");
		break;
	case EXT_SENSOR_EVT_TEMPERATURE_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_TEMPERATURE_ERROR");
		break;
	case EXT_SENSOR_EVT_HUMIDITY_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_HUMIDITY_ERROR");
		break;
	case EXT_SENSOR_EVT_PRESSURE_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_PRESSURE_ERROR");
		break;
	case EXT_SENSOR_EVT_AIR_QUALITY_ERROR:
		LOG_ERR("EXT_SENSOR_EVT_AIR_QUALITY_ERROR");
		break;
	default:
		break;
	}
}
#endif /* CONFIG_EXTERNAL_SENSORS */

#if defined(CONFIG_EXTERNAL_SENSORS)
static void configure_acc(const struct cloud_data_cfg *cfg)
{
		int err;
		double accelerometer_activity_threshold =
			cfg->accelerometer_activity_threshold;
		double accelerometer_inactivity_threshold =
			cfg->accelerometer_inactivity_threshold;
		double accelerometer_inactivity_timeout =
			cfg->accelerometer_inactivity_timeout;

		err = ext_sensors_accelerometer_threshold_set(accelerometer_activity_threshold,
							      true);
		if (err == -ENOTSUP) {
			LOG_WRN("The requested act threshold value not valid");
		} else if (err) {
			LOG_ERR("Failed to set act threshold, error: %d", err);
		}
		err = ext_sensors_accelerometer_threshold_set(accelerometer_inactivity_threshold,
							      false);
		if (err == -ENOTSUP) {
			LOG_WRN("The requested inact threshold value not valid");
		} else if (err) {
			LOG_ERR("Failed to set inact threshold, error: %d", err);
		}
		err = ext_sensors_inactivity_timeout_set(accelerometer_inactivity_timeout);
		if (err == -ENOTSUP) {
			LOG_WRN("The requested timeout value not valid");
		} else if (err) {
			LOG_ERR("Failed to set timeout, error: %d", err);
		}
}
#endif

static void apply_config(struct sensor_msg_data *msg)
{
#if defined(CONFIG_EXTERNAL_SENSORS)
	configure_acc(&msg->module.data.data.cfg);

	if (msg->module.data.data.cfg.active_mode) {
		accelerometer_callback_set(false);
	} else {
		accelerometer_callback_set(true);
	}
#endif /* CONFIG_EXTERNAL_SENSORS */
}

static void battery_data_get(void)
{
	struct sensor_module_event *sensor_module_event;

#if defined(CONFIG_ADP536X)
	int err;
	uint8_t percentage;

	err = adp536x_fg_soc(&percentage);
	if (err) {
		LOG_ERR("Failed to get battery level: %d", err);
		return;
	}

	sensor_module_event = new_sensor_module_event();

	__ASSERT(sensor_module_event, "Not enough heap left to allocate event");

	sensor_module_event->data.bat.timestamp = k_uptime_get();
	sensor_module_event->data.bat.battery_level = percentage;
	sensor_module_event->type = SENSOR_EVT_FUEL_GAUGE_READY;
#else
	sensor_module_event = new_sensor_module_event();

	__ASSERT(sensor_module_event, "Not enough heap left to allocate event");

	sensor_module_event->type = SENSOR_EVT_FUEL_GAUGE_NOT_SUPPORTED;
#endif
	APP_EVENT_SUBMIT(sensor_module_event);
}

static void environmental_data_get(void)
{
	struct sensor_module_event *sensor_module_event;
#if defined(CONFIG_EXTERNAL_SENSORS)
	int err;
	double temperature = 0, humidity = 0, pressure = 0;
	uint16_t bsec_air_quality = UINT16_MAX;

	/* Request data from external sensors. */
	err = ext_sensors_temperature_get(&temperature);
	if (err) {
		LOG_ERR("ext_sensors_temperature_get, error: %d", err);
	}

	err = ext_sensors_humidity_get(&humidity);
	if (err) {
		LOG_ERR("ext_sensors_humidity_get, error: %d", err);
	}

	err = ext_sensors_pressure_get(&pressure);
	if (err) {
		LOG_ERR("ext_sensors_pressure_get, error: %d", err);
	}

	err = ext_sensors_air_quality_get(&bsec_air_quality);
	if (err && err == -ENOTSUP) {
		/* Air quality is not available, enable the Bosch BSEC library driver.
		 * Propagate the air quality value as -1.
		 */
	} else if (err) {
		LOG_ERR("ext_sensors_bsec_air_quality_get, error: %d", err);
	}

	sensor_module_event = new_sensor_module_event();

	__ASSERT(sensor_module_event, "Not enough heap left to allocate event");

	sensor_module_event->data.sensors.timestamp = k_uptime_get();
	sensor_module_event->data.sensors.temperature = temperature;
	sensor_module_event->data.sensors.humidity = humidity;
	sensor_module_event->data.sensors.pressure = pressure;
	sensor_module_event->data.sensors.bsec_air_quality =
					(bsec_air_quality == UINT16_MAX) ? -1 : bsec_air_quality;
	sensor_module_event->type = SENSOR_EVT_ENVIRONMENTAL_DATA_READY;
#else

	/* This event must be sent even though environmental sensors are not
	 * available on the nRF9160DK. This is because the Data module expects
	 * responses from the different modules within a certain amount of time
	 * after the APP_EVT_DATA_GET event has been emitted.
	 */
	LOG_DBG("No external sensors, submitting dummy sensor data");

	/* Set this entry to false signifying that the event carries no data.
	 * This makes sure that the entry is not stored in the circular buffer.
	 */
	sensor_module_event = new_sensor_module_event();

	__ASSERT(sensor_module_event, "Not enough heap left to allocate event");

	sensor_module_event->type = SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED;
#endif
	APP_EVENT_SUBMIT(sensor_module_event);
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

static bool battery_data_requested(enum app_module_data_type *data_list,
					 size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (data_list[i] == APP_DATA_BATTERY) {
			return true;
		}
	}

	return false;
}

/* Message handler for STATE_INIT. */
static void on_state_init(struct sensor_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_INIT)) {
		apply_config(msg);
		state_set(STATE_RUNNING);
	}
}

/* Message handler for STATE_RUNNING. */
static void on_state_running(struct sensor_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_READY)) {
		apply_config(msg);
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		if (environmental_data_requested(msg->module.app.data_list,
						 msg->module.app.count)) {
			environmental_data_get();
		}

		if (battery_data_requested(msg->module.app.data_list, msg->module.app.count)) {
			battery_data_get();
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
		SEND_SHUTDOWN_ACK(sensor, SENSOR_EVT_SHUTDOWN_READY, self.id);
		state_set(STATE_SHUTDOWN);
	}
}

static void module_thread_fn(void)
{
	int err;
	struct sensor_msg_data msg = { 0 };

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(sensor, SENSOR_EVT_ERROR, err);
	}

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
			LOG_ERR("Unknown state.");
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(sensor_module_thread, CONFIG_SENSOR_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, data_module_event);
APP_EVENT_SUBSCRIBE(MODULE, util_module_event);
