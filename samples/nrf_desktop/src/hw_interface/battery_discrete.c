/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <soc.h>
#include <device.h>
#include <gpio.h>
#include <atomic.h>

#include "event_manager.h"
#include "power_event.h"
#include "battery_event.h"

#define MODULE battery
#include "module_state_event.h"

#include <logging/log.h>
#define LOG_LEVEL CONFIG_DESKTOP_LOG_BATTERY_MODULE_LEVEL
LOG_MODULE_REGISTER(MODULE);


#define CSO_PERIOD_HZ          CONFIG_DESKTOP_BATTERY_CSO_FREQ
#define CSO_CHANGE_MAX         3

#define ERROR_CHECK_TIMEOUT_MS (1 + CSO_CHANGE_MAX * (1000 / CSO_PERIOD_HZ))


static struct device *gpio_dev;
static struct gpio_callback gpio_cb;

static struct k_delayed_work error_check;
static unsigned int cso_counter;

static enum battery_state battery_state = -1;
static atomic_t active;


static void error_check_handler(struct k_work *work)
{
	unsigned int cnt;

	unsigned int key = irq_lock();
	cnt = cso_counter;
	cso_counter = 0;
	irq_unlock(key);

	enum battery_state state;

	if (cnt <= CSO_CHANGE_MAX) {
		u32_t val = 0;
		gpio_pin_read(gpio_dev, CONFIG_DESKTOP_BATTERY_CSO_PIN, &val);
		state = (val == 0) ? (BATTERY_STATE_CHARGING) :
				     (BATTERY_STATE_IDLE);

	} else {
		gpio_pin_disable_callback(gpio_dev,
					  CONFIG_DESKTOP_BATTERY_CSO_PIN);
		state = BATTERY_STATE_ERROR;
	}

	if (state != battery_state) {
		battery_state = state;

		struct battery_state_event *event = new_battery_state_event();
		event->state = battery_state;

		EVENT_SUBMIT(event);
	}
}


static void cs_change(struct device *gpio_dev, struct gpio_callback *cb,
	       u32_t pins)
{
	if (!atomic_get(&active)) {
		return;
	}

	if (cso_counter == 0) {
		k_delayed_work_submit(&error_check, ERROR_CHECK_TIMEOUT_MS);
	}

	cso_counter++;
}

static int init_fn(void)
{
	int err;

	gpio_dev = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		LOG_ERR("cannot get gpio device");
		err = -ENXIO;
		goto error;
	}

	int flags = GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
		    GPIO_INT_DOUBLE_EDGE;

	err = gpio_pin_configure(gpio_dev, CONFIG_DESKTOP_BATTERY_CSO_PIN,
				 flags);
	if (err) {
		goto error;
	}

	gpio_init_callback(&gpio_cb, cs_change,
			   BIT(CONFIG_DESKTOP_BATTERY_CSO_PIN));
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		goto error;
	}


	k_delayed_work_init(&error_check, error_check_handler);

	cso_counter = 1;
	k_delayed_work_submit(&error_check, ERROR_CHECK_TIMEOUT_MS);

	err = gpio_pin_enable_callback(gpio_dev,
				       CONFIG_DESKTOP_BATTERY_CSO_PIN);

error:
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

			int err = init_fn();

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
				atomic_set(&active, true);
			}

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!atomic_get(&active)) {
			atomic_set(&active, true);

			cso_counter = 1;
			k_delayed_work_submit(&error_check,
					      ERROR_CHECK_TIMEOUT_MS);

			int err = gpio_pin_enable_callback(gpio_dev,
					CONFIG_DESKTOP_BATTERY_CSO_PIN);

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (atomic_get(&active)) {
			atomic_set(&active, false);
			gpio_pin_disable_callback(gpio_dev,
					CONFIG_DESKTOP_BATTERY_CSO_PIN);

			module_set_state(MODULE_STATE_STANDBY);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
