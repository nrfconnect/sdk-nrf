/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/spinlock.h>

#include <app_event_manager.h>
#include <caf/events/power_event.h>
#include "battery_event.h"
#include "usb_event.h"

#define MODULE battery_charger
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BATTERY_CHARGER_LOG_LEVEL);


#define BATTERY_CHARGER		DT_NODELABEL(battery_charger)

#define CSO_PERIOD_HZ		DT_PROP(BATTERY_CHARGER, cso_switching_freq)
#define CSO_CHANGE_MAX		3

#define ERROR_CHECK_TIMEOUT_MS	(1 + CSO_CHANGE_MAX * (1000 / CSO_PERIOD_HZ))

static const struct gpio_dt_spec enable_gpio =
	GPIO_DT_SPEC_GET(BATTERY_CHARGER, enable_gpios);
static const struct gpio_dt_spec cso_gpio =
	GPIO_DT_SPEC_GET(BATTERY_CHARGER, cso_gpios);
static struct gpio_callback gpio_cb;

static struct k_work_delayable error_check;
static unsigned int cso_counter;
static struct k_spinlock lock;

static enum battery_state battery_state = -1;
static atomic_t active;

static void error_check_handler(struct k_work *work)
{
	unsigned int cnt;

	k_spinlock_key_t key = k_spin_lock(&lock);
	cnt = cso_counter;
	cso_counter = 0;
	k_spin_unlock(&lock, key);

	enum battery_state state;

	if (cnt <= CSO_CHANGE_MAX) {
		int val = gpio_pin_get_dt(&cso_gpio);
		if (val < 0) {
			LOG_ERR("Cannot get CSO pin value");
			state = BATTERY_STATE_ERROR;
		} else {
			state = (val == 0) ? (BATTERY_STATE_CHARGING) :
					     (BATTERY_STATE_IDLE);
		}
	} else {
		int err = gpio_pin_interrupt_configure_dt(&cso_gpio,
							  GPIO_INT_DISABLE);
		if (err) {
			LOG_ERR("Cannot disable CSO pin interrupt");
		}

		state = BATTERY_STATE_ERROR;
	}

	if (state != battery_state) {
		battery_state = state;

		struct battery_state_event *event = new_battery_state_event();
		event->state = battery_state;

		APP_EVENT_SUBMIT(event);
	}
}


static void cs_change(const struct device *gpio_dev, struct gpio_callback *cb,
	       uint32_t pins)
{
	if (!atomic_get(&active)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	if (cso_counter == 0) {
		k_work_reschedule(&error_check,
				      K_MSEC(ERROR_CHECK_TIMEOUT_MS));
	}
	cso_counter++;
	k_spin_unlock(&lock, key);
}

static int cso_pin_control(bool enable)
{
	gpio_flags_t flags = cso_gpio.dt_flags | GPIO_INPUT;

	/* clear pull-up/down flags if not enabling */
	if (!enable) {
		flags &= ~(GPIO_PULL_DOWN | GPIO_PULL_UP);
	}

	int err = gpio_pin_configure(cso_gpio.port, cso_gpio.pin, flags);

	if (err) {
		LOG_ERR("Cannot configure CSO pin");
	}

	return err;
}

static int charging_switch(bool enable)
{
	return gpio_pin_set_dt(&enable_gpio, (int)enable);
}

static int start_battery_state_check(void)
{
	int err = gpio_pin_interrupt_configure_dt(&cso_gpio, GPIO_INT_DISABLE);

	if (!err) {
		/* Lock is not needed as interrupt is disabled */
		cso_counter = 1;
		k_work_reschedule(&error_check,
				      K_MSEC(ERROR_CHECK_TIMEOUT_MS));

		err = gpio_pin_interrupt_configure_dt(&cso_gpio,
						      GPIO_INT_EDGE_BOTH);
	}

	if (err) {
		LOG_ERR("Cannot start battery state check");
	}

	return err;
}

static int init_fn(void)
{
	int err;

	if (!device_is_ready(enable_gpio.port)) {
		LOG_ERR("Enable GPIO controller not ready");
		err = -ENODEV;
		goto error;
	}

	err = gpio_pin_configure_dt(&enable_gpio, GPIO_OUTPUT);
	if (err) {
		goto error;
	}

	if (!device_is_ready(cso_gpio.port)) {
		LOG_ERR("CSO GPIO controller not ready");
		err = -ENODEV;
		goto error;
	}

	err = cso_pin_control(true);
	if (err) {
		goto error;
	}

	gpio_init_callback(&gpio_cb, cs_change, BIT(cso_gpio.pin));
	err = gpio_add_callback(cso_gpio.port, &gpio_cb);
	if (err) {
		goto error;
	}

	k_work_init_delayable(&error_check, error_check_handler);

	err = start_battery_state_check();
	if (err) {
		goto error;
	}
error:
	return err;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			int err = init_fn();

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
				atomic_set(&active, true);
			}
		}

		return false;
	}

	if (is_wake_up_event(aeh)) {
		if (!atomic_get(&active)) {
			atomic_set(&active, true);

			int err = cso_pin_control(true);
			if (!err) {
				err = start_battery_state_check();
			}

			if (!err) {
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	if (is_power_down_event(aeh)) {
		if (atomic_get(&active)) {
			atomic_set(&active, false);

			int err = gpio_pin_interrupt_configure_dt(
					&cso_gpio, GPIO_INT_DISABLE);

			/* Cancel cannot fail if executed from another work's context. */
			(void)k_work_cancel_delayable(&error_check);
			if (!err) {
				err = cso_pin_control(false);
			}

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	if (is_usb_state_event(aeh)) {
		struct usb_state_event *event = cast_usb_state_event(aeh);
		int err;

		switch (event->state) {
		case USB_STATE_SUSPENDED:
		case USB_STATE_DISCONNECTED:
			err = charging_switch(false);
			break;

		case USB_STATE_POWERED:
		case USB_STATE_ACTIVE:
			err = charging_switch(true);
			break;

		default:
			/* Ignore */
			err = 0;
			break;
		}
		if (!err) {
			err = start_battery_state_check();
		}
		if (err) {
			module_set_state(MODULE_STATE_ERROR);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
APP_EVENT_SUBSCRIBE(MODULE, usb_state_event);
