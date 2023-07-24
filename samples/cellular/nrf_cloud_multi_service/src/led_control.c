/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/led.h>
#include <stdlib.h>

#include "led_control.h"

/* We implement, here, both 4-led and RGB LED control, but you may feel free to implement
 * whatever your board can support. You must make sure your board Device Tree Source or overlays
 * include a device marked as compatible with either the pwm-leds or gpio-leds driver in order to
 * use the LED driver provided by Zephyr, as we do here. Both the nRF9160dk and thingy91 have such
 * devices defined in their default Device Tree Sources. See nrf9160dk_nrf9160_common.dts and
 * thingy91_nrf9160_common.dts.
 */

LOG_MODULE_REGISTER(led_control, CONFIG_MULTI_SERVICE_LOG_LEVEL);

/* Find a Device Tree node compatible with either the pwm_leds or gpio_leds driver, depending on
 * what has been configured.
 */
#if defined(CONFIG_LED_INDICATION_PWM)
const static struct device *led_device = DEVICE_DT_GET_ANY(pwm_leds);
#elif defined(CONFIG_LED_INDICATION_GPIO)
const static struct device *led_device = DEVICE_DT_GET_ANY(gpio_leds);
#else
const static struct device *led_device;
#endif

#define LED_RGB_R 0
#define LED_RGB_G 1
#define LED_RGB_B 2

#define LED_4LED_1 0
#define LED_4LED_2 1
#define LED_4LED_3 2
#define LED_4LED_4 3

/**
 * @brief If the LED mode is 4 binary LEDs, set the LED state
 *
 * @param l1 - LED 1 on?
 * @param l2 - LED 2 on?
 * @param l3 - LED 3 on?
 * @param l4 - LED 4 on?
 */
static void led_set_4led(bool l1, bool l2, bool l3, bool l4)
{
	led_set_brightness(led_device, LED_4LED_1, l1 ? 100 : 0);
	led_set_brightness(led_device, LED_4LED_2, l2 ? 100 : 0);
	led_set_brightness(led_device, LED_4LED_3, l3 ? 100 : 0);
	led_set_brightness(led_device, LED_4LED_4, l4 ? 100 : 0);
}

/**
 * @brief If the LED mode is RGB, set the RGB values.
 *
 * @param r - LED Red channel.
 * @param g - LED Green channel.
 * @param b - LED Blue channel.
 */
static void led_set_rgb(int r, int g, int b)
{
	led_set_brightness(led_device, LED_RGB_R, r);
	led_set_brightness(led_device, LED_RGB_G, g);
	led_set_brightness(led_device, LED_RGB_B, b);
}

/**
 * @brief Change the LEDs to a pattern that indicates success.
 *
 * @param frame_number - An integer used for basic animations.
 */
static void led_pattern_success(int frame_number)
{
	/* Square wave with period 12 frames and duty cycle 25% */
	bool blink = (frame_number % 12) < 3;

	if (IS_ENABLED(CONFIG_LED_INDICATOR_RGB)) {
		/* Blink the green channel */
		led_set_rgb(0, blink ? 100 : 0, 0);
	} else if (IS_ENABLED(CONFIG_LED_INDICATOR_4LED)) {
		/* Blink alternating diagonals on a 4LED square */
		led_set_4led(blink, !blink, !blink, blink);
	}
}

/**
 * @brief Change the LEDs to a pattern that indicates failure.
 *
 * @param frame_number - An integer used for basic animations.
 */
static void led_pattern_failure(int frame_number)
{
	/* Square wave with period 20 frames and duty cycle 75% */
	int blink = (frame_number % 20) < 15;

	if (IS_ENABLED(CONFIG_LED_INDICATOR_RGB)) {
		/* Blink red channel */
		led_set_rgb(blink ? 100 : 0, 0, 0);
	} else if (IS_ENABLED(CONFIG_LED_INDICATOR_4LED)) {
		/* Blink all four LEDs */
		led_set_4led(blink, blink, blink, blink);
	}
}

/**
 * @brief Change the LEDs to a pattern that indicates no relevant activity.
 *
 * @param frame_number - An integer used for basic animations.
 */
static void led_pattern_idle(int frame_number)
{
	if (IS_ENABLED(CONFIG_LED_INDICATOR_RGB)) {
		/* A triangle wave between 0 and 20 */
		int breath = abs((frame_number % 40) - 20);

		/* Breathe back and forth between cyan and pure blue */
		led_set_rgb(0, breath, 20);
	} else if (IS_ENABLED(CONFIG_LED_INDICATOR_4LED)) {
		/* Square wave with period 40 frames and duty cycle 50% */
		bool blink = (frame_number % 40) < 20;

		/* Blink between two LEDs on opposite sides of a diagonal, of the 4LED square,
		 * namely "LED1" and "LED4" on 9160dk boards
		 */
		led_set_4led(blink, false, false, !blink);
	}
}

/**
 * @brief Change the LEDs to a pattern that indicates waiting.
 *
 * @param frame_number - An integer used for basic animations.
 */
