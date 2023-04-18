/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>

#include <caf/events/sensor_event.h>
#include <caf/sensor_manager.h>

#include CONFIG_CAF_SENSOR_MANAGER_DEF_PATH

#define MODULE sensor_manager
#include <caf/events/module_state_event.h>
#include <caf/events/power_event.h>
#include <caf/events/power_manager_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_SENSOR_MANAGER_LOG_LEVEL);

#define SAMPLE_THREAD_STACK_SIZE	CONFIG_CAF_SENSOR_MANAGER_THREAD_STACK_SIZE
#define SAMPLE_THREAD_PRIORITY		CONFIG_CAF_SENSOR_MANAGER_THREAD_PRIORITY

struct sensor_data {
	int sampling_period;
	int64_t sample_timeout;
	struct sensor_value *prev;
	atomic_t state;
	unsigned int sleep_cntd;
	atomic_t event_cnt;
};

static struct sensor_data sensor_data[ARRAY_SIZE(sensor_configs)];

static K_THREAD_STACK_DEFINE(sample_thread_stack, SAMPLE_THREAD_STACK_SIZE);
static struct k_thread sample_thread;
static struct k_sem can_sample;


static void update_sensor_state(const struct sm_sensor_config *sc, struct sensor_data *sd,
				const enum sensor_state state)
{
	struct sensor_state_event *event = new_sensor_state_event();

	event->descr = sc->event_descr;
	event->state = state;

	atomic_set(&sd->state, state);
	APP_EVENT_SUBMIT(event);
}

static void send_sensor_event(const char *descr, const struct sensor_value *data, const size_t data_cnt,
			      atomic_t *event_cnt)
{
	struct sensor_event *event = new_sensor_event(sizeof(struct sensor_value) * data_cnt);
	struct sensor_value *data_ptr = sensor_event_get_data_ptr(event);

	event->descr = descr;

	__ASSERT_NO_MSG(sensor_event_get_data_cnt(event) == data_cnt);
	memcpy(data_ptr, data, sizeof(struct sensor_value) * data_cnt);

	atomic_inc(event_cnt);
	APP_EVENT_SUBMIT(event);
}

static struct sensor_data *get_sensor_data(const struct device *dev)
{
	for (size_t i = 0; i < ARRAY_SIZE(sensor_configs); i++) {
		if (dev == sensor_configs[i].dev) {
			/* sensor_configs indices match sensor_data ones */
			return &sensor_data[i];
		}
	}

	__ASSERT_NO_MSG(false);

	return NULL;
}

static const struct sm_sensor_config *get_sensor_config(const struct device *dev)
{
	for (size_t i = 0; i < ARRAY_SIZE(sensor_configs); i++) {
		if (dev == sensor_configs[i].dev) {
			return &sensor_configs[i];
		}
	}

	__ASSERT_NO_MSG(false);

	return NULL;
}

static size_t get_sensor_data_cnt(const struct sm_sensor_config *sc)
{
	size_t data_cnt = 0;

	for (size_t i = 0; i < sc->chan_cnt; i++) {
		data_cnt += sc->chans[i].data_cnt;
	}

	return data_cnt;
}

static void reset_sensor_sleep_cnt(const struct sm_sensor_config *sc,
				   struct sensor_data *sd)
{
	unsigned int timeout = sc->trigger->activation.timeout_ms;
	unsigned int period = sc->sampling_period_ms;

	sd->sleep_cntd = DIV_ROUND_UP(timeout, period);
}

static bool process_sensor_trigger_values(const struct sm_sensor_config *sc, struct sensor_data *sd,
					  const struct sensor_value curr, const struct sensor_value prev)
{
	enum act_type type = sc->trigger->activation.type;
	struct sensor_value thresh = sc->trigger->activation.thresh;

	bool is_active = false;

	switch (type) {
	case ACT_TYPE_ABS:
		is_active = sensor_value_greater_then(sensor_value_abs_difference(curr, prev), thresh);
		break;
	default:
		__ASSERT(false, "Invalid configuration");
		update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
		break;
	}

	return is_active;
}

static void process_sensor_activity(const struct sm_sensor_config *sc,
				    struct sensor_data *sd,
				    const struct sensor_value *curr)
{
	size_t data_cnt = get_sensor_data_cnt(sc);
	bool sleep = true;

	for (size_t i = 0; i < data_cnt; i++) {
		if (process_sensor_trigger_values(sc, sd, curr[i], sd->prev[i])) {
			sleep = false;
			break;
		}
	}

	if (sleep) {
		if (sd->sleep_cntd > 0) {
			--(sd->sleep_cntd);
			LOG_DBG("Sleep_cntd: %d", sd->sleep_cntd);
		}
	} else {
		memcpy(sd->prev, curr, data_cnt * sizeof(struct sensor_value));
		reset_sensor_sleep_cnt(sc, sd);
	}
}

