/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include <caf/events/sensor_event.h>

#include CONFIG_APP_SENSORS_DEF_PATH

#include "app_sensor_event.h"

#define MODULE app_sensors
#include <caf/events/module_state_event.h>
#include <caf/events/power_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_APP_SENSORS_LOG_LEVEL);

#define SAMPLE_THREAD_STACK_SIZE	CONFIG_APP_SENSORS_THREAD_STACK_SIZE
#define SAMPLE_THREAD_PRIORITY		CONFIG_APP_SENSORS_THREAD_PRIORITY

struct sensor_data {
	int64_t timeout;
	uint32_t period;
	uint32_t delay;
	atomic_t state;
	atomic_t event_cnt;
};

static struct sensor_data sensor_data[ARRAY_SIZE(sensor_configs)];

static K_THREAD_STACK_DEFINE(sample_thread_stack, SAMPLE_THREAD_STACK_SIZE);
static struct k_thread sample_thread;
static struct k_sem can_sample;

static struct sensor_trigger trig = {
	.type = SENSOR_TRIG_MOTION,
	.chan = SENSOR_CHAN_ACCEL_XYZ,
};

static int bmi270_configure(const struct device *dev)
{
	struct sensor_value full_scale, sampling_freq, oversampling;
	int ret = 0;

	/* Setting scale in G, due to loss of precision if the SI unit m/s^2
	 * is used
	 */
	full_scale.val1 = 2;		/* G */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;	/* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;		/* Normal mode */
	oversampling.val2 = 0;

	ret = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	if (ret != 0) {
		LOG_ERR("Failed to set %s attribute", dev->name);
		return ret;
	}

	ret = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	if (ret != 0) {
		LOG_ERR("Failed to set %s attribute", dev->name);
		return ret;
	}

	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change to 0.0Hz before changing
	 * other attributes
	 */
	ret = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);
	if (ret != 0) {
		LOG_ERR("Failed to set %s attribute", dev->name);
		return ret;
	}

	/* Setting scale in degrees/s to match the sensor scale */
	full_scale.val1 = 500;		/* dps */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;	/* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;		/* Normal mode */
	oversampling.val2 = 0;

	ret = sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	if (ret != 0) {
		LOG_ERR("Failed to set %s attribute", dev->name);
		return ret;
	}

	ret = sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	if (ret != 0) {
		LOG_ERR("Failed to set %s attribute", dev->name);
		return ret;
	}

	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change sampling frequency to
	 * 0.0Hz before changing other attributes
	 */
	ret = sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);
	if (ret != 0) {
		LOG_ERR("Failed to set %s attribute", dev->name);
		return ret;
	}

	return ret;
}

/**
 * @brief Motion trigger callback.
 *
 * Callback registered to imu sensor, that is called on motion detection.
 *
 * @param dev		Device.
 * @param trigger	Sensor trigger configuration.
 */
static void motion(const struct device *dev, const struct sensor_trigger *trigger)
{
	/* The procedure might be more complex, as sensor interrupt should be disabled. */
	int ret = sensor_trigger_set(dev, trigger, NULL);

	if (ret != 0) {
		LOG_ERR("Can not disable trigger, err: %d", ret);
	}

	APP_EVENT_SUBMIT(new_wake_up_event());
}

/**
 * @brief Notify about sensor state.
 *
 * Send sensor_state_event notifying about state change.
 *
 * @param[in] descr	Sensor descriptor.
 * @param[in] data	Data to be send with an event.
 * @param[in] data_cnt	Data cnt.
 * @param[in] event_cnt	Event cnt. Used to track active events.
 */
static void update_sensor_state(const struct sensor_config *sc, struct sensor_data *sd,
				const enum sensor_state state)
{
	if (atomic_get(&sd->state) != state) {
		struct sensor_state_event *event = new_sensor_state_event();

		event->descr = sc->event_descr;
		event->state = state;

		atomic_set(&sd->state, state);

		APP_EVENT_SUBMIT(event);
	}
}

/**
 * @brief Send sensor_event.
 *
 * Send sensor_event with measured data.
 *
 * @param[in] descr	Sensor descriptor.
 * @param[in] data	Data to be send with an event.
 * @param[in] data_cnt	Data cnt.
 * @param[in] event_cnt	Event cnt. Used to track active events.
 */
static void send_sensor_event(const char *descr, const struct sensor_value *data,
			      const size_t data_cnt, atomic_t *event_cnt)
{
	struct sensor_event *event = new_sensor_event(sizeof(struct sensor_value) * data_cnt);
	struct sensor_value *data_ptr = sensor_event_get_data_ptr(event);

	event->descr = descr;

	__ASSERT_NO_MSG(sensor_event_get_data_cnt(event) == data_cnt);
	memcpy(data_ptr, data, sizeof(struct sensor_value) * data_cnt);

	atomic_inc(event_cnt);
	APP_EVENT_SUBMIT(event);
}

