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
	SW0_GPIO_PIN, SW1_GPIO_PIN, SW2_GPIO_PIN, SW3_GPIO_PIN
};

enum {
	IDLE_STATE,
	ACTIVE_STATE,
	TERMINATING_STATE,
	SUSPENDED_STATE,
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

static void pin_irq_disable(void)
{
	int err = 0;
	/* Disable pin IRQs */
	unsigned int flags = irq_lock();

	for (size_t i = 0; (i < BUTTONS_NUM) && !err; i++) {
		err = gpio_pin_disable_callback(gpio_devs[i], pin_id[i]);
		
		if (err) {
			LOG_ERR("Cannot control cb (pin:%zu)", i);
		}
	}
	irq_unlock(flags);
}

void button_pressed(struct device *gpio_dev, struct gpio_callback *cb,
		    u32_t pins)
{
	s8_t val_x = 0;
	s8_t val_y = 0;

	if (atomic_get(&state) == SUSPENDED_STATE) {
		pin_irq_disable();
		EVENT_SUBMIT(new_wake_up_event());
		LOG_INF("--- woke up! Pin pressed: 0x%x", pins);
		return;
	}
	if (pins & (1 << SW0_GPIO_PIN)) {
		val_x -= MOVEMENT_SPEED;
		LOG_DBG("Left");
	}
	if (pins & (1 << SW1_GPIO_PIN)) {
		val_y -= MOVEMENT_SPEED;
		LOG_DBG("Up");
	}
	if (pins & (1 << SW2_GPIO_PIN)) {
		val_x += MOVEMENT_SPEED;
		LOG_DBG("Right");
	}
	if (pins & (1 << SW3_GPIO_PIN)) {
		val_y += MOVEMENT_SPEED;
		LOG_DBG("Down");
	}

	motion_event_send(val_x, val_y);
}

#include <hal/nrf_gpio.h>
static void async_init_fn(struct k_work *work)
{
	static const char *port_name[BUTTONS_NUM] = {
		SW0_GPIO_CONTROLLER,
		SW1_GPIO_CONTROLLER,
		SW2_GPIO_CONTROLLER,
		SW3_GPIO_CONTROLLER
	};
	
	for (size_t i = 0; i < ARRAY_SIZE(pin_id); i++) {
		gpio_devs[i] = device_get_binding(port_name[i]);
		if (gpio_devs[i]) {
			LOG_DBG("Port %zu bound", i);

			gpio_pin_configure(gpio_devs[i], pin_id[i],
					   GPIO_PUD_PULL_UP | GPIO_DIR_IN |
					   GPIO_INT |/* GPIO_INT_EDGE |*/
					   GPIO_INT_ACTIVE_LOW);
			gpio_init_callback(&gpio_cbs[i], button_pressed,
					   BIT(pin_id[i]));
			LOG_INF("--- set up GPIO: %d", pin_id[i]);
			gpio_add_callback(gpio_devs[i], &gpio_cbs[i]);
			gpio_pin_enable_callback(gpio_devs[i], pin_id[i]);
		}
	}
	u32_t p11 = nrf_gpio_pin_read(11);
	if (!p11) LOG_INF("-- Button 1 pressed during config! %d", p11);

	module_set_state(MODULE_STATE_READY);
}
K_WORK_DEFINE(motion_async_init, async_init_fn);

static void async_term_fn(struct k_work *work)
{
	for (size_t i = 0; i < ARRAY_SIZE(gpio_devs); i++) {
		if (gpio_devs[i]) {
			LOG_DBG("Port %zu set up for wakeup", i);
			gpio_pin_configure(gpio_devs[i], pin_id[i],
				GPIO_DIR_IN | GPIO_INT | GPIO_PUD_PULL_UP |
				GPIO_INT_ACTIVE_LOW);
		}
	}

	LOG_INF("--- SUSPENDED_STATE");
	atomic_set(&state, /*IDLE_STATE*/SUSPENDED_STATE);
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
				LOG_INF("--- ACTIVE");
			}
			else if (atomic_cas(&state, SUSPENDED_STATE, ACTIVE_STATE)) {
				LOG_INF("--- ACTIVE from SUSPENDED");
				/* No need to re-initialize */
				module_set_state(MODULE_STATE_READY);
			}
			else {
				/* Do nothing */
			}
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (atomic_cas(&state, ACTIVE_STATE, TERMINATING_STATE)) {
			k_work_submit(&motion_async_term);
			LOG_INF("--- TERMINATING");
		}
		LOG_INF("--- handlig power down event in state: %d", atomic_get(&state));

		return (atomic_get(&state) != SUSPENDED_STATE/*IDLE_STATE*/);
	}
	
	if (is_wake_up_event(eh)) {
		if (atomic_cas(&state, SUSPENDED_STATE, ACTIVE_STATE)) {
			LOG_INF("--- ACTIVE from SUSPENDED");
			/* No need to re-initialize */
			module_set_state(MODULE_STATE_READY);
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