static bool is_sensor_active(const struct sensor_data *sd)
{
	return sd->sleep_cntd != 0;
}

static void sensor_wake_up_post(const struct sm_sensor_config *sc, struct sensor_data *sd)
{
	sd->sample_timeout = k_uptime_get();
	if (sc->trigger) {
		reset_sensor_sleep_cnt(sc, sd);
	}
	update_sensor_state(sc, sd, SENSOR_STATE_ACTIVE);
}

static void trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	int err = sensor_trigger_set(dev, trigger, NULL);

	ARG_UNUSED(err);
	__ASSERT_NO_MSG(!err);
	const struct sm_sensor_config *sc = get_sensor_config(dev);
	struct sensor_data *sd = get_sensor_data(dev);

	if (atomic_get(&sd->state) != SENSOR_STATE_SLEEP) {
		__ASSERT_NO_MSG(false);
		update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
		return;
	}

	sensor_wake_up_post(sc, sd);

	if (IS_ENABLED(CONFIG_CAF_SENSOR_MANAGER_ACTIVE_PM)) {
		LOG_INF("Event (%d) from sensor %s triggers wake up",
			trigger->type, sc->dev->name);
		APP_EVENT_SUBMIT(new_wake_up_event());
	}

	k_sem_give(&can_sample);
}

static void enter_sleep(const struct sm_sensor_config *sc,
			struct sensor_data *sd)
{
	k_sched_lock();
	int err = sensor_trigger_set(sc->dev, &sc->trigger->cfg, trigger_handler);

	if (err) {
		LOG_ERR("Error setting trigger (err:%d)", err);
		update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
	} else {
		update_sensor_state(sc, sd, SENSOR_STATE_SLEEP);
	}
	k_sched_unlock();
}

static void sample_sensor(struct sensor_data *sd, const struct sm_sensor_config *sc)
{
	size_t data_idx = 0;
	size_t data_cnt = get_sensor_data_cnt(sc);
	struct sensor_value data[data_cnt];

	int err = sensor_sample_fetch(sc->dev);

	for (size_t i = 0; !err && (i < sc->chan_cnt); i++) {
		const struct caf_sampled_channel *sampled_chan = &sc->chans[i];

		err = sensor_channel_get(sc->dev, sampled_chan->chan, &data[data_idx]);
		data_idx += sampled_chan->data_cnt;
	}

	if (err) {
		LOG_ERR("Sensor sampling error (err %d)", err);
		update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
	} else {
		if (atomic_get(&sd->event_cnt) < sc->active_events_limit) {
			send_sensor_event(sc->event_descr, data, ARRAY_SIZE(data),
					  &sd->event_cnt);
		} else {
			LOG_WRN("Did not send event due to too many active events on sensor: %s",
				sc->dev->name);
		}

		if (sc->trigger && IS_ENABLED(CONFIG_CAF_SENSOR_MANAGER_PM)) {
			process_sensor_activity(sc, sd, data);
			if (!is_sensor_active(sd)) {
				enter_sleep(sc, sd);
			}

		}
	}
}

