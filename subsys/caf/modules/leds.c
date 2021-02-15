/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <assert.h>
#include <drivers/pwm.h>

#include <caf/events/power_event.h>
#include <caf/events/led_event.h>

#include CONFIG_CAF_LEDS_DEF_PATH

#define MODULE leds
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_LEDS_LOG_LEVEL);

#define LED_ID(led) ((led) - &leds[0])

struct led {
	const struct device *pwm_dev;

	struct led_color color;
	const struct led_effect *effect;
	uint16_t effect_step;
	uint16_t effect_substep;

	struct k_delayed_work work;
};

static struct led leds[CONFIG_CAF_LEDS_COUNT];


static void pwm_out(struct led *led, struct led_color *color)
{
	for (size_t i = 0; i < ARRAY_SIZE(color->c); i++) {
		int err = pwm_pin_set_usec(led->pwm_dev,
					   led_pins[LED_ID(led)][i],
					   CONFIG_CAF_LEDS_BRIGHTNESS_MAX,
					   color->c[i],
					   0);

		if (err) {
			LOG_ERR("Cannot set PWM output (err: %d)", err);
		}
	}
}

static void pwm_off(struct led *led)
{
	struct led_color nocolor = {0};

	pwm_out(led, &nocolor);
}

static void work_handler(struct k_work *work)
{
	struct led *led = CONTAINER_OF(work, struct led, work);

	const struct led_effect_step *effect_step =
		&led->effect->steps[led->effect_step];

	__ASSERT_NO_MSG(effect_step->substep_count > 0);
	int substeps_left = effect_step->substep_count - led->effect_substep;

	for (size_t i = 0; i < ARRAY_SIZE(led->color.c); i++) {
		int diff = (effect_step->color.c[i] - led->color.c[i]) /
			substeps_left;
		led->color.c[i] += diff;
	}
	pwm_out(led, &led->color);

	led->effect_substep++;
	if (led->effect_substep == effect_step->substep_count) {
		led->effect_substep = 0;
		led->effect_step++;

		if (led->effect_step == led->effect->step_count) {
			if (led->effect->loop_forever) {
				led->effect_step = 0;
			} else {
				struct led_ready_event *ready_event = new_led_ready_event();

				ready_event->led_id = LED_ID(led);
				ready_event->led_effect = led->effect;

				EVENT_SUBMIT(ready_event);
			}
		}
	}

	if (led->effect_step < led->effect->step_count) {
		int32_t next_delay =
			led->effect->steps[led->effect_step].substep_time;

		k_delayed_work_submit(&led->work, K_MSEC(next_delay));
	}
}

static void led_update(struct led *led)
{
	k_delayed_work_cancel(&led->work);

	led->effect_step = 0;
	led->effect_substep = 0;

	if (!led->effect) {
		LOG_WRN("No effect set");
		return;
	}

	__ASSERT_NO_MSG(led->effect->steps);

	if (led->effect->step_count > 0) {
		int32_t next_delay =
			led->effect->steps[led->effect_step].substep_time;

		k_delayed_work_submit(&led->work, K_MSEC(next_delay));
	} else {
		LOG_WRN("LED effect with no effect");
	}
}

static int leds_init(void)
{
	static const char * const dev_name[] = {
#if defined(CONFIG_PWM) && DT_NODE_HAS_STATUS(DT_NODELABEL(pwm0), okay)
		DT_LABEL(DT_NODELABEL(pwm0)),
#endif
#if defined(CONFIG_PWM) && DT_NODE_HAS_STATUS(DT_NODELABEL(pwm1), okay)
		DT_LABEL(DT_NODELABEL(pwm1)),
#endif
#if defined(CONFIG_PWM) && DT_NODE_HAS_STATUS(DT_NODELABEL(pwm2), okay)
		DT_LABEL(DT_NODELABEL(pwm2)),
#endif
#if defined(CONFIG_PWM) && DT_NODE_HAS_STATUS(DT_NODELABEL(pwm3), okay)
		DT_LABEL(DT_NODELABEL(pwm3)),
#endif
	};

	int err = 0;

	BUILD_ASSERT(ARRAY_SIZE(leds) <= ARRAY_SIZE(dev_name),
			 "not enough PWMs");

	for (size_t i = 0; (i < ARRAY_SIZE(leds)) && !err; i++) {
		leds[i].pwm_dev = device_get_binding(dev_name[i]);

		if (!leds[i].pwm_dev) {
			LOG_ERR("Cannot bind %s", dev_name[i]);
			err = -ENXIO;
		} else {
			k_delayed_work_init(&leds[i].work, work_handler);
			led_update(&leds[i]);
		}
	}

	return err;
}

static void leds_start(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		int err = device_set_power_state(leds[i].pwm_dev,
						 DEVICE_PM_ACTIVE_STATE,
						 NULL, NULL);
		if (err) {
			LOG_ERR("PWM enable failed");
		}
		led_update(&leds[i]);
	}
}

static void leds_stop(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		k_delayed_work_cancel(&leds[i].work);

		pwm_off(&leds[i]);

		int err = device_set_power_state(leds[i].pwm_dev,
						 DEVICE_PM_SUSPEND_STATE,
						 NULL, NULL);
		if (err) {
			LOG_ERR("PWM disable failed");
		}
	}
}

static bool event_handler(const struct event_header *eh)
{
	static bool initialized;

	if (is_led_event(eh)) {
		const struct led_event *event = cast_led_event(eh);

		__ASSERT_NO_MSG(event->led_id < CONFIG_CAF_LEDS_COUNT);

		struct led *led = &leds[event->led_id];

		led->effect = event->led_effect;

		if (initialized) {
			led_update(led);
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

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

	if (IS_ENABLED(CONFIG_CAF_LEDS_PM_EVENTS) &&
	    is_wake_up_event(eh)) {
		if (!initialized) {
			leds_start();
			initialized = true;
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_LEDS_PM_EVENTS) &&
	    is_power_down_event(eh)) {
		const struct power_down_event *event =
			cast_power_down_event(eh);

		/* Leds should keep working on system error. */
		if (event->error) {
			return false;
		}

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
#if CONFIG_CAF_LEDS_PM_EVENTS
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif
