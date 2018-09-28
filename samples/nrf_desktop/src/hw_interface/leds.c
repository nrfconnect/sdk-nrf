/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <assert.h>
#include <misc/util.h>
#include <pwm.h>

#include "power_event.h"
#include "led_event.h"

#define MODULE leds
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_LED_MODULE_LEVEL
#include <logging/sys_log.h>


struct led {
	struct device *pwm_dev;

	enum led_mode mode;
	struct led_color color;

	u16_t period;
	bool dir;

	struct k_delayed_work work;
};


static struct led leds[CONFIG_DESKTOP_LED_COUNT];

static void pwm_out(struct device *pwm_dev, struct led_color *color)
{
	for (size_t i = 0; i < ARRAY_SIZE(color->c); i++) {
		pwm_pin_set_usec(pwm_dev, i, CONFIG_DESKTOP_LED_BRIGHTNESS_MAX,
				 color->c[i]);
	}
}

static void pwm_off(struct device *pwm_dev)
{
	struct led_color nocolor = {0};

	pwm_out(pwm_dev, &nocolor);
}

static void led_blink(struct led *led)
{
	if (led->dir) {
		pwm_off(led->pwm_dev);
	} else {
		pwm_out(led->pwm_dev, &led->color);
	}
	led->dir = !led->dir;

	k_delayed_work_submit(&led->work, led->period);
}

static void work_handler(struct k_work *work)
{
	struct led *led = CONTAINER_OF(work, struct led, work);

	switch (led->mode) {
	case LED_MODE_BLINKING:
		led_blink(led);
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}
}

static void led_mode_update(struct led *led, enum led_mode mode)
{
	k_delayed_work_cancel(&led->work);

	switch (mode) {
	case LED_MODE_ON:
		pwm_out(led->pwm_dev, &led->color);
		break;

	case LED_MODE_OFF:
		pwm_off(led->pwm_dev);
		break;

	case LED_MODE_BLINKING:
		__ASSERT_NO_MSG(led->period > 0);
		k_delayed_work_submit(&led->work, 0);
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}
}

static int leds_init(void)
{
	const char *dev_name[] = {
#if CONFIG_PWM_0
		CONFIG_PWM_0_NAME,
#endif
#if CONFIG_PWM_1
		CONFIG_PWM_1_NAME,
#endif
#if CONFIG_PWM_2
		CONFIG_PWM_2_NAME,
#endif
#if CONFIG_PWM_3
		CONFIG_PWM_4_NAME,
#endif
	};

	int err = 0;

	static_assert(ARRAY_SIZE(leds) <= ARRAY_SIZE(dev_name),
		      "not enough PWMs");

	for (size_t i = 0; (i < ARRAY_SIZE(leds)) && !err; i++) {
		leds[i].pwm_dev = device_get_binding(dev_name[i]);
		if (!leds[i].pwm_dev) {
			SYS_LOG_ERR("cannot bind %s\n", dev_name[i]);
			err = -ENXIO;
		} else {
			k_delayed_work_init(&leds[i].work, work_handler);
			led_mode_update(&leds[i], leds[i].mode);
		}
	}

	return err;
}

static void leds_start(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		led_mode_update(&leds[i], leds[i].mode);
	}
}

static void leds_stop(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		led_mode_update(&leds[i], LED_MODE_OFF);
	}
}

static bool event_handler(const struct event_header *eh)
{
	static bool initialized;

	if (is_led_event(eh)) {
		struct led_event *event = cast_led_event(eh);

		__ASSERT_NO_MSG(event->led_id < CONFIG_DESKTOP_LED_COUNT);

		/* Record params */
		struct led *led = &leds[event->led_id];

		led->color = event->color;
		led->mode = event->mode;
		led->period = event->period;

		/* Update mode */
		if (initialized) {
			led_mode_update(led, led->mode);
		}
		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			int err = leds_init();
			if (!err) {
				initialized = true;
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}
		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!initialized) {
			leds_start();
			initialized = true;
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (initialized) {
			leds_stop();
			initialized = false;
			module_set_state(MODULE_STATE_OFF);
		}

		return initialized;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, led_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