/**
 * @brief Get the sensor data cnt value.
 *
 * @param[in] sc	Sensor config.
 * @return size_t	data_cnt for a sensor config.
 */
static size_t get_sensor_data_cnt(const struct sensor_config *sc)
{
	size_t data_cnt = 0;

	for (size_t i = 0; i < sc->chan_cnt; i++) {
		data_cnt += sc->chans[i].data_cnt;
	}

	return data_cnt;
}

/**
 * @brief Get the sensor data based on the sensor descriptor.
 *
 * @param[in] event_descr	Event descriptor.
 * @return struct sensor_data*	Pointer to the sensor_data.
 */
static struct sensor_data *get_sensor_data(const char *const event_descr)
{
	for (size_t i = 0; i < ARRAY_SIZE(sensor_configs); i++) {
		if (event_descr == sensor_configs[i].event_descr ||
		    !strcmp(event_descr, sensor_configs[i].event_descr)) {
			return &sensor_data[i];
		}
	}

	return NULL;
}

/**
 * @brief Get the sensor config based on the sensor descriptor.
 *
 * @param[in] event_descr		Event descriptor.
 * @return struct sensor_config*	Pointer to the sensor_data.
 */
static const struct sensor_config *get_sensor_config(const char *const event_descr)
{
	for (size_t i = 0; i < ARRAY_SIZE(sensor_configs); i++) {
		if (event_descr == sensor_configs[i].event_descr ||
		    !strcmp(event_descr, sensor_configs[i].event_descr)) {
			return &sensor_configs[i];
		}
	}

	return NULL;
}

/**
 * @brief Initialize sensors.
 *
 * @return size_t Number of correctly initialized sensors.
 */
static size_t sensors_init(void)
{
	size_t alive_sensors = 0;

	for (size_t i = 0; i < ARRAY_SIZE(sensor_configs); i++) {
		const struct sensor_config *sc = &sensor_configs[i];
		struct sensor_data *sd = &sensor_data[i];

		sd->period = UINT32_MAX;
		sd->timeout = INT64_MAX;

		if (!device_is_ready(sc->dev)) {
			update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
			LOG_ERR("%s sensor not ready", sc->dev->name);
			continue;
		}

		if (sc == get_sensor_config("imu")) {
			if (bmi270_configure(sc->dev) != 0) {
				update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
				LOG_ERR("%s sensor not ready", sc->dev->name);
				continue;
			}
		}

		if (sc == get_sensor_config("wu_imu")) {
			/* TODO: Trigger enable should be done in response to
			 * application event.
			 */
			int ret = sensor_trigger_set(sc->dev, &trig, motion);

			if (ret != 0 && ret != -ENOSYS) {
				LOG_ERR("Can not set trigger, err: %d", ret);
				update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
				continue;
			}
		}

		update_sensor_state(sc, sd, SENSOR_STATE_SLEEP);
		alive_sensors++;
	}

	return alive_sensors;
}

/**
 * @brief Sample sensor based on its configuration
 *
 * @param[in] sc Sensor configuration.
 * @param[in] sd Sensor data.
 */
static void sample_sensor(const struct sensor_config *sc, struct sensor_data *sd)
{
	size_t data_idx = 0;
	size_t data_cnt = get_sensor_data_cnt(sc);
	struct sensor_value data[data_cnt];

	int err = sensor_sample_fetch(sc->dev);

	for (size_t i = 0; !err && (i < sc->chan_cnt); i++) {
		const struct sampled_channel *sampled_chan = &sc->chans[i];

		err = sensor_channel_get(sc->dev, sampled_chan->chan, &data[data_idx]);
		data_idx += sampled_chan->data_cnt;
	}

	if (err) {
		LOG_ERR("Sensor sampling error (err %d)", err);
		update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
	} else {
		if (atomic_get(&sd->event_cnt) < sc->events_limit) {
			send_sensor_event(sc->event_descr, data, ARRAY_SIZE(data),
					  &sd->event_cnt);
		} else {
			LOG_WRN("Too many events, drop: %s",
				sc->dev->name);
		}
	}
}

/**
 * @brief Sample sensors handled by the sensor_sampler module. If the sensor
 *	  timeout expires, sample the sensor and send sensor_event with the
 *	  measured data.
 *
 * @param[inout] next_timeout	Minimal timeout when the next sampling should
 *				be performed. Calculated based on the
 *				configured sensors by the sensor_sampler
 *				module.
 */
