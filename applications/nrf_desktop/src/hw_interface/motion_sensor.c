/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <atomic.h>
#include <spinlock.h>
#include <sys/byteorder.h>

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


#define THREAD_STACK_SIZE	CONFIG_DESKTOP_MOTION_SENSOR_THREAD_STACK_SIZE
#define THREAD_PRIORITY		K_PRIO_PREEMPT(0)

#define NODATA_LIMIT		CONFIG_DESKTOP_MOTION_SENSOR_EMPTY_SAMPLES_COUNT


enum state {
	STATE_DISABLED,
	STATE_DISABLED_SUSPENDED,
	STATE_DISCONNECTED,
	STATE_IDLE,
	STATE_FETCHING,
	STATE_SUSPENDED,
	STATE_SUSPENDED_DISCONNECTED,
};

struct sensor_state {
	struct k_spinlock lock;

	enum state state;
	bool sample;
	u8_t peer_count;
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
	k_spinlock_key_t key = k_spin_lock(&state.lock);

	disable_trigger();

	switch (state.state) {
	case STATE_IDLE:
		state.state = STATE_FETCHING;
		state.sample = true;
		/* Fall-through */

	case STATE_DISCONNECTED:
		/* Wake up thread */
		k_sem_give(&sem);
		break;

	case STATE_SUSPENDED:
	case STATE_SUSPENDED_DISCONNECTED:
		/* Wake up system - this will wake up thread */
		EVENT_SUBMIT(new_wake_up_event());
		break;

	case STATE_FETCHING:
	case STATE_DISABLED:
	case STATE_DISABLED_SUSPENDED:
		/* Invalid state */
		__ASSERT_NO_MSG(false);
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	k_spin_unlock(&state.lock, key);
}

static int motion_read(bool send_event)
{
	struct sensor_value value_x;
	struct sensor_value value_y;

	int err = sensor_sample_fetch(sensor_dev);

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
	BUILD_ASSERT_MSG((MOTION_SENSOR_OPTION_COUNT < 8 *
			  sizeof(state.option_mask)),
			 "");

	if (CONFIG_DESKTOP_MOTION_SENSOR_CPI) {
		set_option(MOTION_SENSOR_OPTION_CPI,
			   CONFIG_DESKTOP_MOTION_SENSOR_CPI);
	}

	if (CONFIG_DESKTOP_MOTION_SENSOR_SLEEP1_TIMEOUT_MS) {
		set_option(MOTION_SENSOR_OPTION_SLEEP1_TIMEOUT,
			   CONFIG_DESKTOP_MOTION_SENSOR_SLEEP1_TIMEOUT_MS);
	}

	if (CONFIG_DESKTOP_MOTION_SENSOR_SLEEP2_TIMEOUT_MS) {
		set_option(MOTION_SENSOR_OPTION_SLEEP2_TIMEOUT,
			   CONFIG_DESKTOP_MOTION_SENSOR_SLEEP2_TIMEOUT_MS);
	}

	if (CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_TIMEOUT_MS) {
		set_option(MOTION_SENSOR_OPTION_SLEEP3_TIMEOUT,
			   CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_TIMEOUT_MS);
	}
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

	k_spinlock_key_t key = k_spin_lock(&state.lock);
	bool is_connected = (state.peer_count != 0);

	switch (state.state) {
	case STATE_DISABLED:
		if (is_connected) {
			state.state = STATE_IDLE;
		} else {
			state.state = STATE_DISCONNECTED;
		}
		break;
	case STATE_DISABLED_SUSPENDED:
		if (is_connected) {
			state.state = STATE_SUSPENDED;
		} else {
			state.state = STATE_SUSPENDED_DISCONNECTED;
		}
		break;
	default:
		/* No action */
		break;
	}

	k_spin_unlock(&state.lock, key);

	do {
		err = enable_trigger();
		if (err == -EBUSY) {
			k_sleep(1);
		}
	} while (err == -EBUSY);

	if (err) {
		LOG_ERR("Cannot enable trigger");
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

	BUILD_ASSERT_MSG(sizeof(option) == sizeof(state.option), "");

	k_spinlock_key_t key = k_spin_lock(&state.lock);
	mask = state.option_mask;
	memcpy(option, state.option, sizeof(option));
	state.option_mask = 0;
	k_spin_unlock(&state.lock, key);

	for (enum motion_sensor_option i = 0;
	     i < MOTION_SENSOR_OPTION_COUNT;
	     i++) {
		int attr = motion_sensor_option_attr[i];

		if ((mask & BIT(i)) == 0) {
			continue;
		}

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
	int err = init();

	if (!err) {
		module_set_state(MODULE_STATE_READY);
	}

	while (!err) {
		bool send_event;
		u32_t option_bm;

		k_sem_take(&sem, K_FOREVER);

		k_spinlock_key_t key = k_spin_lock(&state.lock);
		send_event = (state.state == STATE_FETCHING) && state.sample;
		state.sample = false;
		option_bm = state.option_mask;
		k_spin_unlock(&state.lock, key);

		err = motion_read(send_event);

		bool no_motion = (err == -ENODATA);
		if (unlikely(no_motion)) {
			err = 0;
		}

		if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
		    !err && unlikely(option_bm)) {
			write_config();
		}

		key = k_spin_lock(&state.lock);
		if ((state.state == STATE_FETCHING) && no_motion) {
			state.state = STATE_IDLE;
		}
		if (state.state != STATE_FETCHING) {
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
			if (event->enabled) {
				__ASSERT_NO_MSG(state.peer_count < UCHAR_MAX);
				state.peer_count++;
			} else {
				__ASSERT_NO_MSG(state.peer_count > 0);
				state.peer_count--;
			}

			bool is_connected = (state.peer_count != 0);

			k_spinlock_key_t key = k_spin_lock(&state.lock);
			switch (state.state) {
			case STATE_DISCONNECTED:
				if (is_connected) {
					state.state = STATE_IDLE;
				}
				break;
			case STATE_IDLE:
			case STATE_FETCHING:
				if (!is_connected) {
					state.state = STATE_DISCONNECTED;
				}
				break;
			case STATE_SUSPENDED:
				if (!is_connected) {
					state.state = STATE_SUSPENDED_DISCONNECTED;
				}
				break;
			case STATE_SUSPENDED_DISCONNECTED:
				if (is_connected) {
					state.state = STATE_SUSPENDED;
				}
				break;
			case STATE_DISABLED:
			case STATE_DISABLED_SUSPENDED:
			default:
				/* Ignore */
				break;
			}

			k_spin_unlock(&state.lock, key);

			k_sem_give(&sem);
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
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
		if ((state.state == STATE_SUSPENDED) ||
		    (state.state == STATE_SUSPENDED_DISCONNECTED)) {
			disable_trigger();
			if (state.state == STATE_SUSPENDED) {
				state.state = STATE_FETCHING;
				state.sample = true;
			} else {
				state.state = STATE_DISCONNECTED;
			}
			k_sem_give(&sem);

			module_set_state(MODULE_STATE_READY);
		} else if (state.state == STATE_DISABLED_SUSPENDED) {
			state.state = STATE_DISABLED;
		}
		k_spin_unlock(&state.lock, key);

		return false;
	}

	if (is_power_down_event(eh)) {
		k_spinlock_key_t key = k_spin_lock(&state.lock);

		switch (state.state) {
		case STATE_FETCHING:
		case STATE_IDLE:
		case STATE_DISCONNECTED:
			enable_trigger();

			/* Fall-through */

		case STATE_DISABLED:
			if (state.state == STATE_DISCONNECTED) {
				state.state = STATE_SUSPENDED_DISCONNECTED;
			} else if (state.state == STATE_DISABLED) {
				state.state = STATE_DISABLED_SUSPENDED;
			} else {
				state.state = STATE_SUSPENDED;
			}
			module_set_state(MODULE_STATE_STANDBY);

			/* Sensor will downshift to "rest modes" when inactive.
			 * Interrupt is left enabled to wake system up.
			 * If module is still disabled interrupt will be
			 * enabled when initialization completes.
			 */
			break;

		case STATE_SUSPENDED:
		case STATE_SUSPENDED_DISCONNECTED:
		case STATE_DISABLED_SUSPENDED:
			/* No action */
			break;

		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		k_spin_unlock(&state.lock, key);

		return false;
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
EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
#endif
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
