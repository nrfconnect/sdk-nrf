/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/pwm.h>
#include <string.h>

#include "led.h"
#include "led_pwm.h"
#include "led_effect.h"

struct led {
	const struct device *pwm_dev;

	size_t id;
	struct led_color color;
	const struct led_effect *effect;
	uint16_t effect_step;
	uint16_t effect_substep;

	struct k_work_delayable work;
	struct k_work_sync work_sync;
};

static const struct led_effect effect[] = {
	[LED_LTE_CONNECTING] = LED_EFFECT_LED_BREATHE(
					LED_ON_PERIOD_NORMAL,
					LED_OFF_PERIOD_NORMAL,
					LED_LTE_CONNECTING_COLOR),
	[LED_CLOUD_PUBLISHING] = LED_EFFECT_LED_BREATHE(
					LED_ON_PERIOD_ERROR,
					LED_OFF_PERIOD_ERROR,
					LED_CLOUD_PUBLISHING_COLOR),
	[LED_ERROR_SYSTEM_FAULT] = LED_EFFECT_LED_BREATHE(
					LED_ON_PERIOD_ERROR,
					LED_OFF_PERIOD_ERROR,
					LED_ERROR_SYSTEM_FAULT_COLOR),
	[LED_FOTA_UPDATE_REBOOT] = LED_EFFECT_LED_BREATHE(
					LED_ON_PERIOD_STROBE,
					LED_OFF_PERIOD_STROBE,
					LED_FOTA_UPDATE_REBOOT_COLOR),
	[LED_GPS_SEARCHING] = LED_EFFECT_LED_BREATHE(
					LED_ON_PERIOD_ERROR,
					LED_OFF_PERIOD_ERROR,
					LED_GPS_SEARCHING_COLOR),
	[LED_ACTIVE_MODE] = LED_EFFECT_LED_BREATHE(
					LED_ON_PERIOD_ERROR,
					LED_OFF_PERIOD_ERROR,
					LED_ACTIVE_MODE_COLOR),
	[LED_PASSIVE_MODE] = LED_EFFECT_LED_BREATHE(
					LED_ON_PERIOD_SHORT,
					LED_OFF_PERIOD_LONG,
					LED_PASSIVE_MODE_COLOR),
};

static struct led_effect custom_effect = LED_EFFECT_LED_BREATHE(
	LED_ON_PERIOD_NORMAL, LED_OFF_PERIOD_NORMAL, LED_NOCOLOR());

static struct led leds;
static const size_t led_pins[3] = {
	CONFIG_LED_RED_PIN,
	CONFIG_LED_GREEN_PIN,
	CONFIG_LED_BLUE_PIN,
};

static void pwm_out(struct led *led, struct led_color *color)
{
	for (size_t i = 0; i < ARRAY_SIZE(color->c); i++) {
		pwm_pin_set_usec(led->pwm_dev, led_pins[i], 255, color->c[i],
				 0);
	}
}

static void pwm_off(struct led *led)
{
	struct led_color nocolor = { 0 };

	pwm_out(led, &nocolor);
}

static void work_handler(struct k_work *work)
{
	struct led *led = CONTAINER_OF(work, struct led, work);
	const struct led_effect_step *effect_step =
		&leds.effect->steps[leds.effect_step];
	int substeps_left = effect_step->substep_cnt - leds.effect_substep;

	for (size_t i = 0; i < ARRAY_SIZE(leds.color.c); i++) {
		int diff = (effect_step->color.c[i] - leds.color.c[i]) /
			   substeps_left;
		leds.color.c[i] += diff;
	}

	pwm_out(led, &leds.color);

	leds.effect_substep++;
	if (leds.effect_substep == effect_step->substep_cnt) {
		leds.effect_substep = 0;
		leds.effect_step++;

		if (leds.effect_step == leds.effect->step_cnt) {
			if (leds.effect->loop_forever) {
				leds.effect_step = 0;
			}
		} else {
			__ASSERT_NO_MSG(leds.effect->steps[leds.effect_step]
						.substep_cnt > 0);
		}
	}

	if (leds.effect_step < leds.effect->step_cnt) {
		int32_t next_delay =
			leds.effect->steps[leds.effect_step].substep_time;

		k_work_reschedule(&leds.work, K_MSEC(next_delay));
	}
}

static void led_update(struct led *led)
{
	k_work_cancel_delayable_sync(&led->work, &led->work_sync);

	led->effect_step = 0;
	led->effect_substep = 0;

	if (!led->effect) {
		printk("No effect set");
		return;
	}

	__ASSERT_NO_MSG(led->effect->steps);

	if (led->effect->step_cnt > 0) {
		int32_t next_delay =
			led->effect->steps[led->effect_step].substep_time;

		k_work_schedule(&led->work, K_MSEC(next_delay));
	} else {
		printk("LED effect with no effect");
	}
}

int led_pwm_init(void)
{
	const char *dev_name = CONFIG_LED_PWM_DEV_NAME;

	leds.pwm_dev = device_get_binding(dev_name);

	if (!leds.pwm_dev) {
		printk("Could not bind to device %s\n", dev_name);
		return -ENODEV;
	}

	k_work_init_delayable(&leds.work, work_handler);

	return 0;
}

void led_pwm_start(void)
{
#if defined(CONFIG_PM_DEVICE)
	int err = pm_device_state_set(leds.pwm_dev, PM_DEVICE_STATE_ACTIVE,
				      NULL, NULL);
	if (err) {
		printk("PWM enable failed\n");
	}
#endif
	led_update(&leds);
}

void led_pwm_stop(void)
{
	k_work_cancel_delayable_sync(&leds.work, &leds.work_sync);
#if defined(CONFIG_PM_DEVICE)
	int err = pm_device_state_set(leds.pwm_dev, PM_DEVICE_STATE_SUSPEND,
				      NULL, NULL);
	if (err) {
		LOG_ERR("PWM disable failed");
	}
#endif
	pwm_off(&leds);
}

void led_pwm_set_effect(enum led_pattern state)
{
	leds.effect = &effect[state];
	led_update(&leds);
}

int led_pwm_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
	struct led_effect effect =
		LED_EFFECT_LED_BREATHE(LED_ON_PERIOD_NORMAL,
				       LED_OFF_PERIOD_NORMAL,
				       LED_COLOR(red, green, blue));

	memcpy((void *)custom_effect.steps, (void *)effect.steps,
	       effect.step_cnt * sizeof(struct led_effect_step));

	leds.effect = &custom_effect;
	led_update(&leds);

	return 0;
}
