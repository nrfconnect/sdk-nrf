/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>

#include <zephyr/types.h>

#include <soc.h>
#include <device.h>
#include <sensor.h>
#include <gpio.h>

#include "event_manager.h"
#include "wheel_event.h"
#include "power_event.h"

#define MODULE wheel
#include "module_state_event.h"

#include <logging/log.h>
#define LOG_LEVEL CONFIG_DESKTOP_LOG_WHEEL_MODULE_LEVEL
LOG_MODULE_REGISTER(MODULE);

static const u32_t qdec_pin[] = {
	DT_NORDIC_NRF_QDEC_QDEC_0_A_PIN,
	DT_NORDIC_NRF_QDEC_QDEC_0_B_PIN
};

static const struct sensor_trigger qdec_trig = {
	.type = SENSOR_TRIG_DATA_READY,
	.chan = SENSOR_CHAN_ROTATION,
};

static struct device *qdec_dev;
static struct device *gpio_dev;
static struct gpio_callback gpio_cbs[2];
static atomic_t active;


static void data_ready_handler(struct device *dev, struct sensor_trigger *trig)
{
	if (!atomic_get(&active)) {
		/* Module is in power down state */
		return;
	}

	struct sensor_value value;

	int err = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &value);
	if (err) {
		LOG_ERR("cannot get sensor value");
		return;
	}

	struct wheel_event *event = new_wheel_event();

	s32_t wheel = value.val1;

	if (!IS_ENABLED(CONFIG_DESKTOP_WHEEL_INVERT_AXIS)) {
		wheel *= -1;
	}

	static_assert(CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER > 0,
		      "Divider must be non-negative");
	if (CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER > 1) {
		wheel /= CONFIG_DESKTOP_WHEEL_SENSOR_VALUE_DIVIDER;
	}

	event->wheel = max(min(wheel, SCHAR_MAX), SCHAR_MIN);

	EVENT_SUBMIT(event);
}

static int wakeup_int_ctrl(bool enable)
{
	int err = 0;

	/* This must be done with irqs disabled to avoid pin callback
	 * being fired before others are still not set up.
	 */
	unsigned int flags = irq_lock();

	for (size_t i = 0; (i < ARRAY_SIZE(qdec_pin)) && !err; i++) {
		if (enable) {
			err = gpio_pin_enable_callback(gpio_dev, qdec_pin[i]);
		} else {
			err = gpio_pin_disable_callback(gpio_dev, qdec_pin[i]);
		}

		if (err) {
			LOG_ERR("cannot control cb (pin:%zu)", i);
		}
	}

	irq_unlock(flags);

	return err;
}

static void wakeup_cb(struct device *gpio_dev, struct gpio_callback *cb,
		      u32_t pins)
{
	__ASSERT_NO_MSG(!atomic_get(&active));

	int err = wakeup_int_ctrl(false);
	if (!err) {
		struct wake_up_event *event = new_wake_up_event();
		EVENT_SUBMIT(event);
	}
}

static int setup_wakeup(void)
{
	int err = gpio_pin_configure(gpio_dev, DT_NORDIC_NRF_QDEC_QDEC_0_ENABLE_PIN,
				     GPIO_DIR_OUT);
	if (err) {
		LOG_ERR("cannot configure enable pin");
		goto error;
	}

	err = gpio_pin_write(gpio_dev, DT_NORDIC_NRF_QDEC_QDEC_0_ENABLE_PIN, 0);
	if (err) {
		LOG_ERR("failed to set enable pin");
		goto error;
	}

	for (size_t i = 0; i < ARRAY_SIZE(qdec_pin); i++) {

		u32_t val;
		err = gpio_pin_read(gpio_dev, qdec_pin[i], &val);
		if (err) {
			LOG_ERR("cannot read pin %zu", i);
			goto error;
		}

		int flags = GPIO_DIR_IN | GPIO_INT | GPIO_INT_LEVEL;
		flags |= (val) ? GPIO_INT_ACTIVE_LOW : GPIO_INT_ACTIVE_HIGH;

		err = gpio_pin_configure(gpio_dev, qdec_pin[i], flags);
		if (err) {
			LOG_ERR("cannot configure pin %zu", i);
			goto error;
		}
	}

	err = wakeup_int_ctrl(true);

error:
	return err;
}

static int init(void)
{
	qdec_dev = device_get_binding(DT_NORDIC_NRF_QDEC_QDEC_0_LABEL);
	if (!qdec_dev) {
		LOG_ERR("cannot get qdec device");
		return -ENXIO;
	}

	gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		LOG_ERR("cannot get gpio device");
		return -ENXIO;
	}

	static_assert(ARRAY_SIZE(qdec_pin) == ARRAY_SIZE(gpio_cbs),
		      "Invalid array size");
	int err = 0;

	for (size_t i = 0; (i < ARRAY_SIZE(qdec_pin)) && !err; i++) {
		gpio_init_callback(&gpio_cbs[i], wakeup_cb, BIT(qdec_pin[i]));
		err = gpio_add_callback(gpio_dev, &gpio_cbs[i]);
		if (err) {
			LOG_ERR("cannot configure cb (pin:%zu)", i);
		}
	}

	return err;
}

static int enable(void)
{
	int err = device_set_power_state(qdec_dev, DEVICE_PM_ACTIVE_STATE);
	if (err) {
		LOG_ERR("cannot activate qdec");
		return err;
	}

	err = sensor_trigger_set(qdec_dev, (struct sensor_trigger *)&qdec_trig,
				 data_ready_handler);
	if (err) {
		LOG_ERR("cannot setup trigger");
	}

	return err;
}

static int disable(void)
{
	int err = sensor_trigger_set(qdec_dev,
				     (struct sensor_trigger *)&qdec_trig, NULL);
	if (err) {
		LOG_ERR("cannot disable trigger");
		return err;
	}

	err = device_set_power_state(qdec_dev, DEVICE_PM_SUSPEND_STATE);
	if (err) {
		LOG_ERR("cannot suspend qdec");
	}

	return err;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			int err = init();
			if (!err) {
				err = enable();
			}
			if (!err) {
				atomic_set(&active, true);
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!atomic_get(&active)) {
			int err = wakeup_int_ctrl(false);
			if (!err) {
				err = enable();
			}

			if (!err) {
				atomic_set(&active, true);
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (atomic_get(&active)) {
			atomic_set(&active, false);

			int err = disable();
			if (!err) {
				err = setup_wakeup();
			}

			if (!err) {
				module_set_state(MODULE_STATE_STANDBY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
