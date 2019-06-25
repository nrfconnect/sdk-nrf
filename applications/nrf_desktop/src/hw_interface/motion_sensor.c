/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <atomic.h>
#include <spinlock.h>
#include <misc/byteorder.h>

#include <device.h>
#include <sensor.h>

#include "motion_sensor.h"

#include "event_manager.h"
#include "motion_event.h"
#include "power_event.h"
#include "hid_event.h"
#include "config_event.h"

#define MODULE motion
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_MOTION_LOG_LEVEL);


#define POLL_TIMEOUT_MS		K_MSEC(500)

#define THREAD_STACK_SIZE	CONFIG_DESKTOP_MOTION_THREAD_STACK_SIZE
#define THREAD_PRIORITY		K_PRIO_PREEMPT(0)

#define NODATA_LIMIT		10


enum state {
	STATE_DISABLED,
	STATE_IDLE,
	STATE_FETCHING,
	STATE_SUSPENDED,
};

struct sensor_state {
	struct k_spinlock lock;

	enum state state;
	bool connected;
	bool sample;
	bool last_read_burst;
	u32_t option[MOTION_SENSOR_OPTION_COUNT];
	u32_t option_mask;
};


static K_SEM_DEFINE(sem, 1, 1);
static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;

static struct device *sensor_dev;

static struct sensor_state state;


static void data_ready_handler(struct device *dev, struct sensor_trigger *trig);


static int enable_trigger(void)
{
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	int err = sensor_trigger_set(sensor_dev, &trig, data_ready_handler);

	return err;
}

static int disable_trigger(void)
{
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	int err = sensor_trigger_set(sensor_dev, &trig, NULL);

	return err;
}

static void data_ready_handler(struct device *dev, struct sensor_trigger *trig)
{
	/* Enter active polling mode until no motion */

	disable_trigger();

	switch (state.state) {
	case STATE_IDLE:
		/* Wake up thread */
		state.state = STATE_FETCHING;
		state.sample = true;
		k_sem_give(&sem);
		break;

	case STATE_SUSPENDED:
		/* Wake up system - this will wake up thread */
		EVENT_SUBMIT(new_wake_up_event());
		break;

	case STATE_FETCHING:
	case STATE_DISABLED:
		/* Invalid state */
		__ASSERT_NO_MSG(false);
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}
}

static int motion_read(bool send_event)
{
	struct sensor_value value_x;
	struct sensor_value value_y;

	int err;

	err = sensor_sample_fetch(sensor_dev);

	if (!err) {
		err = sensor_channel_get(sensor_dev, SENSOR_CHAN_POS_DX,
					 &value_x);
	}
	if (!err) {
		err = sensor_channel_get(sensor_dev, SENSOR_CHAN_POS_DY,
					 &value_y);
	}

	if (err || !send_event) {
		return err;
	}

	static unsigned int nodata;
	if (!value_x.val1 && !value_y.val1) {
		if (nodata < NODATA_LIMIT) {
			nodata++;
		} else {
			nodata = 0;

			return -ENODATA;
		}
	} else {
		nodata = 0;
	}

	struct motion_event *event = new_motion_event();

	event->dx = value_x.val1;
	event->dy = value_y.val1;
	EVENT_SUBMIT(event);

	return err;
}

static bool set_option(enum motion_sensor_option option, u32_t value)
{
	if (motion_sensor_option_attr[option] != -ENOTSUP) {
		k_spinlock_key_t key = k_spin_lock(&state.lock);
		state.option[option] = value;
		WRITE_BIT(state.option_mask, option, true);
		k_spin_unlock(&state.lock, key);

		return true;
	}

	LOG_INF("Sensor option %d is not supported", option);

	return false;
}

static void set_default_configuration(void)
{
	static_assert(MOTION_SENSOR_OPTION_COUNT < 8 * sizeof(state.option_mask), "");

	set_option(MOTION_SENSOR_OPTION_CPI, CONFIG_DESKTOP_MOTION_SENSOR_CPI);
	set_option(MOTION_SENSOR_OPTION_SLEEP1_TIMEOUT,
			CONFIG_DESKTOP_MOTION_SENSOR_SLEEP1_TIMEOUT_MS);
	set_option(MOTION_SENSOR_OPTION_SLEEP2_TIMEOUT,
			CONFIG_DESKTOP_MOTION_SENSOR_SLEEP2_TIMEOUT_MS);
	set_option(MOTION_SENSOR_OPTION_SLEEP3_TIMEOUT,
			CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_TIMEOUT_MS);
}

