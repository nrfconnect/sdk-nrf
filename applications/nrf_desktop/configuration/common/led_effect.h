/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _LED_EFFECT_H_
#define _LED_EFFECT_H_

/**
 * @brief LED Effect
 * @defgroup led_effect_DESK LED Effect
 * @{
 */

#include <zephyr/types.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Color of LED.
 */
struct led_color {
	/** Values for color channels. */
	u8_t c[CONFIG_DESKTOP_LED_COLOR_COUNT];
};


/** @brief Single step of a LED effect.
 *
 * During a single step, color of LED changes from the color before the step
 * to the color defined in the step. The color update may be done in multiple
 * substeps to achieve smooth, gradual change.
 */
struct led_effect_step {
	/** LED color at the end of the step. */
	struct led_color color;

	/** Number of substeps. */
	u16_t substep_count;

	/** Duration of a single substep. */
	u16_t substep_time;
};


/** @brief Single LED effect.
 */
struct led_effect {
	/** Sequence of LED color changes. It is defined by subsequent steps. */
	const struct led_effect_step *steps;

	/** Number of steps for the given effect. */
	u16_t step_count;

	/** Flag that indicates if the sequence should start again after it finishes. */
	bool loop_forever;
};


/** Create LED color initializer for LED turned on.
 *
 * @note As arguments, pass the brightness levels for every color channel.
 * The amount of the color channels is defined in the configuration (three
 * channels by default).
 */
#if CONFIG_DESKTOP_LED_COLOR_COUNT == 1
	#define LED_COLOR(_brightness) {	\
			.c = {_brightness}	\
		}
#elif CONFIG_DESKTOP_LED_COLOR_COUNT == 3
	#define LED_COLOR(_r, _g, _b) {		\
			.c = {_r, _g, _b}	\
	}
#else
	#error "Unsupported color count"
#endif


/** Create LED color initializer for LED turned off.
 */
#if CONFIG_DESKTOP_LED_COLOR_COUNT == 1
	#define LED_NOCOLOR() {			\
			.c = {0}		\
		}
#elif CONFIG_DESKTOP_LED_COLOR_COUNT == 3
	#define LED_NOCOLOR() {			\
			.c = {0, 0, 0}		\
	}
#else
	#error "Unsupported color count"
#endif


/** Create LED turned on effect initializer.
 *
 * LED color remains constant.
 *
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_ON(_color)						\
	{									\
		.steps = ((const struct led_effect_step[]) {			\
			{							\
				.color = _color,				\
				.substep_count = 1,				\
				.substep_time = 0,				\
			},							\
		}),								\
		.step_count = 1,						\
		.loop_forever = false,						\
	}


/** Create LED turned off effect initializer.
 */
#define LED_EFFECT_LED_OFF() LED_EFFECT_LED_ON(LED_NOCOLOR())


/** Create LED turned on for a brief period effect initializer.
 *
 * LED color remains constant for a defined time, then goes off.
 *
 * @param _color	Selected LED color.
 * @param _on_time	Time LED will remain on in milliseconds.
 * @param _off_time	Time in which LED will gradually switch to off
 *			(in milliseconds).
 */
#define LED_EFFECT_LED_ON_GO_OFF(_color, _on_time, _off_delay)			\
	{									\
		.steps = ((const struct led_effect_step[]) {			\
			{							\
				.color = _color,				\
				.substep_count = 1,				\
				.substep_time = 0,				\
			},							\
			{							\
				.color = _color,				\
				.substep_count = 1,				\
				.substep_time = (_on_time),			\
			},							\
			{							\
				.color = LED_NOCOLOR(),				\
				.substep_count = (_off_delay)/10,		\
				.substep_time = 10,				\
			},							\
		}),								\
		.step_count = 3,						\
		.loop_forever = false,						\
	}



/** Create LED blinking effect initializer.
 *
 * LED color is periodically changed between the selected color and the LED
 * turned off.
 *
 * @param _period	Period of time between LED color switches.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_BLINK(_period, _color)					\
	{									\
		.steps = ((const struct led_effect_step[]) {			\
			{							\
				.color = _color,				\
				.substep_count = 1,				\
				.substep_time = (_period),			\
			},							\
			{							\
				.color = LED_NOCOLOR(),				\
				.substep_count = 1,				\
				.substep_time = (_period),			\
			},							\
		}),								\
		.step_count = 2,						\
		.loop_forever = true,						\
	}


/** @def _BREATH_SUBSTEPS
 *
 * @brief Substeps for color update for LED breathing effect.
 */
#define _BREATH_SUBSTEPS 15


/** Create LED breathing effect initializer.
 *
 * LED color is smoothly, gradually changed between the LED turned off
 * and the selected color.
 *
 * @param _period	Period of time for single substep.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_BREATH(_period, _color)					\
	{									\
		.steps = ((const struct led_effect_step[]) {			\
			{							\
				.color = _color,				\
				.substep_count = _BREATH_SUBSTEPS,		\
				.substep_time = ((_period + (_BREATH_SUBSTEPS - 1)) / _BREATH_SUBSTEPS),\
			},							\
			{							\
				.color = _color,				\
				.substep_count = 1,				\
				.substep_time = _period,			\
			},							\
			{							\
				.color = LED_NOCOLOR(),				\
				.substep_count = _BREATH_SUBSTEPS,		\
				.substep_time = ((_period + (_BREATH_SUBSTEPS - 1)) / _BREATH_SUBSTEPS),\
			},							\
			{							\
				.color = LED_NOCOLOR(),				\
				.substep_count = 1,				\
				.substep_time = _period,			\
			},							\
		}),								\
		.step_count = 4,						\
		.loop_forever = true,						\
	}


/** @def LED_CLOCK_BLINK_PERIOD
 *
 * @brief Period of time between color changes while the LED is blinking
 * (LED clock effect).
 */
#define LED_CLOCK_BLINK_PERIOD 200


/** @def LED_CLOCK_SLEEP_PERIOD
 *
 * @brief Period of time when the LED is turned off (LED clock effect).
 */
#define LED_CLOCK_SLEEP_PERIOD 1000


/** Create LED steps initializer for single clock tick.
 *
 * A single clock tick is a single LED blink with the defined color.
 * This macro is used by UTIL_LISTIFY macro.
 *
 * @param i		Tick number (required by UTIL_LISTIFY).
 * @param _color	Selected LED color.
 */
#define LED_CLOCK_TIK(i, _color)					\
		{							\
			.color = _color,				\
			.substep_count = 1,				\
			.substep_time = LED_CLOCK_BLINK_PERIOD,		\
		},							\
		{							\
			.color = LED_NOCOLOR(),				\
			.substep_count = 1,				\
			.substep_time = LED_CLOCK_BLINK_PERIOD,		\
		},							\


/** Create LED clock effect initializer.
 *
 * LED blinks a defined number of times, then it is turned off for a defined
 * period of time. The sequence is repeated periodically.
 *
 * @note You can pass only one additional argument to the UTIL_LISTIFY macro,
 * which in this case is LED color. Period is defined separately.
 *
 * @param _ticks	Number of ticks.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_CLOCK(_ticks, _color)					\
	{									\
		.steps = ((const struct led_effect_step[]) {			\
			{							\
				.color = LED_NOCOLOR(),				\
				.substep_count = 1,				\
				.substep_time = LED_CLOCK_SLEEP_PERIOD,		\
			},							\
			UTIL_LISTIFY(_ticks, LED_CLOCK_TIK, _color)		\
		}),								\
		.step_count = (2 * _ticks + 1),					\
		.loop_forever = true,						\
	}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LED_EFFECT_H_ */