static size_t sample_sensors(int64_t *next_timeout)
{
	size_t alive_sensors = 0;
	int64_t cur_uptime = k_uptime_get();

	*next_timeout = INT64_MAX;

	for (size_t i = 0; i < ARRAY_SIZE(sensor_data); i++) {
		struct sensor_data *sd = &sensor_data[i];
		const struct sm_sensor_config *sc = &sensor_configs[i];

		if (atomic_get(&sd->state) == SENSOR_STATE_ACTIVE) {
			if (sd->sample_timeout <= cur_uptime) {
				sample_sensor(sd, sc);
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

		if (atomic_get(&sd->state) != SENSOR_STATE_ERROR) {
			alive_sensors++;
			if (atomic_get(&sd->state) == SENSOR_STATE_ACTIVE) {
				if (*next_timeout > sd->sample_timeout) {
					*next_timeout = sd->sample_timeout;
				}
			}
		}
	}

	return alive_sensors;
}

static int sensor_trigger_init(const struct sm_sensor_config *sc, struct sensor_data *sd)
{
	if (IS_ENABLED(CONFIG_ASSERT)) {
		unsigned int timeout = sc->trigger->activation.timeout_ms;
		unsigned int period = sc->sampling_period_ms;

		__ASSERT(period > 0, "Sampling period must be bigger than 0");
		__ASSERT(timeout > period,
			 "Activation timeout must be bigger than sampling period");
		ARG_UNUSED(timeout);
		ARG_UNUSED(period);
	}

	size_t data_cnt = get_sensor_data_cnt(sc);

	sd->prev = k_malloc(data_cnt * sizeof(struct sensor_value));

	if (!sd->prev) {
		LOG_ERR("Failed to allocate memory");
		__ASSERT_NO_MSG(false);
		return -ENOMEM;
	}

	reset_sensor_sleep_cnt(sc, sd);

	LOG_INF("Trigger configured (threshold: %d,%d time: %d ms)",
		sc->trigger->activation.thresh.val1,
		sc->trigger->activation.thresh.val2,
		sc->trigger->activation.timeout_ms);
	return 0;
}

static void configure_max_power_state(void)
{
	if (IS_ENABLED(CONFIG_CAF_SENSOR_MANAGER_ACTIVE_PM)) {
		static enum power_manager_level last_power_state = POWER_MANAGER_LEVEL_MAX;
		enum power_manager_level power_state;
		bool active = false;
		bool trigger_present = false;

		/* Travel through all the sensors and check their state
		 * and configuration.
		 * Depending on the "most active" sensor the overall allowed
		 * power state would be decided.
		 */
		for (size_t i = 0; i < ARRAY_SIZE(sensor_data); i++) {
			struct sensor_data *sd = &sensor_data[i];
			const struct sm_sensor_config *sc = &sensor_configs[i];

			if (atomic_get(&sd->state) == SENSOR_STATE_ERROR) {
				/* Ignoring sensor in error state */
				continue;
			}
			if (is_sensor_active(sd)) {
				/* Some sensor is active - nothing more to search,
				 * the highest power state needs to be configured.
				 */
				active = true;
				break;
			}

			/* Sensor is not active - check if it has trigger enabled.
			 * If the trigger is enabled we cannot power down lower
			 * than SUSPENDED state.
			 */
			if (sc->trigger != NULL) {
				trigger_present = true;
			}
		}

		power_state = active ? POWER_MANAGER_LEVEL_ALIVE :
			      trigger_present ? POWER_MANAGER_LEVEL_SUSPENDED :
			      POWER_MANAGER_LEVEL_MAX;

		if (power_state != last_power_state) {
			power_manager_restrict(MODULE_IDX(MODULE), power_state);
			last_power_state = power_state;
		}
	}
}

static size_t sensor_init(void)
{
	size_t alive_sensors = 0;
	int64_t cur_uptime = k_uptime_get();

	for (size_t i = 0; i < ARRAY_SIZE(sensor_data); i++) {
		struct sensor_data *sd = &sensor_data[i];
		const struct sm_sensor_config *sc = &sensor_configs[i];

		if (!device_is_ready(sc->dev)) {
			update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
			LOG_ERR("%s sensor not ready", sc->dev->name);
			continue;
		}
		sd->sampling_period = sc->sampling_period_ms;
		sd->sample_timeout = cur_uptime + sc->sampling_period_ms;

		if (sc->trigger && IS_ENABLED(CONFIG_CAF_SENSOR_MANAGER_PM)) {
			int err = sensor_trigger_init(sc, sd);

			if (err) {
				update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
				LOG_ERR("%s sensor cannot initialize trigger", sc->dev->name);
				continue;
			}
		}

		update_sensor_state(sc, sd, SENSOR_STATE_ACTIVE);
		alive_sensors++;
	}

	return alive_sensors;
}

static void sample_thread_fn(void)
{
	size_t alive_sensors = 0;
	int64_t next_timeout = 0;

	k_sem_init(&can_sample, 0, 1);

	alive_sensors = sensor_init();

	if (alive_sensors) {
		module_set_state(MODULE_STATE_READY);

		while (alive_sensors > 0) {
			k_sem_take(&can_sample, K_TIMEOUT_ABS_MS(next_timeout));

			alive_sensors = sample_sensors(&next_timeout);
			configure_max_power_state();
		}
	}

	module_set_state(MODULE_STATE_ERROR);
}

static void init(void)
{
	k_thread_create(&sample_thread, sample_thread_stack, SAMPLE_THREAD_STACK_SIZE,
			(k_thread_entry_t)sample_thread_fn, NULL, NULL, NULL,
			SAMPLE_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&sample_thread, "caf_sensor_manager");
}

static bool handle_power_down_event(const struct app_event_header *aeh)
{
	for (size_t i = 0; i < ARRAY_SIZE(sensor_data); i++) {
		struct sensor_data *sd = &sensor_data[i];
		const struct sm_sensor_config *sc = &sensor_configs[i];

		/* Locking the scheduler to prevent concurrent access to sensor state. */
		k_sched_lock();
		if (atomic_get(&sd->state) != SENSOR_STATE_ERROR) {
			if (sc->trigger) {
				enter_sleep(sc, sd);
			} else if (atomic_get(&sd->state) == SENSOR_STATE_ACTIVE) {
				int ret = 0;

				if (sc->suspend) {
					ret = pm_device_action_run(sc->dev,
								   PM_DEVICE_ACTION_SUSPEND);
				}

				if (ret) {
					LOG_ERR("Sensor %s cannot be suspended (%d)",
						sc->dev->name, ret);
					update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
				} else {
					update_sensor_state(sc, sd, SENSOR_STATE_SLEEP);
				}
			}
		}
		k_sched_unlock();
	}
	configure_max_power_state();
	return false;
}

static bool handle_wake_up_event(const struct app_event_header *aeh)
{
	for (size_t i = 0; i < ARRAY_SIZE(sensor_data); i++) {
		struct sensor_data *sd = &sensor_data[i];
		const struct sm_sensor_config *sc = &sensor_configs[i];

		/* Locking the scheduler to prevent concurrent access to sensor state. */
		k_sched_lock();
		if (atomic_get(&sd->state) != SENSOR_STATE_ERROR) {
			int ret = 0;

			if (sc->trigger) {
				ret = sensor_trigger_set(sc->dev, &sc->trigger->cfg, NULL);
				__ASSERT_NO_MSG(!ret);
				ret = 0;
			} else if (sc->suspend) {
				ret = pm_device_action_run(sc->dev, PM_DEVICE_ACTION_RESUME);
				if (ret) {
					LOG_ERR("Sensor %s cannot be resumed (%d)",
						sc->dev->name, ret);
					update_sensor_state(sc, sd, SENSOR_STATE_ERROR);
				}
			}

			if (!ret) {
				LOG_DBG("Sensor %s wake up", sc->dev->name);
				sensor_wake_up_post(sc, sd);
			}
		}
		k_sched_unlock();
	}
	k_sem_give(&can_sample);
	return false;
}

static bool handle_sensor_event(const struct app_event_header *aeh)
{
	const struct sensor_event *event = cast_sensor_event(aeh);

	for (size_t i = 0; i < ARRAY_SIZE(sensor_configs); i++) {
		if (event->descr == sensor_configs[i].event_descr) {
			struct sensor_data *sd = &sensor_data[i];

			atomic_dec(&sd->event_cnt);
			__ASSERT_NO_MSG(!(atomic_get(&sd->event_cnt) < 0));
			return false;
		}
	}

	return false;
}

static bool handle_set_sensor_period_event(const struct app_event_header *aeh)
{
	const struct set_sensor_period_event *event = cast_set_sensor_period_event(aeh);

	for (size_t i = 0; i < ARRAY_SIZE(sensor_data); i++) {
		const struct sm_sensor_config *sc = &sensor_configs[i];

		if (event->descr == sc->event_descr) {
			struct sensor_data *sd = &sensor_data[i];

			sd->sampling_period = event->sampling_period;
			sd->sample_timeout = k_uptime_get() + event->sampling_period;
			if (sd->state == SENSOR_STATE_ACTIVE) {
				k_sem_give(&can_sample);
			}

			break;
		}
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
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

	if (is_sensor_event(aeh)) {
		return handle_sensor_event(aeh);
	}

	if (is_set_sensor_period_event(aeh)) {
		return handle_set_sensor_period_event(aeh);
	}

	if (IS_ENABLED(CONFIG_CAF_SENSOR_MANAGER_PM) && is_power_down_event(aeh)) {
		return handle_power_down_event(aeh);
	}

	if (IS_ENABLED(CONFIG_CAF_SENSOR_MANAGER_PM) && is_wake_up_event(aeh)) {
		return handle_wake_up_event(aeh);
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, set_sensor_period_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, sensor_event);
#if CONFIG_CAF_SENSOR_MANAGER_PM
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif /* CONFIG_CAF_SENSOR_MANAGER_PM */
