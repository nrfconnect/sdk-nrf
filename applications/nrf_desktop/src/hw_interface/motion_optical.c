/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <atomic.h>
#include <spinlock.h>
#include <misc/byteorder.h>

#include <device.h>
#include <sensor.h>

#include <sensor/pmw3360.h>

#include "event_manager.h"
#include "motion_event.h"
#include "power_event.h"
#include "hid_event.h"
#include "config_event.h"

#define MODULE motion
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_MOTION_LOG_LEVEL);


#define OPTICAL_POLL_TIMEOUT_MS		500

#define OPTICAL_THREAD_STACK_SIZE	CONFIG_DESKTOP_OPTICAL_THREAD_STACK_SIZE
#define OPTICAL_THREAD_PRIORITY		K_PRIO_PREEMPT(0)

#define NODATA_LIMIT			10


enum state {
	STATE_DISABLED,
	STATE_IDLE,
	STATE_FETCHING,
	STATE_SUSPENDED,
};

enum config_option {
	CONFIG_OPTION_CPI,
	CONFIG_OPTION_TIME_RUN,
	CONFIG_OPTION_TIME_REST1,
	CONFIG_OPTION_TIME_REST2,

	CONFIG_OPTION_COUNT,
};

struct sensor_state {
	struct k_spinlock lock;

	enum state state;
	bool connected;
	bool sample;
	bool last_read_burst;
	u32_t option[CONFIG_OPTION_COUNT];
	u32_t option_mask;
};


static K_SEM_DEFINE(sem, 1, 1);
static K_THREAD_STACK_DEFINE(thread_stack, OPTICAL_THREAD_STACK_SIZE);
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

static int init(void)
{
	int err = -ENXIO;

	sensor_dev = device_get_binding("PMW3360");
	if (!sensor_dev) {
		LOG_ERR("Cannot get motion sensor device");
		return err;
	}

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

static void update_config(const u8_t config_id, const u8_t *data, size_t size)
{
	if ((GROUP_FIELD_GET(config_id) != EVENT_GROUP_SETUP) ||
	    (MOD_FIELD_GET(config_id) != SETUP_MODULE_SENSOR)) {
		/* Not for us */
		return;
	}

	enum config_option option;

	switch (OPT_FIELD_GET(config_id)) {
	case SENSOR_OPT_CPI:
		option = CONFIG_OPTION_CPI;
		break;

	case SENSOR_OPT_DOWNSHIFT_RUN:
		option = CONFIG_OPTION_TIME_RUN;
		break;

	case SENSOR_OPT_DOWNSHIFT_REST1:
		option = CONFIG_OPTION_TIME_REST1;
		break;

	case SENSOR_OPT_DOWNSHIFT_REST2:
		option = CONFIG_OPTION_TIME_REST2;
		break;

	default:
		LOG_WRN("Unsupported sensor option (%" PRIu8 ")", config_id);
		return;
	}

	if (size != sizeof(state.option[option])) {
		LOG_WRN("Invalid option size (%zu)", size);
		return;
	}

	static_assert(CONFIG_OPTION_COUNT < 8 * sizeof(state.option_mask), "");

	k_spinlock_key_t key = k_spin_lock(&state.lock);
	state.option[option] = sys_get_le32(data);
	WRITE_BIT(state.option_mask, option, true);
	k_spin_unlock(&state.lock, key);

	k_sem_give(&sem);
}

static int write_config(void)
{
	u32_t option[CONFIG_OPTION_COUNT];
	u32_t mask;

	static_assert(sizeof(option) == sizeof(state.option), "");

	k_spinlock_key_t key = k_spin_lock(&state.lock);
	mask = state.option_mask;
	memcpy(option, state.option, sizeof(option));
	state.option_mask = 0;
	k_spin_unlock(&state.lock, key);

	if ((mask & BIT(CONFIG_OPTION_CPI)) != 0) {
		struct sensor_value val =
			PMW3360_CPI_TO_SVALUE(state.option[CONFIG_OPTION_CPI]);
		int err = sensor_attr_set(sensor_dev, SENSOR_CHAN_ALL,
					  PMW3360_ATTR_CPI, &val);

		if (err) {
			LOG_ERR("Cannot update CPI");
			return err;
		}
	}
	if ((mask & BIT(CONFIG_OPTION_TIME_RUN)) != 0) {
		struct sensor_value val =
			PMW3360_TIME_TO_SVALUE(state.option[CONFIG_OPTION_TIME_RUN]);
		int err = sensor_attr_set(sensor_dev, SENSOR_CHAN_ALL,
					  PMW3360_ATTR_RUN_DOWNSHIFT_TIME, &val);

		if (err) {
			LOG_ERR("Cannot update RUN downshift time");
			return err;
		}
	}
	if ((mask & BIT(CONFIG_OPTION_TIME_REST1)) != 0) {
		struct sensor_value val =
			PMW3360_TIME_TO_SVALUE(state.option[CONFIG_OPTION_TIME_REST1]);
		int err = sensor_attr_set(sensor_dev, SENSOR_CHAN_ALL,
					  PMW3360_ATTR_REST1_DOWNSHIFT_TIME, &val);

		if (err) {
			LOG_ERR("Cannot update REST1 downshift time");
			return err;
		}
	}
	if ((mask & BIT(CONFIG_OPTION_TIME_REST2)) != 0) {
		struct sensor_value val =
			PMW3360_TIME_TO_SVALUE(state.option[CONFIG_OPTION_TIME_REST2]);
		int err = sensor_attr_set(sensor_dev, SENSOR_CHAN_ALL,
					  PMW3360_ATTR_REST2_DOWNSHIFT_TIME, &val);

		if (err) {
			LOG_ERR("Cannot update REST2 downshift time");
			return err;
		}
	}

	return 0;
}

static void optical_thread_fn(void)
{
	s32_t timeout = K_MSEC(OPTICAL_POLL_TIMEOUT_MS);

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
			err = write_config();
		}

		if (err) {
			continue;
		}

		if (!sample) {
			timeout = K_MSEC(OPTICAL_POLL_TIMEOUT_MS);
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
					OPTICAL_THREAD_STACK_SIZE,
					(k_thread_entry_t)optical_thread_fn,
					NULL, NULL, NULL,
					OPTICAL_THREAD_PRIORITY, 0, K_NO_WAIT);
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
			const struct config_event *event =
				cast_config_event(eh);

			update_config(event->id, event->dyndata.data,
				      event->dyndata.size);

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
EVENT_SUBSCRIBE(MODULE, config_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