static int init(void)
{
	int err = -ENXIO;

	sensor_dev = device_get_binding(MOTION_SENSOR_DEV_NAME);
	if (!sensor_dev) {
		LOG_ERR("Cannot get motion sensor device");
		return err;
	}

	set_default_configuration();

	state.state = STATE_IDLE;
	module_set_state(MODULE_STATE_READY);

	do {
		err = enable_trigger();
		if (err == -EBUSY) {
			k_sleep(1);
		}
	} while (err == -EBUSY);

	if (err) {
		LOG_ERR("Cannot enable trigger");
		module_set_state(MODULE_STATE_ERROR);
	}

	return err;
}

static bool is_my_config_id(u8_t config_id)
{
	return (GROUP_FIELD_GET(config_id) == EVENT_GROUP_SETUP) &&
	       (MOD_FIELD_GET(config_id) == SETUP_MODULE_SENSOR);
}

static enum motion_sensor_option configid_2_option(u8_t config_id)
{
	switch (OPT_FIELD_GET(config_id)) {
	case SENSOR_OPT_CPI:
		return MOTION_SENSOR_OPTION_CPI;

	case SENSOR_OPT_DOWNSHIFT_RUN:
		return MOTION_SENSOR_OPTION_SLEEP1_TIMEOUT;

	case SENSOR_OPT_DOWNSHIFT_REST1:
		return MOTION_SENSOR_OPTION_SLEEP2_TIMEOUT;

	case SENSOR_OPT_DOWNSHIFT_REST2:
		return MOTION_SENSOR_OPTION_SLEEP3_TIMEOUT;

	default:
		LOG_WRN("Unsupported sensor option (%" PRIu8 ")", config_id);
		return MOTION_SENSOR_OPTION_COUNT;
	}
}

static void fetch_config(u8_t config_id, u8_t *data)
{
	enum motion_sensor_option option = configid_2_option(config_id);

	if (option < MOTION_SENSOR_OPTION_COUNT) {
		k_spinlock_key_t key = k_spin_lock(&state.lock);
		sys_put_le32(state.option[option], data);
		k_spin_unlock(&state.lock, key);
	} else {
		sys_put_le32(0, data);
	}

	return;
}

static void update_config(const u8_t config_id, const u8_t *data, size_t size)
{
	enum motion_sensor_option option = configid_2_option(config_id);

	if (option < MOTION_SENSOR_OPTION_COUNT) {
		if (size != sizeof(state.option[option])) {
			LOG_WRN("Invalid option size (%zu)", size);
			return;
		}

		if (set_option(option, sys_get_le32(data))) {
			k_sem_give(&sem);
		}
	}
}

static void write_config(void)
{
	u32_t option[MOTION_SENSOR_OPTION_COUNT];
	u32_t mask;

	static_assert(sizeof(option) == sizeof(state.option), "");

	k_spinlock_key_t key = k_spin_lock(&state.lock);
	mask = state.option_mask;
	memcpy(option, state.option, sizeof(option));
	state.option_mask = 0;
	k_spin_unlock(&state.lock, key);

	if (IS_ENABLED(MOTION_SENSOR_ATTR_CPI) &&
	    ((mask & BIT(MOTION_SENSOR_OPTION_CPI)) != 0)) {
	}

	for (enum motion_sensor_option i = 0;
	     i < MOTION_SENSOR_OPTION_COUNT;
	     i++) {
		int attr = motion_sensor_option_attr[i];

		__ASSERT_NO_MSG(attr != -ENOTSUP);

		struct sensor_value val = {
			.val1 = option[i]
		};

		int err = sensor_attr_set(sensor_dev, SENSOR_CHAN_ALL,
					  (enum sensor_attribute)attr, &val);

		if (err) {
			LOG_WRN("Cannot update attr %d (err:%d)", attr, err);
		}
	}
}

