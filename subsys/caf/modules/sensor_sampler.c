/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <math.h>
#include <drivers/sensor.h>

#include <caf/events/sensor_event.h>
#include <caf/sensor_sampler.h>

#include CONFIG_CAF_SENSOR_SAMPLER_DEF_PATH

#define MODULE sensor_sampler
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_SENSOR_SAMPLER_LOG_LEVEL);

#define SAMPLE_THREAD_STACK_SIZE	CONFIG_CAF_SENSOR_SAMPLER_THREAD_STACK_SIZE
#define SAMPLE_THREAD_PRIORITY		CONFIG_CAF_SENSOR_SAMPLER_THREAD_PRIORITY

enum sensor_state {
	SENSOR_DISABLED,
	SENSOR_SLEEP,
	SENSOR_ACTIVE,
	SENSOR_ERROR,
};

struct sensor_data {
	const struct device *dev;
	int sampling_period;
	int64_t sample_timeout;
	float *prev;
	atomic_t state;
	unsigned int sleep_cnt;
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
static struct k_sem can_sample;


static void report_error(void)
{
	state = STATE_ERROR;
	module_set_state(MODULE_STATE_ERROR);
}

static void send_sensor_event(const char *descr, const float *data, const size_t data_cnt)
{
	struct sensor_event *event = new_sensor_event(sizeof(float) * data_cnt);
	float *data_ptr = sensor_event_get_data_ptr(event);

	event->descr = descr;

	__ASSERT_NO_MSG(sensor_event_get_data_cnt(event) == data_cnt);
	memcpy(data_ptr, data, sizeof(float) * data_cnt);

	EVENT_SUBMIT(event);
}

static struct sensor_data *get_sensor_data(const struct device *dev)
{
	for (size_t i = 0; i < ARRAY_SIZE(sensor_data); i++) {
		if (dev == sensor_data[i].dev) {
			return &sensor_data[i];
		}
	}

