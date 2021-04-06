/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <drivers/sensor.h>

#include <caf/events/sensor_event.h>

#include CONFIG_CAF_SENSOR_SAMPLER_DEF_PATH

#define MODULE sensor_sampler
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_SENSOR_SAMPLER_LOG_LEVEL);

#define SAMPLE_THREAD_STACK_SIZE	CONFIG_CAF_SENSOR_SAMPLER_THREAD_STACK_SIZE
#define SAMPLE_THREAD_PRIORITY		CONFIG_CAF_SENSOR_SAMPLER_THREAD_PRIORITY

struct sensor_data {
	const struct device *dev;
	int sampling_period;
	int64_t sample_timeout;
};

enum state {
	STATE_DISABLED,
	STATE_ACTIVE,
	STATE_ERROR
};

static struct sensor_data sensor_data[ARRAY_SIZE(sensor_configs)];

static enum state state;

static K_THREAD_STACK_DEFINE(sample_thread_stack, SAMPLE_THREAD_STACK_SIZE);
static struct k_thread sample_thread;


static void report_error(void)
{
	state = STATE_ERROR;
	module_set_state(MODULE_STATE_ERROR);
}

static void send_sensor_event(const char *descr, struct sensor_value *data, size_t data_cnt)
{
	struct sensor_event *event = new_sensor_event(sizeof(float) * data_cnt);
	float *data_ptr = sensor_event_get_data_ptr(event);

	event->descr = descr;

	__ASSERT_NO_MSG(sensor_event_get_data_cnt(event) == data_cnt);
	for (size_t i = 0; i < data_cnt; i++) {
		data_ptr[i] = sensor_value_to_double(&data[i]);
	}

	EVENT_SUBMIT(event);
}

static int sample_sensor(const struct device *dev, const struct sensor_config *sc)
{
	size_t data_cnt = 0;

	for (size_t i = 0; i < sc->chan_cnt; i++) {
		data_cnt += sc->chans[i].data_cnt;
	}

	int err = 0;
	size_t data_idx = 0;
	struct sensor_value data[data_cnt];

	err = sensor_sample_fetch(dev);
	for (size_t i = 0; !err && (i < sc->chan_cnt); i++) {
		const struct sampled_channel *sampled_chan = &sc->chans[i];

		err = sensor_channel_get(dev, sampled_chan->chan, &data[data_idx]);
		data_idx += sampled_chan->data_cnt;
	}

	if (err) {
		LOG_ERR("Sensor sampling error (err %d)", err);
	} else {
		send_sensor_event(sc->event_descr, data, ARRAY_SIZE(data));
	}

	return err;
}

static int sample_sensors(int64_t *next_timeout)
{
	int err = 0;
	int64_t cur_uptime = k_uptime_get();

	*next_timeout = INT64_MAX;

	for (size_t i = 0; !err && (i < ARRAY_SIZE(sensor_data)); i++) {
		struct sensor_data *sd = &sensor_data[i];
		const struct sensor_config *sc = &sensor_configs[i];

		if (sd->sample_timeout <= cur_uptime) {
			err = sample_sensor(sd->dev, sc);

			while (sd->sample_timeout <= cur_uptime) {
				sd->sample_timeout += sd->sampling_period;
			}
		}

		if (*next_timeout > sd->sample_timeout) {
			*next_timeout = sd->sample_timeout;
		}
	}

	return err;
}

static int sensor_init(void)
{
	int err = 0;
	int64_t cur_uptime = k_uptime_get();

	for (size_t i = 0; i < ARRAY_SIZE(sensor_data); i++) {
		struct sensor_data *sd = &sensor_data[i];
		const struct sensor_config *sc = &sensor_configs[i];

		sd->dev = device_get_binding(sc->dev_name);
		if (!sd->dev) {
			err = -ENXIO;
			break;
		}
		sd->sampling_period = sc->sampling_period_ms;
		sd->sample_timeout = cur_uptime + sc->sampling_period_ms;
	}

	return err;
}

static void sample_thread_fn(void)
{
	int err = 0;
	int64_t next_timeout;

	err = sensor_init();

	if (!err) {
		state = STATE_ACTIVE;
		module_set_state(MODULE_STATE_READY);
	}

	while (!err) {
		err = sample_sensors(&next_timeout);

		__ASSERT_NO_MSG(next_timeout != INT64_MAX);
		k_sleep(K_TIMEOUT_ABS_MS(next_timeout));
	}

	report_error();
}

static void init(void)
{
	k_thread_create(&sample_thread, sample_thread_stack, SAMPLE_THREAD_STACK_SIZE,
			(k_thread_entry_t)sample_thread_fn, NULL, NULL, NULL,
			SAMPLE_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&sample_thread, "caf_sensor_sampler");
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);

			init();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
