/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LED_EFFECT_H_
#define _LED_EFFECT_H_

/**
 * @brief LED Effect
 * @defgroup led_effect_CAF LED Effect
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _CAF_LED_COLOR_CHANNEL_COUNT 3

/** @brief Color of LED.
 */
struct led_color {
	/** Values for color channels. */
	uint8_t c[_CAF_LED_COLOR_CHANNEL_COUNT];
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
	uint16_t substep_count;

	/** Duration of a single substep. */
	uint16_t substep_time;
};

/** @brief Single LED effect.
 */
struct led_effect {
	/** Sequence of LED color changes. It is defined by subsequent steps. */
	const struct led_effect_step *steps;

	/** Number of steps for the given effect. */
	uint16_t step_count;

	/** Flag that indicates if the sequence should start again after it finishes. */
	bool loop_forever;
};

/** Transform color brightness from 8-bit space to percentage representation.
 *
 * @param _val Color brightness in 0-255 range
 *
 * @return Color brightness in 0-100 range
 */
#define COLOR_BRIGHTNESS_TO_PCT(_val) ((_val * 100) / UINT8_MAX)

/**
 * @brief Pass a color value as a macro argument
 *
 * @ref LED_COLOR macro contains commas in its body.
 * This means that after the argument is expanded, the macro is to be treated
 * as multiple arguments. This also makes it impossible to pass an argument
 * from macro level to another macro.
 * The macro allows the usage of the color argument as an argument to
 * another macro level.
 *
 * @param ... Any list of arguments that have to be treated as
 *            a single argument for the macro.
 */
#define LED_COLOR_ARG_PASS(...) __VA_ARGS__

/** Create LED color initializer for LED turned on.
 *
 * @note As arguments, pass the brightness levels for every color channel.
 *
 * @note The macro returns the structure initializer that once expanded
 *       contains commas not placed in brackets.
 *       This means that when passed as an argument, this argument
 *       cannot be passed simply to another macro.
 *       Use @ref LED_COLOR_ARG_PASS macro for the preprocessor
 *       to treat it as a single argument again.
 */
#define LED_COLOR(_r, _g, _b) {				\
		.c = {					\
			COLOR_BRIGHTNESS_TO_PCT(_r),	\
			COLOR_BRIGHTNESS_TO_PCT(_g),	\
			COLOR_BRIGHTNESS_TO_PCT(_b)	\
		}					\
}

/** Create LED color initializer for LED turned off.
 */
#define LED_NOCOLOR() {			\
		.c = {0, 0, 0}		\
}

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
 * @param _color      Selected LED color.
 * @param _on_time    Time LED will remain on in milliseconds.
 * @param _off_delay  Time in which LED will gradually switch to off
 *                    (in milliseconds).
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

/** Create LED blinking effect initializer with two periods as arguments.
 *
 * LED color is periodically changed between the selected color and the LED
 * turned off.
 * This macro takes two periods: for on and off time.
 *
 * @param _period_on	Period of time for which LED is on.
 * @param _period_off	Period of time for which LED is off.
 * @param _color	Selected LED color.
 *
 * @sa LED_EFFECT_LED_BLINK
 */
#define LED_EFFECT_LED_BLINK2(_period_on, _period_off, _color)			\
	{									\
		.steps = ((const struct led_effect_step[]) {			\
			{							\
				.color = _color,				\
				.substep_count = 1,				\
				.substep_time = (_period_off),			\
			},							\
			{							\
				.color = LED_NOCOLOR(),				\
				.substep_count = 1,				\
				.substep_time = (_period_on),			\
			},							\
		}),								\
		.step_count = 2,						\
		.loop_forever = true,						\
	}

/** Create LED blinking effect initializer with one period given
 *
 * LED color is periodically changed between the selected color and the LED
 * turned off.
 * The same time is used for both: on and off time.
 *
 * @param _period	Period of time between LED color switches.
 * @param _color	Selected LED color.
 *
 * @sa LED_EFFECT_LED_BLINK
 */
#define LED_EFFECT_LED_BLINK(_period, _color) \
	LED_EFFECT_LED_BLINK2(_period, _period, LED_COLOR_ARG_PASS(_color))

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
#define LED_EFFECT_LED_BREATH(_period, _color)						\
	{										\
		.steps = ((const struct led_effect_step[]) {				\
			{								\
				.color = _color,					\
				.substep_count = _BREATH_SUBSTEPS,			\
				.substep_time = ((_period + (_BREATH_SUBSTEPS - 1))	\
						/ _BREATH_SUBSTEPS),			\
			},								\
			{								\
				.color = _color,					\
				.substep_count = 1,					\
				.substep_time = _period,				\
			},								\
			{								\
				.color = LED_NOCOLOR(),					\
				.substep_count = _BREATH_SUBSTEPS,			\
				.substep_time = ((_period + (_BREATH_SUBSTEPS - 1))	\
						/ _BREATH_SUBSTEPS),			\
			},								\
			{								\
				.color = LED_NOCOLOR(),					\
				.substep_count = 1,					\
				.substep_time = _period,				\
			},								\
		}),									\
		.step_count = 4,							\
		.loop_forever = true,							\
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
 * @param i	Tick number (required by UTIL_LISTIFY).
 * @param ...	Selected LED color.
 */
#define _LED_CLOCK_TIK(i, ...)						\
		{							\
			.color = __VA_ARGS__,				\
			.substep_count = 1,				\
			.substep_time = LED_CLOCK_BLINK_PERIOD,		\
		},							\
		{							\
			.color = LED_NOCOLOR(),				\
			.substep_count = 1,				\
			.substep_time = LED_CLOCK_BLINK_PERIOD,		\
		}

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
			LISTIFY(_ticks, _LED_CLOCK_TIK, (,), _color)		\
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
