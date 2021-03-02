/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

#include <device.h>
#include <drivers/gpio.h>

#include "selector_hw_def.h"

#include "event_manager.h"
#include "selector_event.h"
#include <caf/events/power_event.h>

#define MODULE selector
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_SELECTOR_HW_LOG_LEVEL);

#define DEFAULT_POSITION UCHAR_MAX
#define DEBOUNCE_DELAY K_MSEC(5)


enum state {
	STATE_DISABLED,
	STATE_ACTIVE,
	STATE_OFF,
};

struct selector {
	const struct selector_config *config;
	struct gpio_callback gpio_cb[ARRAY_SIZE(port_map)];
	struct k_delayed_work work;
	uint8_t position;
};

static const struct device *gpio_dev[ARRAY_SIZE(port_map)];
static struct selector selectors[ARRAY_SIZE(selector_config)];
static enum state state;


static void selector_event_send(const struct selector *selector)
{
	__ASSERT_NO_MSG(selector->position < selector->config->pins_size);

	struct selector_event *event = new_selector_event();

	event->selector_id = selector->config->id;
	event->position = selector->position;

	EVENT_SUBMIT(event);
}

static int read_state(struct selector *selector)
{
	const struct gpio_pin *sel_pins = selector->config->pins;
	int value;

	if (selector->config->pins_size == 0) {
		return 0;
	}

	for (size_t i = 0; i < selector->config->pins_size; i++) {
		value = gpio_pin_get_raw(gpio_dev[sel_pins[i].port],
					 sel_pins[i].pin);
		if (value < 0) {
			LOG_ERR("Cannot read value from port:%" PRIu8
				" pin: %" PRIu8, sel_pins[i].port,
				sel_pins[i].pin);
			break;
		}

		if (value) {
			selector->position = i;
			break;
		}
	}

	if (value >= 0) {
		if (value) {
			selector_event_send(selector);
		} else {
			selector->position = DEFAULT_POSITION;
			LOG_WRN("Selector %" PRIu8 " position is unknown",
				selector->config->id);
		}
	}

	return (value < 0) ? value : 0;
}

static int enable_interrupts_lock(struct selector *selector)
{
	const struct gpio_pin *sel_pins = selector->config->pins;
	int err;

	int key = irq_lock();

	for (size_t i = 0; i < selector->config->pins_size; i++) {
		if (i == selector->position) {
			continue;
		}

		err = gpio_pin_interrupt_configure(gpio_dev[sel_pins[i].port],
						   sel_pins[i].pin,
						   GPIO_INT_LEVEL_HIGH);
		if (err) {
			LOG_ERR("Cannot enable callback (err %d)", err);
			break;
		}
	}

	irq_unlock(key);

	return err;
}

static int disable_interrupts_nolock(struct selector *selector)
{
	const struct gpio_pin *sel_pins = selector->config->pins;
	int err;

	for (size_t i = 0; i < selector->config->pins_size; i++) {
		err = gpio_pin_interrupt_configure(gpio_dev[sel_pins[i].port],
						   sel_pins[i].pin,
						   GPIO_INT_DISABLE);

		if (err) {
			LOG_ERR("Cannot disable callback (err %d)", err);
			break;
		}
	}

	return err;
}

static void selector_isr(const struct device *dev, struct gpio_callback *cb,
			 uint32_t pins_mask)
{
	uint8_t port = dev - gpio_dev[0];
	struct selector *sel;

	sel = CONTAINER_OF((uint8_t *)cb - port * sizeof(sel->gpio_cb[0]),
			   struct selector,
			   gpio_cb);
	disable_interrupts_nolock(sel);

	k_delayed_work_submit(&sel->work, DEBOUNCE_DELAY);
}

static int read_state_and_enable_interrupts(struct selector *selector)
{
	int err = read_state(selector);

	if (!err)
		enable_interrupts_lock(selector);

	return err;
}

static void selector_work_fn(struct k_work *w)
{
	struct selector *selector = CONTAINER_OF(w, struct selector, work);

	int err = read_state_and_enable_interrupts(selector);

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}
}

static int configure_callbacks(struct selector *selector)
{
	const struct gpio_pin *sel_pins = selector->config->pins;
	uint32_t bitmask[ARRAY_SIZE(gpio_dev)] = {0};
	int err = 0;

	__ASSERT_NO_MSG(selector->config->pins_size > 0);

	for (size_t i = 0; i < selector->config->pins_size; i++) {
		__ASSERT_NO_MSG(sel_pins[i].port < ARRAY_SIZE(bitmask));
		__ASSERT_NO_MSG(
			sel_pins[i].pin < __CHAR_BIT__ * sizeof(bitmask[0]));

		bitmask[sel_pins[i].port] |= BIT(sel_pins[i].pin);
	}

	for (size_t i = 0; (i < ARRAY_SIZE(gpio_dev)) && !err; i++) {
		if (!gpio_dev[i]) {
			__ASSERT_NO_MSG(bitmask[i] == 0);
			continue;
		}
		gpio_init_callback(&selector->gpio_cb[i], selector_isr,
				   bitmask[i]);
		err = gpio_add_callback(gpio_dev[i], &selector->gpio_cb[i]);
	}

	if (err) {
		LOG_ERR("Cannot add callback (err %d)", err);
	}

	return err;
}

static int configure_pins(struct selector *selector, bool enabled)
{
	const struct gpio_pin *sel_pins = selector->config->pins;
	int err = 0;
	gpio_flags_t flags = GPIO_INPUT;

	if (enabled) {
		flags |= GPIO_PULL_DOWN;
	}

	for (size_t i = 0; (i < selector->config->pins_size) && !err; i++) {
		err = gpio_pin_configure(gpio_dev[sel_pins[i].port],
					 sel_pins[i].pin, flags);
	}

	if (err) {
		LOG_ERR("Cannot configure pin (err %d)", err);
	}

	return err;
}

static int sleep(void)
{
	int err = 0;

	for (size_t i = 0; (i < ARRAY_SIZE(selectors)) && !err; i++) {
		err = disable_interrupts_nolock(&selectors[i]);

		if (!err) {
			k_delayed_work_cancel(&selectors[i].work);
			err = configure_pins(&selectors[i], false);
		}
	}

	return err;
}

static int wake_up(void)
{
	int err = 0;

	for (size_t i = 0; (i < ARRAY_SIZE(selectors)) && !err; i++) {
		err = configure_pins(&selectors[i], true);

		if (!err) {
			err = read_state_and_enable_interrupts(&selectors[i]);
		}
	}

	return err;
}

static int init(void)
{
	int err = 0;

	BUILD_ASSERT(ARRAY_SIZE(selector_config) <= UCHAR_MAX,
			 "Maximum number of selectors is 255");
	BUILD_ASSERT(ARRAY_SIZE(selector_config) > 0,
			 "There is no active selector");

	for (size_t i = 0; i < ARRAY_SIZE(port_map); i++) {
		if (!port_map[i]) {
			continue;
		}

		gpio_dev[i] = device_get_binding(port_map[i]);

		if (!gpio_dev[i]) {
			LOG_ERR("Cannot get GPIO%u dev binding", i);
			return -ENXIO;
		}
	}

	for (size_t i = 0; (i < ARRAY_SIZE(selectors)) && !err; i++) {
		selectors[i].config = selector_config[i];

		k_delayed_work_init(&selectors[i].work, selector_work_fn);

		err = configure_callbacks(&selectors[i]);
	}

	if (!err) {
		err = wake_up();
	}

	return err;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);

			int err = init();

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				state = STATE_ACTIVE;
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (state == STATE_ACTIVE) {
			int err = sleep();

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				state = STATE_OFF;
				module_set_state(MODULE_STATE_OFF);
			}
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (state == STATE_OFF) {
			int err = wake_up();

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				state = STATE_ACTIVE;
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
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
