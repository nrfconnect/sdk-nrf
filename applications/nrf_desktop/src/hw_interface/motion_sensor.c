/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/settings/settings.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "motion_sensor.h"

#include <app_event_manager.h>
#include "motion_event.h"
#include <caf/events/power_event.h>
#include "hid_event.h"
#include "config_event.h"
#include "usb_event.h"

#define MODULE motion
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_MOTION_LOG_LEVEL);


#define THREAD_STACK_SIZE	CONFIG_DESKTOP_MOTION_SENSOR_THREAD_STACK_SIZE
#define THREAD_PRIORITY		K_PRIO_PREEMPT(0)

#define NODATA_LIMIT		CONFIG_DESKTOP_MOTION_SENSOR_EMPTY_SAMPLES_COUNT

#define MAX_KEY_LEN 20

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
	uint8_t peer_count;
	uint32_t option[MOTION_SENSOR_OPTION_COUNT];
	uint32_t option_mask;
};

enum sensor_opt {
	SENSOR_OPT_VARIANT,
	SENSOR_OPT_CPI,
	SENSOR_OPT_DOWNSHIFT_RUN,
	SENSOR_OPT_DOWNSHIFT_REST1,
	SENSOR_OPT_DOWNSHIFT_REST2,

	SENSOR_OPT_COUNT
};

static K_SEM_DEFINE(sem, 1, 1);
static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;

static const struct device *sensor_dev =
	DEVICE_DT_GET_ONE(MOTION_SENSOR_COMPATIBLE);

static struct sensor_state state;

static const char * const opt_descr[] = {
	[SENSOR_OPT_VARIANT] = OPT_DESCR_MODULE_VARIANT,
	[SENSOR_OPT_CPI] = "cpi",
	[SENSOR_OPT_DOWNSHIFT_RUN] = "downshift",
	[SENSOR_OPT_DOWNSHIFT_REST1] = "rest1",
	[SENSOR_OPT_DOWNSHIFT_REST2] = "rest2"
};


static enum motion_sensor_option config_opt_id_2_option(uint8_t config_opt_id)
{
	switch (config_opt_id) {
	case SENSOR_OPT_CPI:
		return MOTION_SENSOR_OPTION_CPI;

	case SENSOR_OPT_DOWNSHIFT_RUN:
		return MOTION_SENSOR_OPTION_SLEEP1_TIMEOUT;

	case SENSOR_OPT_DOWNSHIFT_REST1:
		return MOTION_SENSOR_OPTION_SLEEP2_TIMEOUT;

	case SENSOR_OPT_DOWNSHIFT_REST2:
		return MOTION_SENSOR_OPTION_SLEEP3_TIMEOUT;

	default:
		LOG_WRN("Unsupported sensor option (%" PRIu8 ")",
			config_opt_id);
		return MOTION_SENSOR_OPTION_COUNT;
	}
}

