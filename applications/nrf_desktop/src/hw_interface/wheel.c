/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>

#include <zephyr/types.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#ifdef CONFIG_PINCTRL
#include <pinctrl_soc.h>
#endif

#include <app_event_manager.h>
#include "wheel_event.h"
#include <caf/events/power_event.h>

#define MODULE wheel
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_WHEEL_LOG_LEVEL);


#define SENSOR_IDLE_TIMEOUT \
	(CONFIG_DESKTOP_WHEEL_SENSOR_IDLE_TIMEOUT * MSEC_PER_SEC) /* ms */


enum state {
	STATE_DISABLED,
	STATE_ACTIVE_IDLE,
	STATE_ACTIVE,
	STATE_SUSPENDED
};

#ifdef CONFIG_PINCTRL
#define QDEC_PIN_INIT(node_id, prop, idx) \
	NRF_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),

/* obtan qdec pins from default state */
static const uint32_t qdec_pin[] = {
	DT_FOREACH_CHILD_VARGS(
		DT_PINCTRL_BY_NAME(DT_NODELABEL(qdec), default, 0),
		DT_FOREACH_PROP_ELEM, psels, QDEC_PIN_INIT
	)
};
#else
static const uint32_t qdec_pin[] = {
	DT_PROP(DT_NODELABEL(qdec), a_pin),
	DT_PROP(DT_NODELABEL(qdec), b_pin)
};
#endif

static const struct sensor_trigger qdec_trig = {
	.type = SENSOR_TRIG_DATA_READY,
	.chan = SENSOR_CHAN_ROTATION,
};

static const struct device *qdec_dev = DEVICE_DT_GET(DT_NODELABEL(qdec));
static const struct device *const gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static struct gpio_callback gpio_cbs[2];
static struct k_spinlock lock;
static struct k_work_delayable idle_timeout;
static bool qdec_triggered;
static enum state state;

static int enable_qdec(enum state next_state);


static void data_ready_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	if (IS_ENABLED(CONFIG_ASSERT)) {
		k_spinlock_key_t key = k_spin_lock(&lock);

		__ASSERT_NO_MSG(state == STATE_ACTIVE);

		k_spin_unlock(&lock, key);
	}

	struct sensor_value value;

	int err = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &value);
	if (err) {
		LOG_ERR("Cannot get sensor value");
		return;
	}

	struct wheel_event *event = new_wheel_event();

	int32_t wheel = value.val1;

	if (!IS_ENABLED(CONFIG_DESKTOP_WHEEL_INVERT_AXIS)) {
		wheel *= -1;
	}

	BUILD_ASSERT(CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER > 0,
			 "Divider must be non-negative");
	if (CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER > 1) {
		wheel /= CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER;
	}

	event->wheel = MAX(MIN(wheel, SCHAR_MAX), SCHAR_MIN);

	APP_EVENT_SUBMIT(event);

	qdec_triggered = true;
}

static int wakeup_int_ctrl_nolock(bool enable)
{
	int err = 0;

	/* This must be done with irqs disabled to avoid pin callback
	 * being fired before others are still not set up.
	 */
	for (size_t i = 0; (i < ARRAY_SIZE(qdec_pin)) && !err; i++) {
		if (enable) {
			int val = gpio_pin_get_raw(gpio_dev, qdec_pin[i]);

			if (val < 0) {
				LOG_ERR("Cannot read pin %zu", i);
				err = val;
				continue;
			}

			err = gpio_pin_interrupt_configure(gpio_dev,
				qdec_pin[i],
				val ? GPIO_INT_LEVEL_LOW : GPIO_INT_LEVEL_HIGH);
		} else {
			err = gpio_pin_interrupt_configure(gpio_dev, qdec_pin[i],
							   GPIO_INT_DISABLE);
		}

		if (err) {
			LOG_ERR("Cannot control cb (pin:%zu)", i);
		}
	}

	return err;
}

static void wakeup_cb(const struct device *gpio_dev, struct gpio_callback *cb,
		      uint32_t pins)
{
	struct wake_up_event *event;
	int err;

	k_spinlock_key_t key = k_spin_lock(&lock);

	err = wakeup_int_ctrl_nolock(false);

	if (!err) {
		switch (state) {
		case STATE_ACTIVE_IDLE:
			err = enable_qdec(STATE_ACTIVE);
			break;

		case STATE_SUSPENDED:
			event = new_wake_up_event();
			APP_EVENT_SUBMIT(event);
			break;

		case STATE_ACTIVE:
		case STATE_DISABLED:
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
	}

	k_spin_unlock(&lock, key);

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}
}

static int setup_wakeup(void)
{
	int err = gpio_pin_configure(gpio_dev,
				     DT_PROP(DT_NODELABEL(qdec), enable_pin),
				     GPIO_OUTPUT);
	if (err) {
		LOG_ERR("Cannot configure enable pin");
		return err;
	}

	err = gpio_pin_set_raw(gpio_dev,
			       DT_PROP(DT_NODELABEL(qdec), enable_pin), 0);
	if (err) {
		LOG_ERR("Failed to set enable pin");
		return err;
	}

	for (size_t i = 0; i < ARRAY_SIZE(qdec_pin); i++) {
		err = gpio_pin_configure(gpio_dev, qdec_pin[i], GPIO_INPUT);

		if (err) {
			LOG_ERR("Cannot configure pin %zu", i);
			return err;
		}
	}

	err = wakeup_int_ctrl_nolock(true);

	return err;
}

