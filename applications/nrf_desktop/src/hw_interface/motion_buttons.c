/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <soc.h>
#include <device.h>
#include <gpio.h>

#include "event_manager.h"
#include "motion_event.h"
#include "power_event.h"

#define MODULE motion
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_MOTION_LOG_LEVEL);

#define BUTTONS_NUM	4
#define MOVEMENT_SPEED	5

static struct device       *gpio_devs[BUTTONS_NUM];
static struct gpio_callback gpio_cbs[BUTTONS_NUM];
static const  u32_t         pin_id[BUTTONS_NUM] = {
	DT_ALIAS_SW0_GPIOS_PIN, DT_ALIAS_SW1_GPIOS_PIN,
	DT_ALIAS_SW2_GPIOS_PIN, DT_ALIAS_SW3_GPIOS_PIN
};

enum {
	IDLE_STATE,
	ACTIVE_STATE,
	TERMINATING_STATE,
	STATES_NUM
};
static atomic_t state;

static void motion_event_send(s8_t dx, s8_t dy)
{
	struct motion_event *event = new_motion_event();
	event->dx = dx;
	event->dy = dy;
	EVENT_SUBMIT(event);
}


void button_pressed(struct device *gpio_dev, struct gpio_callback *cb,
		    u32_t pins)
{
	s8_t val_x = 0;
	s8_t val_y = 0;

	if (pins & (1 << DT_ALIAS_SW0_GPIOS_PIN)) {
		val_x -= MOVEMENT_SPEED;
		LOG_DBG("Left");
	}
	if (pins & (1 << DT_ALIAS_SW1_GPIOS_PIN)) {
		val_y += MOVEMENT_SPEED;
		LOG_DBG("Up");
	}
	if (pins & (1 << DT_ALIAS_SW2_GPIOS_PIN)) {
		val_x += MOVEMENT_SPEED;
		LOG_DBG("Right");
	}
	if (pins & (1 << DT_ALIAS_SW3_GPIOS_PIN)) {
		val_y -= MOVEMENT_SPEED;
		LOG_DBG("Down");
	}

	motion_event_send(val_x, val_y);
}


static void async_init_fn(struct k_work *work)
{
	static const char *port_name[BUTTONS_NUM] = {
		DT_ALIAS_SW0_GPIOS_CONTROLLER,
		DT_ALIAS_SW1_GPIOS_CONTROLLER,
		DT_ALIAS_SW2_GPIOS_CONTROLLER,
		DT_ALIAS_SW3_GPIOS_CONTROLLER
	};

	for (size_t i = 0; i < ARRAY_SIZE(pin_id); i++) {
		gpio_devs[i] = device_get_binding(port_name[i]);
		if (gpio_devs[i]) {
			LOG_DBG("Port %zu bound", i);

			gpio_pin_configure(gpio_devs[i], pin_id[i],
					   GPIO_PUD_PULL_UP | GPIO_DIR_IN |
					   GPIO_INT | GPIO_INT_EDGE |
					   GPIO_INT_ACTIVE_LOW);
			gpio_init_callback(&gpio_cbs[i], button_pressed,
					   BIT(pin_id[i]));
			gpio_add_callback(gpio_devs[i], &gpio_cbs[i]);
			gpio_pin_enable_callback(gpio_devs[i], pin_id[i]);
		}
	}

	module_set_state(MODULE_STATE_READY);
}
K_WORK_DEFINE(motion_async_init, async_init_fn);

static void async_term_fn(struct k_work *work)
{
	for (size_t i = 0; i < ARRAY_SIZE(gpio_devs); i++) {
		if (gpio_devs[i]) {
			LOG_DBG("Port %zu unbound", i);

			gpio_pin_disable_callback(gpio_devs[i], pin_id[i]);
			gpio_remove_callback(gpio_devs[i], &gpio_cbs[i]);
			gpio_pin_configure(gpio_devs[i], pin_id[i],
					   GPIO_DIR_IN);
		}
	}

	atomic_set(&state, IDLE_STATE);
	module_set_state(MODULE_STATE_OFF);
}
K_WORK_DEFINE(motion_async_term, async_term_fn);

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(board), MODULE_STATE_READY)) {
			if (atomic_cas(&state, IDLE_STATE, ACTIVE_STATE)) {
				k_work_submit(&motion_async_init);
			}
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (atomic_cas(&state, ACTIVE_STATE, TERMINATING_STATE)) {
			k_work_submit(&motion_async_term);
		}

		return (atomic_get(&state) != IDLE_STATE);
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