static void motion_thread_fn(void)
{
	s32_t timeout = POLL_TIMEOUT_MS;

	int err = init();

	while (!err) {
		err = k_sem_take(&sem, timeout);
		if (err && (err != -EAGAIN)) {
			continue;
		}

		k_spinlock_key_t key = k_spin_lock(&state.lock);
		bool sample = (state.state == STATE_FETCHING) &&
			      state.connected &&
			      state.sample;
		state.sample = false;
		u32_t option_mask = state.option_mask;
		k_spin_unlock(&state.lock, key);

		err = motion_read(sample);

		bool no_motion = (err == -ENODATA);
		if (no_motion) {
			err = 0;
		}

		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
		    !err && option_mask) {
			write_config();
		}

		if (err) {
			continue;
		}

		if (!sample) {
			timeout = POLL_TIMEOUT_MS;
			continue;
		}
		timeout = K_FOREVER;

		key = k_spin_lock(&state.lock);
		if ((state.state == STATE_FETCHING) && no_motion) {
			state.state = STATE_IDLE;
			enable_trigger();
		}
		k_spin_unlock(&state.lock, key);
	}

	/* This thread is supposed to run forever. */
	module_set_state(MODULE_STATE_ERROR);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_hid_report_sent_event(eh)) {
		const struct hid_report_sent_event *event =
			cast_hid_report_sent_event(eh);

		if (event->report_type == IN_REPORT_MOUSE) {
			k_spinlock_key_t key = k_spin_lock(&state.lock);
			if (state.state == STATE_FETCHING) {
				state.sample = true;
				k_sem_give(&sem);
			}
			k_spin_unlock(&state.lock, key);
		}

		return false;
	}

	if (is_hid_report_subscription_event(eh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(eh);

		if (event->report_type == IN_REPORT_MOUSE) {
			static u8_t peer_count;

			if (event->enabled) {
				__ASSERT_NO_MSG(peer_count < UCHAR_MAX);
				peer_count++;
			} else {
				__ASSERT_NO_MSG(peer_count > 0);
				peer_count--;
			}

			bool is_connected = (peer_count != 0);

			k_spinlock_key_t key = k_spin_lock(&state.lock);
			state.connected = is_connected;
			k_spin_unlock(&state.lock, key);
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			/* Start state machine thread */
			__ASSERT_NO_MSG(state.state == STATE_DISABLED);
			k_thread_create(&thread, thread_stack,
					THREAD_STACK_SIZE,
					(k_thread_entry_t)motion_thread_fn,
					NULL, NULL, NULL,
					THREAD_PRIORITY, 0, K_NO_WAIT);
			k_thread_name_set(&thread, MODULE_NAME "_thread");

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		k_spinlock_key_t key = k_spin_lock(&state.lock);
		if (state.state == STATE_SUSPENDED) {
			disable_trigger();
			state.state = STATE_FETCHING;
			state.sample = true;
			k_sem_give(&sem);

			module_set_state(MODULE_STATE_READY);
		}
		k_spin_unlock(&state.lock, key);

		return false;
	}

	if (is_power_down_event(eh)) {
		bool suspended = false;

		k_spinlock_key_t key = k_spin_lock(&state.lock);
		switch (state.state) {
		case STATE_FETCHING:
		case STATE_IDLE:
			enable_trigger();
			state.state = STATE_SUSPENDED;
			module_set_state(MODULE_STATE_STANDBY);

			/* Sensor will downshift to "rest modes" when inactive.
			 * Interrupt is left enabled to wake system up.
			 */

			/* Fall-through */

		case STATE_SUSPENDED:
			suspended = true;
			break;

		case STATE_DISABLED:
			/* No action */
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}
		k_spin_unlock(&state.lock, key);

		return !suspended;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {
		if (is_config_event(eh)) {
			const struct config_event *event = cast_config_event(eh);

			if (is_my_config_id(event->id)) {
				update_config(event->id, event->dyndata.data,
				      event->dyndata.size);
			}

			return false;
		}

		if (is_config_fetch_request_event(eh)) {
			const struct config_fetch_request_event *event =
				cast_config_fetch_request_event(eh);

			if (is_my_config_id(event->id)) {
				size_t data_size = sizeof(u32_t);
				struct config_fetch_event *fetch_event =
					new_config_fetch_event(data_size);

				fetch_event->id = event->id;
				fetch_event->recipient = event->recipient;
				fetch_event->channel_id = event->channel_id;
				fetch_config(fetch_event->id,
					     fetch_event->dyndata.data);

				EVENT_SUBMIT(fetch_event);
			}

			return false;
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
#endif
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