static int enable_qdec(enum state next_state)
{
	__ASSERT_NO_MSG(next_state == STATE_ACTIVE);

	int err = 0;

	/* QDEC device driver starts in PM_DEVICE_STATE_ACTIVE state. */
	if (state != STATE_DISABLED) {
		err = pm_device_action_run(qdec_dev, PM_DEVICE_ACTION_RESUME);
	}

	if (err) {
		LOG_ERR("Cannot resume QDEC");
		return err;
	}

	err = sensor_trigger_set(qdec_dev, (struct sensor_trigger *)&qdec_trig,
				 data_ready_handler);
	if (err) {
		LOG_ERR("Cannot setup trigger");
	} else {
		state = next_state;
		if (SENSOR_IDLE_TIMEOUT > 0) {
			qdec_triggered = false;
			k_work_reschedule(&idle_timeout,
					      K_MSEC(SENSOR_IDLE_TIMEOUT));
		}
	}

	return err;
}

static int disable_qdec(enum state next_state)
{
	if (SENSOR_IDLE_TIMEOUT > 0) {
		__ASSERT_NO_MSG((next_state == STATE_ACTIVE_IDLE) ||
				(next_state == STATE_SUSPENDED));
	} else {
		__ASSERT_NO_MSG(next_state == STATE_SUSPENDED);
	}

	int err = sensor_trigger_set(qdec_dev,
				     (struct sensor_trigger *)&qdec_trig, NULL);
	if (err) {
		LOG_ERR("Cannot disable trigger");
		return err;
	}

	err = pm_device_action_run(qdec_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err) {
		LOG_ERR("Cannot suspend QDEC");
	} else {
		err = setup_wakeup();
		if (!err) {
			if (SENSOR_IDLE_TIMEOUT > 0) {
				/* Cancel cannot fail if executed from another work's context. */
				(void)k_work_cancel_delayable(&idle_timeout);
			}
			state = next_state;
		}
	}

	return err;
}

static void idle_timeout_fn(struct k_work *work)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	__ASSERT_NO_MSG(state == STATE_ACTIVE);

	if (!qdec_triggered) {
		int err = disable_qdec(STATE_ACTIVE_IDLE);

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
		}
	} else {
		qdec_triggered = false;
		k_work_reschedule(&idle_timeout,
				      K_MSEC(SENSOR_IDLE_TIMEOUT));
	}

	k_spin_unlock(&lock, key);
}

static int init(void)
{
	__ASSERT_NO_MSG(state == STATE_DISABLED);

	if (SENSOR_IDLE_TIMEOUT > 0) {
		k_work_init_delayable(&idle_timeout, idle_timeout_fn);
	}

	if (!device_is_ready(qdec_dev)) {
		LOG_ERR("QDEC device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(gpio_dev)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	BUILD_ASSERT(ARRAY_SIZE(qdec_pin) == ARRAY_SIZE(gpio_cbs),
		      "Invalid array size");
	int err = 0;

	for (size_t i = 0; (i < ARRAY_SIZE(qdec_pin)) && !err; i++) {
		gpio_init_callback(&gpio_cbs[i], wakeup_cb, BIT(qdec_pin[i]));
		err = gpio_add_callback(gpio_dev, &gpio_cbs[i]);
		if (err) {
			LOG_ERR("Cannot configure cb (pin:%zu)", i);
		}
	}

	return err;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			int err = init();

			if (!err) {
				k_spinlock_key_t key = k_spin_lock(&lock);

				err = enable_qdec(STATE_ACTIVE);

				k_spin_unlock(&lock, key);
			}

			if (!err) {
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}

			return false;
		}

		return false;
	}

	if (is_wake_up_event(aeh)) {
		int err;

		k_spinlock_key_t key = k_spin_lock(&lock);

		switch (state) {
		case STATE_SUSPENDED:
			err = wakeup_int_ctrl_nolock(false);
			if (!err) {
				err = enable_qdec(STATE_ACTIVE);
			}

			if (!err) {
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
			break;

		case STATE_ACTIVE:
		case STATE_ACTIVE_IDLE:
			/* No action */
			break;

		case STATE_DISABLED:
		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		k_spin_unlock(&lock, key);

		return false;
	}

	if (is_power_down_event(aeh)) {
		int err;

		k_spinlock_key_t key = k_spin_lock(&lock);

		switch (state) {
		case STATE_ACTIVE:
			err = disable_qdec(STATE_SUSPENDED);

			if (!err) {
				module_set_state(MODULE_STATE_STANDBY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
			break;

		case STATE_ACTIVE_IDLE:
			state = STATE_SUSPENDED;
			break;

		case STATE_SUSPENDED:
			/* No action */
			break;

		case STATE_DISABLED:
		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		k_spin_unlock(&lock, key);

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