static bool set_option(enum motion_sensor_option option, uint32_t value)
{
	if (motion_sensor_option_attr[option] != -ENOTSUP) {
		k_spinlock_key_t key = k_spin_lock(&state.lock);
		state.option[option] = value;
		WRITE_BIT(state.option_mask, option, true);
		k_spin_unlock(&state.lock, key);
		k_sem_give(&sem);

		return true;
	}

	LOG_INF("Sensor option %d is not supported", option);

	return false;
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	BUILD_ASSERT(SENSOR_OPT_VARIANT == 0);

	for (size_t i = (SENSOR_OPT_VARIANT + 1); i < ARRAY_SIZE(opt_descr); i++) {
		if (!strcmp(key, opt_descr[i])) {
			uint32_t readout;

			BUILD_ASSERT(sizeof(readout) ==
				     sizeof(state.option[i]));

			ssize_t len = read_cb(cb_arg, &readout,
					      sizeof(readout));

			if ((len != sizeof(readout)) || (len != len_rd)) {
				LOG_ERR("Can't read option %s from storage",
					opt_descr[i]);
				return len;
			}

			set_option(config_opt_id_2_option(i), readout);
			break;
		}
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(motion_sensor, MODULE_NAME, NULL, settings_set,
			       NULL, NULL);

static void data_ready_handler(const struct device *dev, const struct sensor_trigger *trig);


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

static void data_ready_handler(const struct device *dev, const struct sensor_trigger *trig)
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
		if (IS_ENABLED(CONFIG_DESKTOP_MOTION_PM_EVENTS)) {
			/* Wake up system - this will wake up thread */
			APP_EVENT_SUBMIT(new_wake_up_event());
			break;
		}
		/* Fall-through */

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
	APP_EVENT_SUBMIT(event);

	return err;
}

static void set_sampling_time_in_sleep3(bool connected)
{
	if (CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_SAMPLE_TIME_DEFAULT ==
	    CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_SAMPLE_TIME_CONNECTED) {
		return;
	}

	uint32_t sampling_time = (connected) ?
		(CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_SAMPLE_TIME_CONNECTED) :
		(CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_SAMPLE_TIME_DEFAULT);

	set_option(MOTION_SENSOR_OPTION_SLEEP3_SAMPLE_TIME, sampling_time);
}

static void set_default_configuration(void)
{
	BUILD_ASSERT((MOTION_SENSOR_OPTION_COUNT < 8 *
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

	if (CONFIG_DESKTOP_MOTION_SENSOR_SLEEP1_SAMPLE_TIME_DEFAULT) {
		set_option(MOTION_SENSOR_OPTION_SLEEP1_SAMPLE_TIME,
		       CONFIG_DESKTOP_MOTION_SENSOR_SLEEP1_SAMPLE_TIME_DEFAULT);
	}

	if (CONFIG_DESKTOP_MOTION_SENSOR_SLEEP2_SAMPLE_TIME_DEFAULT) {
		set_option(MOTION_SENSOR_OPTION_SLEEP2_SAMPLE_TIME,
		       CONFIG_DESKTOP_MOTION_SENSOR_SLEEP2_SAMPLE_TIME_DEFAULT);
	}

	if (CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_SAMPLE_TIME_DEFAULT) {
		set_option(MOTION_SENSOR_OPTION_SLEEP3_SAMPLE_TIME,
		       CONFIG_DESKTOP_MOTION_SENSOR_SLEEP3_SAMPLE_TIME_DEFAULT);
	}

	set_option(MOTION_SENSOR_OPTION_SLEEP_ENABLE, true);
}

static int init(void)
{
	int err;

	if (!device_is_ready(sensor_dev)) {
		LOG_ERR("Sensor device not ready");
		return -ENODEV;
	}

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
			k_sleep(K_MSEC(1));
		}
	} while (err == -EBUSY);

	if (err) {
		LOG_ERR("Cannot enable trigger");
	}

	return err;
}

static void fetch_config(const uint8_t opt_id, uint8_t *data, size_t *size)
{
	if (opt_id == SENSOR_OPT_VARIANT) {
		*size = strlen(CONFIG_DESKTOP_MOTION_SENSOR_TYPE);
		__ASSERT_NO_MSG((*size != 0) &&
				(*size < CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE));
		strcpy((char *)data, CONFIG_DESKTOP_MOTION_SENSOR_TYPE);
	} else {
		enum motion_sensor_option option = config_opt_id_2_option(opt_id);

		if (option < MOTION_SENSOR_OPTION_COUNT) {
			k_spinlock_key_t key = k_spin_lock(&state.lock);
			sys_put_le32(state.option[option], data);
			k_spin_unlock(&state.lock, key);
			*size = sizeof(state.option[option]);
		} else {
			LOG_WRN("Unsupported fetch opt_id: %" PRIu8, opt_id);
		}
	}
}

static void store_config(uint8_t opt_id, const uint8_t *data, size_t data_size)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		char key[MAX_KEY_LEN];

		int err = snprintk(key, sizeof(key), MODULE_NAME "/%s",
				   opt_descr[opt_id]);

		if ((err > 0) && (err < MAX_KEY_LEN)) {
			err = settings_save_one(key, data, data_size);
		}

		if (err) {
			LOG_ERR("Problem storing %s (err = %d)",
				opt_descr[opt_id], err);
		}
	}
}

