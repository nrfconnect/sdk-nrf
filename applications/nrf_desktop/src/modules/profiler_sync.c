/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/gpio.h>
#include <profiler.h>

#include "gpio_pins.h"

#define MODULE profiler_sync
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_PROFILER_SYNC_LOG_LEVEL);

#define SYNC_PERIOD_MIN		1000U
#define SYNC_PERIOD_MAX		2000U
#define SYNC_PERIOD_STEP	100U

#define SYNC_EVENT_NAME		"sync_event"

static const struct device *gpio_dev;
static struct gpio_callback gpio_cb;

static struct k_delayed_work gen_sync_event;
static uint16_t sync_event_id;
static int pin_value;


static void profile_sync_event(void)
{
	struct log_event_buf buf;

	profiler_log_start(&buf);
	profiler_log_send(&buf, sync_event_id);
}

static void register_sync_event(void)
{
	sync_event_id = profiler_register_event_type(SYNC_EVENT_NAME, NULL,
						     NULL, 0);
}

static void gen_sync_event_isr(const struct device *gpio_dev,
			       struct gpio_callback *cb,
			       uint32_t pins)
{
	profile_sync_event();
}

static void gen_sync_event_fn(struct k_work *work)
{
	static uint32_t sync_period = SYNC_PERIOD_MIN;

	sync_period += SYNC_PERIOD_STEP;
	if (sync_period > SYNC_PERIOD_MAX) {
		sync_period = SYNC_PERIOD_MIN;
	}

	pin_value = 1 - pin_value;
	int err = gpio_pin_set_raw(gpio_dev,
			(gpio_pin_t)CONFIG_DESKTOP_PROFILER_SYNC_GPIO_PIN,
			pin_value);

	if (err) {
		LOG_ERR("Cannot set pin");
		module_set_state(MODULE_STATE_ERROR);
	} else {
		profile_sync_event();
		k_delayed_work_submit(&gen_sync_event,
				      K_MSEC(sync_period));
	}
}

static int init(void)
{
	static bool initialized;

	__ASSERT_NO_MSG(!initialized);
	initialized = true;

	register_sync_event();

	BUILD_ASSERT(CONFIG_DESKTOP_PROFILER_SYNC_GPIO_PORT <
		     ARRAY_SIZE(port_map));
	__ASSERT_NO_MSG(port_map[CONFIG_DESKTOP_PROFILER_SYNC_GPIO_PORT]);
	gpio_dev = device_get_binding(
			port_map[CONFIG_DESKTOP_PROFILER_SYNC_GPIO_PORT]);

	if (!gpio_dev) {
		LOG_ERR("Cannot get GPIO device binding");
		return -ENXIO;
	}

	int err;
	gpio_pin_t pin = CONFIG_DESKTOP_PROFILER_SYNC_GPIO_PIN;

	if (IS_ENABLED(CONFIG_DESKTOP_PROFILER_SYNC_CENTRAL)) {
		err = gpio_pin_configure(gpio_dev, pin, GPIO_OUTPUT);

		if (!err) {
			err = gpio_pin_set_raw(gpio_dev, pin, pin_value);
		}

		if (!err) {
			k_delayed_work_init(&gen_sync_event, gen_sync_event_fn);
			k_delayed_work_submit(&gen_sync_event,
					      K_MSEC(SYNC_PERIOD_MIN));
		}
	} else {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_PROFILER_SYNC_PERIPHERAL));
		err = gpio_pin_configure(gpio_dev, pin, GPIO_INPUT);

		if (err) {
			return err;
		}

		gpio_port_pins_t pin_mask = BIT(pin);

		gpio_init_callback(&gpio_cb, gen_sync_event_isr, pin_mask);
		err = gpio_add_callback(gpio_dev, &gpio_cb);

		if (!err) {
			err = gpio_pin_interrupt_configure(gpio_dev, pin,
							   GPIO_INT_EDGE_BOTH);
		}
	}

	return err;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
				cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			if (init()) {
				LOG_ERR("Cannot initialize GPIO");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