static void sample_sensors(int64_t *next_timeout)
{
	int64_t uptime = k_uptime_get();

	*next_timeout = INT64_MAX;

	for (size_t i = 0; i < ARRAY_SIZE(sensor_data); i++) {
		const struct sensor_config *sc = &sensor_configs[i];
		struct sensor_data *sd = &sensor_data[i];

		/* Synchronize access to sensor_data. */
		k_sched_lock();
		if (atomic_get(&sd->state) == SENSOR_STATE_ACTIVE) {
			if (sd->timeout <= uptime) {
				sample_sensor(sc, sd);
			}

			int drops = 0;

			if ((sd->period > 0) && (sd->timeout <= uptime)) {
				int64_t diff = uptime - sd->timeout;

				drops = diff / sd->period;
				sd->timeout += sd->period - (diff % sd->period);
			}

			if (drops > 0) {
				LOG_WRN("%d sample dropped", drops);
			}
		}

		if (atomic_get(&sd->state) == SENSOR_STATE_ACTIVE) {
			if (*next_timeout > sd->timeout) {
				*next_timeout = sd->timeout;
			}
		}

		k_sched_unlock();
	}
}

/**
 * @brief Sampling thread function.
 */
static void sample_thread_fn(void)
{
	int64_t next_timeout = INT64_MAX;

	while (true) {
		k_sem_take(&can_sample, next_timeout == INT64_MAX ?
				K_FOREVER : K_TIMEOUT_ABS_MS(next_timeout));

		sample_sensors(&next_timeout);
	}
}

/**
 * @brief Initialize the module.
 */
static void init(void)
{
	size_t alive_sensors = sensors_init();

	if (!alive_sensors) {
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	k_sem_init(&can_sample, 0, 1);

	k_thread_create(&sample_thread, sample_thread_stack, SAMPLE_THREAD_STACK_SIZE,
			(k_thread_entry_t)sample_thread_fn, NULL, NULL, NULL,
			SAMPLE_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&sample_thread, "app_sensors");

	module_set_state(MODULE_STATE_READY);
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
	const struct sensor_config *sc = get_sensor_config("wu_imu");

	if (sc) {
		int ret = sensor_trigger_set(sc->dev, &trig, motion);

		if (ret != 0) {
			LOG_ERR("Can not set trigger, err: %d", ret);
		}
	}

	return false;
}

/**
 * @brief Process the sensor_start_event.
 *
 * @param[in] event	Sensor start event.
 * @return true		Consume the event.
 * @return false	Do not consume the event.
 */
static bool handle_sensor_start_event(const struct sensor_start_event *event)
{
	int64_t uptime = k_uptime_get();

	const struct sensor_config *sc = get_sensor_config(event->descr);
	struct sensor_data *sd = get_sensor_data(event->descr);

	if (sd && sc) {
		sd->period = event->period;
		sd->timeout = uptime + event->delay;

		update_sensor_state(sc, sd, SENSOR_STATE_ACTIVE);

		/* Let the sampling thread spin. */
		k_sem_give(&can_sample);
	}

	return false;
}

/**
 * @brief Process the sensor_stop_event.
 *
 * @param[in] event	Sensor stop event.
 * @return true		Consume the event.
 * @return false	Do not consume the event.
 */
static bool handle_sensor_stop_event(const struct sensor_stop_event *event)
{
	const struct sensor_config *sc = get_sensor_config(event->descr);
	struct sensor_data *sd = get_sensor_data(event->descr);

	if (sd && sc) {
		sd->period = UINT32_MAX;
		sd->timeout = INT64_MAX;

		update_sensor_state(sc, sd, SENSOR_STATE_SLEEP);
	}

	return false;
}

/**
 * @brief Process the sensor_event.
 *
 * The funciton is used to calculate active events in the system.
 *
 * @param[in] event	Sensor event.
 * @return true		Consume the event.
 * @return false	Do not consume the event.
 */
static bool handle_sensor_event(const struct sensor_event *event)
{
	struct sensor_data *sd = get_sensor_data(event->descr);

	if (sd == NULL) {
		return false;
	}

	atomic_dec(&sd->event_cnt);
	__ASSERT_NO_MSG(!(atomic_get(&sd->event_cnt) < 0));

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
	if (is_power_down_event(aeh)) {
		return handle_power_down_event(cast_power_down_event(aeh));
	}

	if (is_sensor_start_event(aeh)) {
		return handle_sensor_start_event(cast_sensor_start_event(aeh));
	}

	if (is_sensor_stop_event(aeh)) {
		return handle_sensor_stop_event(cast_sensor_stop_event(aeh));
	}

	if (is_sensor_event(aeh)) {
		return handle_sensor_event(cast_sensor_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
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
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE_FIRST(MODULE, sensor_start_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, sensor_stop_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, sensor_event);