static void update_config(const uint8_t opt_id, const uint8_t *data,
			  const size_t size)
{
	enum motion_sensor_option option = config_opt_id_2_option(opt_id);

	if (option < MOTION_SENSOR_OPTION_COUNT) {
		if (size != sizeof(state.option[option])) {
			LOG_WRN("Invalid option size (%zu)", size);
			return;
		}

		uint32_t value = sys_get_le32(data);

		if (set_option(option, value)) {
			store_config(opt_id, data, size);
			LOG_INF("Option: %s set to %" PRIu32, opt_descr[opt_id], value);
		}
	} else {
		LOG_WRN("Unsupported set opt_id: %" PRIu8, opt_id);
	}
}

static void write_config(void)
{
	uint32_t option[MOTION_SENSOR_OPTION_COUNT];
	uint32_t mask;

	BUILD_ASSERT(sizeof(option) == sizeof(state.option), "");

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
		uint32_t option_bm;

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

static bool handle_usb_state_event(const struct usb_state_event *event)
{
	switch (event->state) {
	case USB_STATE_POWERED:
		set_option(MOTION_SENSOR_OPTION_SLEEP_ENABLE, false);
		break;

	case USB_STATE_DISCONNECTED:
		set_option(MOTION_SENSOR_OPTION_SLEEP_ENABLE, true);
		break;

	default:
		break;
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_hid_report_sent_event(aeh)) {
		const struct hid_report_sent_event *event =
			cast_hid_report_sent_event(aeh);

		if ((event->report_id == REPORT_ID_MOUSE) ||
		    (event->report_id == REPORT_ID_BOOT_MOUSE)) {
			k_spinlock_key_t key = k_spin_lock(&state.lock);
			if (state.state == STATE_FETCHING) {
				state.sample = true;
				k_sem_give(&sem);
			}
			k_spin_unlock(&state.lock, key);
		}

		return false;
	}

	if (is_hid_report_subscription_event(aeh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(aeh);

		if ((event->report_id == REPORT_ID_MOUSE) ||
		    (event->report_id == REPORT_ID_BOOT_MOUSE)) {
			if (event->enabled) {
				__ASSERT_NO_MSG(state.peer_count < UCHAR_MAX);
				state.peer_count++;
			} else {
				__ASSERT_NO_MSG(state.peer_count > 0);
				state.peer_count--;
			}

			bool is_connected = (state.peer_count != 0);

			set_sampling_time_in_sleep3(is_connected);

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

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			/* Start state machine thread */
			__ASSERT_NO_MSG(state.state == STATE_DISABLED);

			set_default_configuration();

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

	if (IS_ENABLED(CONFIG_DESKTOP_MOTION_PM_EVENTS) &&
	    is_wake_up_event(aeh)) {
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

	if (IS_ENABLED(CONFIG_DESKTOP_MOTION_PM_EVENTS) &&
	    is_power_down_event(aeh)) {
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

	if (IS_ENABLED(CONFIG_DESKTOP_MOTION_SENSOR_SLEEP_DISABLE_ON_USB) &&
	    is_usb_state_event(aeh)) {
		return handle_usb_state_event(cast_usb_state_event(aeh));
	}

	GEN_CONFIG_EVENT_HANDLERS(STRINGIFY(MODULE), opt_descr, update_config,
				  fetch_config);

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
#if CONFIG_DESKTOP_MOTION_SENSOR_SLEEP_DISABLE_ON_USB
APP_EVENT_SUBSCRIBE(MODULE, usb_state_event);
#endif
#if CONFIG_DESKTOP_MOTION_PM_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
#endif
