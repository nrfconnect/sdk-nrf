/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <assert.h>
#include <drivers/led.h>

#include <caf/events/power_event.h>
#include <caf/events/led_event.h>

#define MODULE leds
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_LEDS_LOG_LEVEL);

#define LED_ID(led) ((led) - &leds[0])

struct led {
	const char *label;
	const struct device *dev;
	uint8_t color_count;

	struct led_color color;
	const struct led_effect *effect;
	uint16_t effect_step;
	uint16_t effect_substep;

	struct k_work_delayable work;
};

#ifdef CONFIG_CAF_LEDS_PWM
  #define DT_DRV_COMPAT pwm_leds
  #define DEFAULT_LABEL(id) STRINGIFY(_CONCAT(LED_PWM_, id))
#elif defined(CONFIG_CAF_LEDS_GPIO)
  #define DT_DRV_COMPAT gpio_leds
  #define DEFAULT_LABEL(id) "leds"
#else
  #error "LED driver must be specified in configuration"
#endif

#define _LED_COLOR_ID(id) 0,

#define _LED_COLOR_COUNT(id)					\
	static const uint8_t led_colors_##id[] = {		\
		DT_INST_FOREACH_CHILD(id, _LED_COLOR_ID)	\
	};

DT_INST_FOREACH_STATUS_OKAY(_LED_COLOR_COUNT)

#define _LED_INSTANCE_DEF(id)						\
	{								\
		.label = DT_INST_PROP_OR(id, label, DEFAULT_LABEL(id)),	\
		.color_count = ARRAY_SIZE(led_colors_##id),		\
	},


static struct led leds[] = {
	DT_INST_FOREACH_STATUS_OKAY(_LED_INSTANCE_DEF)
};


static int set_color_one_channel(struct led *led, struct led_color *color)
{
	/* For a single color LED convert color to brightness. */
	unsigned int brightness = 0;

	for (size_t i = 0; i < ARRAY_SIZE(color->c); i++) {
		brightness += color->c[i];
	}
	brightness /= ARRAY_SIZE(color->c);

	return led_set_brightness(led->dev, 0, brightness);
}

static int set_color_all_channels(struct led *led, struct led_color *color)
{
	int err = 0;

	for (size_t i = 0; (i < ARRAY_SIZE(color->c)) && !err; i++) {
		err = led_set_brightness(led->dev, i, color->c[i]);
	}

	return err;
}

static void set_color(struct led *led, struct led_color *color)
{
	int err;

	if (led->color_count == ARRAY_SIZE(color->c)) {
		err = set_color_all_channels(led, color);
	} else {
		err = set_color_one_channel(led, color);
	}

	if (err) {
		LOG_ERR("Cannot set LED brightness (err: %d)", err);
	}
}

static void set_off(struct led *led)
{
	struct led_color nocolor = {0};

	set_color(led, &nocolor);
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
	set_color(led, &led->color);

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

		k_work_reschedule(&led->work, K_MSEC(next_delay));
	}
}

static void led_update(struct led *led)
{
	k_work_cancel_delayable(&led->work);

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

		k_work_reschedule(&led->work, K_MSEC(next_delay));
	} else {
		LOG_WRN("LED effect with no effect");
	}
}

static void verify_labels(void)
{
	/* Zephyr boards by default define "gpio_leds" compatible DT node called "leds". CAF LEDs
	 * uses "leds" as a default name to get the related GPIO LED driver binding. In that case
	 * only one CAF led instance can use the default driver device name. Using the same driver
	 * device name by multiple instances would result in referring to improper driver device.
	 */
	if (IS_ENABLED(CONFIG_ASSERT) && IS_ENABLED(CONFIG_CAF_LEDS_GPIO)) {
		size_t default_label_cnt = 0;

		for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
			if (!strcmp(leds[i].label, DEFAULT_LABEL(0))) {
				default_label_cnt++;
			}
		}

		__ASSERT_NO_MSG(default_label_cnt <= 1);
	}
}

static int leds_init(void)
{
	int err = 0;

	BUILD_ASSERT(DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) > 0, "No LEDs defined");

	verify_labels();

	for (size_t i = 0; (i < ARRAY_SIZE(leds)) && !err; i++) {
		struct led *led = &leds[i];

		__ASSERT_NO_MSG((led->color_count == 1) || (led->color_count == 3));

		led->dev = device_get_binding(led->label);

		if (!led->dev) {
			LOG_ERR("Cannot bind %s", led->label);
			err = -ENXIO;
		} else {
			k_work_init_delayable(&led->work, work_handler);
			led_update(led);
		}
	}

	return err;
}

static void leds_start(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
#ifdef CONFIG_PM_DEVICE
		int err = pm_device_state_set(leds[i].dev,
					      PM_DEVICE_STATE_ACTIVE,
					      NULL, NULL);
		if (err) {
			LOG_ERR("PWM enable failed");
		}
#endif
		led_update(&leds[i]);
	}
}

static void leds_stop(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		k_work_cancel_delayable(&leds[i].work);

		set_off(&leds[i]);

#ifdef CONFIG_PM_DEVICE
		int err = pm_device_state_set(leds[i].dev,
					      PM_DEVICE_STATE_SUSPEND,
					      NULL, NULL);
		if (err) {
			LOG_ERR("PWM disable failed");
		}
#endif
	}
}

static bool event_handler(const struct event_header *eh)
{
	static bool initialized;

	if (is_led_event(eh)) {
		const struct led_event *event = cast_led_event(eh);

		__ASSERT_NO_MSG(event->led_id < ARRAY_SIZE(leds));

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
