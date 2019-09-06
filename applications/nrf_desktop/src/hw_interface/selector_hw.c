/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <device.h>
#include <gpio.h>

#include "selector_hw_def.h"

#include "event_manager.h"
#include "selector_event.h"
#include "power_event.h"

#define MODULE selector_hw
#include "module_state_event.h"

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
	u8_t position;
};

static struct device *gpio_dev[ARRAY_SIZE(port_map)];
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
	u32_t value = 0;
	const struct gpio_pin *sel_pins = selector->config->pins;
	int err;

	for (size_t i = 0; i < selector->config->pins_size; i++) {
		err = gpio_pin_read(gpio_dev[sel_pins[i].port],
				    sel_pins[i].pin,
				    &value);

		if (err) {
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

	if (!err) {
		if (value) {
			selector_event_send(selector);
		} else {
			selector->position = DEFAULT_POSITION;
			LOG_WRN("Selector %" PRIu8 " position is unknown",
				selector->config->id);
		}
	}

	return err;
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

		err = gpio_pin_enable_callback(gpio_dev[sel_pins[i].port],
					       sel_pins[i].pin);
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
		err = gpio_pin_disable_callback(gpio_dev[sel_pins[i].port],
						sel_pins[i].pin);

		if (err) {
			LOG_ERR("Cannot disable callback (err %d)", err);
			break;
		}
	}

	return err;
}

static void selector_isr(struct device *dev, struct gpio_callback *cb,
			 u32_t pins_mask)
{
	u8_t port = dev - gpio_dev[0];
	struct selector *sel;

	sel = CONTAINER_OF((u8_t *)cb - port * sizeof(sel->gpio_cb[0]),
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

static int configure(struct selector *selector)
{
	const struct gpio_pin *sel_pins = selector->config->pins;
	u32_t bitmask = 0;
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(gpio_dev); i++) {
		for (size_t j = 0; j < selector->config->pins_size; j++) {
			if (sel_pins[j].port == i) {
				bitmask |= BIT(sel_pins[j].pin);
			}
		}
		gpio_init_callback(&selector->gpio_cb[i], selector_isr,
				   bitmask);
		err = gpio_add_callback(gpio_dev[i], &selector->gpio_cb[i]);

		if (err) {
			LOG_ERR("Cannot add callback (err %d)", err);
			return err;
		}

		bitmask = 0;
	}

	for (size_t i = 0; i < selector->config->pins_size; i++) {
		err = gpio_pin_configure(gpio_dev[sel_pins[i].port],
					 sel_pins[i].pin,
					 GPIO_PUD_PULL_DOWN | GPIO_DIR_IN |
					 GPIO_INT | GPIO_INT_LEVEL |
					 GPIO_INT_ACTIVE_HIGH);

		if (err) {
			LOG_ERR("Cannot configure pin (err %d)", err);
			break;
		}
	}

	k_delayed_work_init(&selector->work, selector_work_fn);

	return err;
}

static int init(void)
{
	BUILD_ASSERT_MSG(ARRAY_SIZE(selector_config) <= UCHAR_MAX,
			 "Maximum number of selectors is 255");
	BUILD_ASSERT_MSG(ARRAY_SIZE(selector_config) > 0,
			 "There is no active selector");
	for (size_t i = 0; i < ARRAY_SIZE(port_map); i++) {
		gpio_dev[i] = device_get_binding(port_map[i]);

		if (!gpio_dev[i]) {
			LOG_ERR("Cannot get GPIO%u dev binding", i);
			return -ENXIO;
		}
	}

	int err;

	for (size_t i = 0; i < ARRAY_SIZE(selectors); i++) {
		selectors[i].config = selector_config[i];
		__ASSERT_NO_MSG(selector_config[i]->pins_size > 0);

		err = configure(&selectors[i]);
		if (err) {
			break;
		}
		err = read_state_and_enable_interrupts(&selectors[i]);
		if (err) {
			break;
		}
	}
	state = STATE_ACTIVE;

	return err;
}

static void sleep(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(selectors); i++) {
		int err = disable_interrupts_nolock(&selectors[i]);

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
			return;
		}

		k_delayed_work_cancel(&selectors[i].work);
	}

	state = STATE_OFF;
	module_set_state(MODULE_STATE_OFF);
}

static void wake_up(void)
{
	for (size_t i = 0; ARRAY_SIZE(selectors); i++) {
		int err = read_state_and_enable_interrupts(&selectors[i]);

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
			return;
		}
	}

	state = STATE_ACTIVE;
	module_set_state(MODULE_STATE_READY);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);
			if (init()) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (state == STATE_ACTIVE) {
			sleep();
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (state == STATE_OFF) {
			wake_up();
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