	return NULL;
}

static size_t get_sensor_data_cnt(const struct sensor_config *sc)
{
	size_t data_cnt = 0;

	for (size_t i = 0; i < sc->chan_cnt; i++) {
		data_cnt += sc->chans[i].data_cnt;
	}

	return data_cnt;
}

static bool is_active(const struct sensor_config *sc, const float curr, const float prev)
{
	enum act_type type = sc->trigger->activation.type;
	float thresh = sc->trigger->activation.thresh;

	bool is_active = false;

	switch (type) {
	case ACT_TYPE_PERC:
		is_active = fabs(curr - prev) > fabs(prev * thresh/100.0);
		break;
	case ACT_TYPE_ABS:
		is_active = fabs(curr - prev) > fabs(thresh);
		break;
	default:
		report_error();
		break;
	}

	return is_active;
}

static bool can_sensor_sleep(const struct sensor_config *sc,
			     struct sensor_data *sd,
			     const float *curr)
{
	size_t data_cnt = get_sensor_data_cnt(sc);
	bool sleep = true;

	for (size_t i = 0; i < data_cnt; i++) {
		if (is_active(sc, curr[i], sd->prev[i])) {
			sleep = false;
			break;
		}
	}

	if (sleep) {
		unsigned int timeout = sc->trigger->activation.timeout_ms;
		unsigned int period = sc->sampling_period_ms;

		sd->sleep_cnt++;
		if (sd->sleep_cnt >= timeout/period) {
			sd->sleep_cnt = 0;
			return true;
		}
	} else {
		memcpy(sd->prev, curr, data_cnt * sizeof(float));
		sd->sleep_cnt = 0;
	}

	return false;
}

static void trigger_handler(const struct device *dev, struct sensor_trigger *trigger)
{
	sensor_trigger_set(dev, trigger, NULL);

	struct sensor_data *sd = get_sensor_data(dev);

	sd->sample_timeout = k_uptime_get();

	if (!atomic_cas(&sd->state, SENSOR_SLEEP, SENSOR_ACTIVE)) {
		__ASSERT_NO_MSG(false);
		report_error();
		return;
	}

	LOG_DBG("%s sensor woken up", dev->name);
	k_sem_give(&can_sample);
}

static int try_enter_sleep(const struct sensor_config *sc,
			   struct sensor_data *sd,
			   const float *curr)
{
	int err = 0;

	if (can_sensor_sleep(sc, sd, curr)) {
		atomic_set(&sd->state, SENSOR_SLEEP);
		err = sensor_trigger_set(sd->dev, &sc->trigger->cfg, trigger_handler);
		if (err) {
			LOG_ERR("Error setting trigger (err:%d)", err);
			atomic_set(&sd->state, SENSOR_ERROR);
			return err;
		}

		LOG_DBG("%s sensor put to sleep", sc->dev_name);
	}

	return err;
}

static int sample_sensor(struct sensor_data *sd, const struct sensor_config *sc)
{
	int err = 0;
	size_t data_idx = 0;
	size_t data_cnt = get_sensor_data_cnt(sc);
	struct sensor_value data[data_cnt];
	float curr[data_cnt];

	err = sensor_sample_fetch(sd->dev);
	for (size_t i = 0; !err && (i < sc->chan_cnt); i++) {
		const struct sampled_channel *sampled_chan = &sc->chans[i];

		err = sensor_channel_get(sd->dev, sampled_chan->chan, &data[data_idx]);
		data_idx += sampled_chan->data_cnt;
	}

	for (size_t i = 0; i < ARRAY_SIZE(curr); i++) {
		curr[i] = sensor_value_to_double(&data[i]);
	}

	if (sc->trigger) {
		err = try_enter_sleep(sc, sd, curr);
	}

	if (err) {
		LOG_ERR("Sensor sampling error (err %d)", err);
	} else {
		send_sensor_event(sc->event_descr, curr, ARRAY_SIZE(curr));
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

		if (atomic_get(&sd->state) == SENSOR_ACTIVE) {
			if (sd->sample_timeout <= cur_uptime) {
				err = sample_sensor(sd, sc);
			}

			int drops = -1;
			while (sd->sample_timeout <= cur_uptime) {
				sd->sample_timeout += sd->sampling_period;
				drops++;
			}

			if (drops > 0) {
				LOG_WRN("%d sample dropped", drops);
			}
		}

		if (atomic_get(&sd->state) == SENSOR_ACTIVE) {
			if (*next_timeout > sd->sample_timeout) {
				*next_timeout = sd->sample_timeout;
			}
		}
	}

	return err;
}

static int sensor_trigger_init(const struct sensor_config *sc, struct sensor_data *sd)
{
	if (IS_ENABLED(CONFIG_ASSERT)) {
		unsigned int timeout = sc->trigger->activation.timeout_ms;
		unsigned int period = sc->sampling_period_ms;

		__ASSERT(period > 0, "Sampling period must be bigger than 0");
		__ASSERT(timeout > period,
			 "Activation timeout must be bigger than sampling period");
	}

	size_t data_cnt = get_sensor_data_cnt(sc);

	sd->prev = k_malloc(data_cnt * sizeof(float));

	if (!sd->prev) {
		LOG_ERR("Failed to allocate memory");
		__ASSERT_NO_MSG(false);
		return -ENOMEM;
	}

	return 0;
}

static int sensor_init(void)
{
	int err = 0;
	int64_t cur_uptime = k_uptime_get();

	for (size_t i = 0; !err && (i < ARRAY_SIZE(sensor_data)); i++) {
		struct sensor_data *sd = &sensor_data[i];
		const struct sensor_config *sc = &sensor_configs[i];

		sd->dev = device_get_binding(sc->dev_name);
		if (!sd->dev) {
			err = -ENXIO;
			break;
		}
		sd->sampling_period = sc->sampling_period_ms;
		sd->sample_timeout = cur_uptime + sc->sampling_period_ms;

		if (sc->trigger) {
			err = sensor_trigger_init(sc, sd);
			if (err) {
				atomic_set(&sd->state, SENSOR_ERROR);
				break;
			}
		}

		atomic_set(&sd->state, SENSOR_ACTIVE);
	}

	return err;
}

static void sample_thread_fn(void)
{
	int err = 0;
	int64_t next_timeout = 0;

	k_sem_init(&can_sample, 0, 1);

	err = sensor_init();

	if (!err) {
		state = STATE_ACTIVE;
		module_set_state(MODULE_STATE_READY);
	}

	while (!err) {
		k_sem_take(&can_sample, K_TIMEOUT_ABS_MS(next_timeout));

		err = sample_sensors(&next_timeout);
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