static void led_pattern_waiting(int frame_number)
{
	if (IS_ENABLED(CONFIG_LED_INDICATOR_RGB)) {
		/* A triangle wave between 0 and 10 */
		int breath = abs((frame_number % 20) - 10);

		/* Pulsating orange */
		led_set_rgb(breath * 10, breath, 0);
	} else if (IS_ENABLED(CONFIG_LED_INDICATOR_4LED)) {
		/* Spin a single LED around the 4LED square */
		led_set_4led((frame_number % 4) == 0,
			     (frame_number % 4) == 1,
			     (frame_number % 4) == 3,
			     (frame_number % 4) == 2);
	}
}

/**
 * @brief Disable the LEDs.
 */
static void led_pattern_disabled(void)
{
	if (IS_ENABLED(CONFIG_LED_INDICATOR_RGB)) {
		led_set_rgb(0, 0, 0);
	} else if (IS_ENABLED(CONFIG_LED_INDICATOR_4LED)) {
		led_set_4led(false, false, false, false);
	}
}

/* Zephyr event for signaling cross-thread when an LED pattern has been requested */
#define LED_PATTERN_REQUESTED (1 << 1)
static K_EVENT_DEFINE(led_events);

/* The currently requested pattern, frames of it remaining, and total number of frames it has
 * been executed for. Negative frames remaining indicates an indefinite effect.
 */
atomic_t requested_pattern = LED_DISABLED;
atomic_t requested_pattern_frames_remaining = -1;
atomic_t requested_pattern_animation_frame;

void start_led_pattern(int frames, enum led_pattern pattern)
{
	if (!IS_ENABLED(CONFIG_LED_INDICATION_DISABLED)) {
		atomic_set(&requested_pattern, pattern);
		atomic_set(&requested_pattern_frames_remaining, frames);
		atomic_set(&requested_pattern_animation_frame, 0);
		k_event_post(&led_events, LED_PATTERN_REQUESTED);
		LOG_DBG("LED Pattern Requested");
	}
}

static K_TIMER_DEFINE(led_animate_timer, NULL, NULL);
/**
 * @brief Display a single frame of a specified pattern
 *
 * @param frame_number - The frame number to show.
 * @param led_pattern - The pattern to show.
 */
static void animate_leds_frame(int frame_number, enum led_pattern pattern)
{
	/* Wait for the timer from the previous call to this function to expire, and then
	 * immediately restart the timer.
	 * This guarantees that this function completes, at most, once every frame interval
	 */

	k_timer_status_sync(&led_animate_timer);
	k_timer_start(&led_animate_timer, K_MSEC(100), K_FOREVER);

	/* Run a single frame of the pattern */
	switch (pattern) {
	case LED_IDLE:
		led_pattern_idle(frame_number);
		break;
	case LED_WAITING:
		led_pattern_waiting(frame_number);
		break;
	case LED_FAILURE:
		led_pattern_failure(frame_number);
		break;
	case LED_SUCCESS:
		led_pattern_success(frame_number);
		break;
	default:
		led_pattern_disabled();
	}
}

void led_animation_thread_fn(void)
{
	LOG_DBG("LED Management Started");

	if (!device_is_ready(led_device)) {
		LOG_ERR("No LED device found, cancelling LED indication.");
		return;
	}

	/* Self-request an initial LED update, using the default state, or otherwise keeping what
	 * any pre-existing thread already requested.
	 */
	k_event_post(&led_events, LED_PATTERN_REQUESTED);

	while (true) {
		/* Wait for a pattern to be started */
		(void)k_event_wait(&led_events, LED_PATTERN_REQUESTED, false, K_FOREVER);

		LOG_DBG("LED Pattern Started");
		while (true) {
			/* Decrement frames remaining, and exit if we hit zero, or if
			 * the requested pattern is LED_DISABLED.
			 *
			 * Negative values will indefinitely decrement without hitting zero.
			 */
			int pattern = atomic_get(&requested_pattern);
			int fr = atomic_dec(&requested_pattern_frames_remaining);

			if (fr == 0 || pattern == LED_DISABLED) {
				break;
			}

			/* Perform an animation frame, incrementing the current animation frame
			 * counter. Note that LED_DISABLED will never be performed by this call,
			 * since that pattern causes us to break out of the loop above.
			 */
			animate_leds_frame(atomic_inc(&requested_pattern_animation_frame), pattern);
		}

		/* The pattern has ended, clear the pattern event flag so that a new one can
		 * be requested.
		 */
		k_event_set(&led_events, 0);

		/* Either switch to the IDLE pattern, or disable LEDs without activating a new
		 * pattern, depending on whether continuous indication is requested.
		 */
		if (IS_ENABLED(CONFIG_LED_CONTINUOUS_INDICATION)) {
			long_led_pattern(LED_IDLE);
		} else {
			animate_leds_frame(0, LED_DISABLED);
		}

		LOG_DBG("LED Pattern Ended");
	}
}

void long_led_pattern(enum led_pattern pattern)
{
	start_led_pattern(-1, pattern);
}

void short_led_pattern(enum led_pattern pattern)
{
	start_led_pattern(30, pattern);
}

void stop_led_pattern(void)
{
	short_led_pattern(LED_DISABLED);
}
