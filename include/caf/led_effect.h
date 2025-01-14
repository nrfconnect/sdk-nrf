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

/**
 * @brief Pass a struct value as a macro argument
 *
 * @ref LED_COLOR and LED_EFFECT_EASING macros contain commas in their bodies.
 * This means that after the argument is expanded, the macro is to be treated
 * as multiple arguments. This also makes it impossible to pass an argument
 * from macro level to another macro.
 * The macro allows the usage of the color argument as an argument to
 * another macro level.
 *
 * @param ... Any list of arguments that have to be treated as
 *            a single argument for the macro.
 */
#define LED_MACRO_ARG_PASS(...) __VA_ARGS__

/** @brief Color of LED.
 */
struct led_color {
	/** Values for color channels. */
	uint8_t c[_CAF_LED_COLOR_CHANNEL_COUNT];
};

/** Transform color brightness from 8-bit space to percentage representation.
 *
 * @param _val Color brightness in 0-255 range
 *
 * @return Color brightness in 0-100 range
 */
#define COLOR_BRIGHTNESS_TO_PCT(_val) ((_val * 100) / UINT8_MAX)

/** Create LED color initializer for LED turned on.
 *
 * @note As arguments, pass the brightness levels for every color channel.
 *
 * @note The macro returns the structure initializer that once expanded
 *       contains commas not placed in brackets.
 *       This means that when passed as an argument, this argument
 *       cannot be passed simply to another macro.
 *       Use @ref LED_MACRO_ARG_PASS macro for the preprocessor
 *       to treat it as a single argument again.
 */
#define LED_COLOR(_r, _g, _b) {				\
		.c = {					\
			COLOR_BRIGHTNESS_TO_PCT(_r),	\
			COLOR_BRIGHTNESS_TO_PCT(_g),	\
			COLOR_BRIGHTNESS_TO_PCT(_b)	\
		}					\
}

#define LED_COLOR_OFF LED_COLOR(0, 0, 0)
#define LED_COLOR_WHITE LED_COLOR(255, 255, 255)
#define LED_COLOR_RED LED_COLOR(255, 0, 0)
#define LED_COLOR_GREEN LED_COLOR(0, 255, 0)
#define LED_COLOR_BLUE LED_COLOR(0, 0, 255)
#define LED_COLOR_YELLOW LED_COLOR(255, 255, 0)
#define LED_COLOR_MAGENTA LED_COLOR(255, 0, 255)
#define LED_COLOR_CYAN LED_COLOR(0, 255, 255)
#define LED_COLOR_ORANGE LED_COLOR(255, 165, 0)
#define LED_COLOR_PINK LED_COLOR(255, 192, 203)
#define LED_COLOR_PURPLE LED_COLOR(128, 0, 128)
#define LED_COLOR_TURQUOISE LED_COLOR(64, 224, 208)
#define LED_COLOR_LIME LED_COLOR(0, 255, 0)
#define LED_COLOR_INDIGO LED_COLOR(75, 0, 130)
#define LED_COLOR_BROWN LED_COLOR(165, 42, 42)
#define LED_COLOR_GREY(brightness) LED_COLOR(brightness, brightness, brightness)

/** @brief LED effect easing type.
 */
enum led_effect_easing_type {
	/** Eases linearly from 0 to 255 */
	LED_EFFECT_EASING_TYPE_LINEAR,
	/** Steps from 0 to 255 at a defined step time offset */
	LED_EFFECT_EASING_TYPE_STEP,
	/** Eases using a sine function, with configurable offset and max value parameters */
	LED_EFFECT_EASING_TYPE_SIN,
	/** Eases using a cubic function */
	LED_EFFECT_EASING_TYPE_CUBIC,
	/** Eases using a quadratic function */
	LED_EFFECT_EASING_TYPE_QUAD,
	/** Eases using an approximation of a cubic function */
	LED_EFFECT_EASING_TYPE_APPROX,
	/** Eases using a custom function */
	LED_EFFECT_EASING_TYPE_CUSTOM,

	/** Number of easing types */
	LED_EFFECT_EASING_TYPE_COUNT,
};

/** @brief The result of an easing function
 */
struct led_effect_easing_result {
	/** The current output value of the easing function. */
	uint8_t output;
	/** The number of input values that can be skipped until
	 * the output value needs to be updated. Returning 0 will
	 * update the output value on the next frame.
	 */
	uint8_t can_skip;
};

/**
 * @brief Easing function type.
 * @param fraction A value from 0 to LED_EFFECT_OUTPUT_MAX representing the fraction of the easing.
 * @return An easing result containing the new output value and the number of input values that
 * can be skipped.
 */
typedef struct led_effect_easing_result (*led_effect_custom_easing_func_t)(uint8_t fraction);

#define LED_EFFECT_OUTPUT_MAX UINT8_MAX

union led_effect_easing_param {
	struct {
		/** The fraction at which the output steps from 0 to LED_EFFECT_OUTPUT_MAX */
		uint8_t step_fraction;
	} step;
	struct {
		/** The offset to theta. This is added to the current time
		 * fraction before passing to the sin8 function.
		 * The output formula is sin8(offset + ((t / period) * max_value)).
		 */
		int8_t offset;
		/** The maximum value to scale to.
		 * The output formula is sin8(offset + ((t / period) * max_value)).
		 */
		uint8_t max_value;
	} sin;
	struct {
		/** A custom easing function. */
		led_effect_custom_easing_func_t func;
	} custom;
};

/** Easing. */
struct led_effect_easing {
	/** The easing type. */
	enum led_effect_easing_type type;

	/** Easing parameters. */
	union led_effect_easing_param param;
};

/** @brief LED effect easing.
 * @param _type The easing type
 */
#define LED_EFFECT_EASING(_type, _parameters) { \
		.type = _type, \
		.param = _parameters \
	}

/** @brief LED effect easing with no parameters.
 * @param _type The easing type
 */
#define LED_EFFECT_EASING_NO_PARAM(_type) LED_EFFECT_EASING(_type, {})

/** @brief LED effect easing step function.
 * @param _step_fraction Fraction at which the output steps from 0 to LED_EFFECT_OUTPUT_MAX.
 */
#define LED_EFFECT_EASING_STEP(_step_fraction) { \
	.type = LED_EFFECT_EASING_TYPE_STEP, \
	.param = { \
		.step = { \
			.step_fraction = _step_fraction, \
		} \
	} \
}

/** @brief LED effect easing step function with instant change.
 */
#define LED_EFFECT_EASING_STEP_INSTANT() LED_EFFECT_EASING_STEP(0)

/** @brief LED effect easing step function with change delayed until the end of the step.
 */
#define LED_EFFECT_EASING_STEP_DELAYED() LED_EFFECT_EASING_STEP(LED_EFFECT_OUTPUT_MAX)

/** @brief LED effect easing sine function.
 * @param _offset Offset to theta.
 * @param _max_value Maximum value to scale to before adding offset
 */
#define LED_EFFECT_EASING_SIN(_offset, _max_value) { \
	.type = LED_EFFECT_EASING_TYPE_SIN, \
	.param = { \
		.sin = { \
			.offset = _offset, \
			.max_value = _max_value \
		} \
	} \
}

/** @brief LED effect easing sine function starting from zero output.
 * @param _max_value Maximum value to scale to before adding offset
 */
#define LED_EFFECT_EASING_SIN_START_ZERO(_max_value) LED_EFFECT_EASING_SIN(-64, _max_value)

/** @brief LED effect easing cosine function.
 * @param _max_value Maximum value to scale to before adding offset
 */
#define LED_EFFECT_EASING_COS(_max_value) LED_EFFECT_EASING_SIN(64, _max_value)

/** @brief LED effect easing custom function.
 * @param _func Custom easing function.
 */
#define LED_EFFECT_EASING_CUSTOM(_func) { \
	.type = LED_EFFECT_EASING_TYPE_CUSTOM, \
	.param = { \
		.custom = { \
			.func = _func, \
		} \
	} \
}

/** @brief Single step of a LED effect.
 *
 * During a single step, color of LED changes from the color before the step
 * to the color defined in the step. The color update may be done in multiple
 * substeps to achieve smooth, gradual change.
 */
struct led_effect_step {
	/** LED color at the end of the step. */
	struct led_color color;

	/** Duration of the step in ms. */
	uint16_t duration;

	/** Easing parameters. */
	struct led_effect_easing easing;
};

/** @brief Single LED effect step
 * @param _color Color of the LED at the end of the step.
 * @param _duration Duration of the step in milliseconds.
 * @param _easing Easing function for the step.
 */
#define LED_EFFECT_STEP(_color, _duration, _easing) \
	{ \
		.color = _color, \
		.duration = _duration, \
		.easing = _easing \
	}

/** @brief Single LED effect step with instant change and no duration.
 * @param _color Color of the LED at the end of the step.
 */
#define LED_EFFECT_STEP_INSTANT(_color) LED_EFFECT_STEP(LED_MACRO_ARG_PASS(_color), 0, {0})

/** @brief Single LED effect step with linear easing.
 * @param _color Color of the LED at the end of the step.
 * @param _duration Duration of the step in milliseconds.
 */
#define LED_EFFECT_STEP_EASING_LINEAR(_color, _duration) \
	LED_EFFECT_STEP(LED_MACRO_ARG_PASS(_color), _duration, \
		LED_MACRO_ARG_PASS(LED_EFFECT_EASING_NO_PARAM(LED_EFFECT_EASING_TYPE_LINEAR)))

/** @brief Single LED effect step with step function and fraction parameter.
 * @param _color Color of the LED at the end of the step.
 * @param _duration Duration of the step in milliseconds.
 * @param _fraction Fraction at which the output steps from 0 to LED_EFFECT_OUTPUT_MAX.
 */
#define LED_EFFECT_STEP_EASING_STEP(_color, _duration, _fraction)	\
	LED_EFFECT_STEP(LED_MACRO_ARG_PASS(_color), _duration, \
		LED_MACRO_ARG_PASS(LED_EFFECT_EASING_STEP(_fraction)))

/** @brief Single LED effect step with instant change and specified duration.
 * @param _color Color of the LED at the end of the step.
 * @param _duration Duration of the step in milliseconds.
 */
#define LED_EFFECT_STEP_EASING_STEP_INSTANT(_color, _duration) \
	LED_EFFECT_STEP_EASING_STEP(LED_MACRO_ARG_PASS(_color), _duration, 0)

/** @brief Single LED effect step with delayed change after specified duration.
 * @param _color Color of the LED at the end of the step.
 * @param _duration Duration of the step in milliseconds.
 */
#define LED_EFFECT_STEP_EASING_STEP_DELAYED(_color, _duration) \
	LED_EFFECT_STEP_EASING_STEP(LED_MACRO_ARG_PASS(_color), _duration, LED_EFFECT_OUTPUT_MAX)

/** @brief Single LED effect step with sine easing function which starts at 0, rises to
 * LED_EFFECT_OUTPUT_MAX, then falls back to 0.
 *
 * @param _color Color of the LED at the end of the step.
 * @param _duration Duration of the step in milliseconds.
 */
#define LED_EFFECT_STEP_EASING_SIN(_color, _duration)	\
	LED_EFFECT_STEP(LED_MACRO_ARG_PASS(_color), _duration, \
		LED_EFFECT_EASING_SIN_START_ZERO(LED_EFFECT_OUTPUT_MAX))

/** @brief Single LED effect step with cosine easing function which starts at
 * LED_EFFECT_OUTPUT_MAX, falls to 0, then rises back to LED_EFFECT_OUTPUT_MAX.
 *
 * @param _color Color of the LED at the end of the step.
 * @param _duration Duration of the step in milliseconds.
 */
#define LED_EFFECT_STEP_EASING_COS(_color, _duration)	\
	LED_EFFECT_STEP(LED_MACRO_ARG_PASS(_color), _duration, \
		LED_EFFECT_EASING_COS(LED_EFFECT_OUTPUT_MAX))

/** @brief Single LED effect step with custom easing function.
 * @param _color Color of the LED at the end of the step.
 * @param _duration Duration of the step in milliseconds.
 * @param _func Custom easing function.
 */
#define LED_EFFECT_STEP_EASING_CUSTOM(_color, _duration, _func)	\
	LED_EFFECT_STEP(LED_MACRO_ARG_PASS(_color), _duration, LED_EFFECT_EASING_CUSTOM(_func))

/** @brief Single LED effect.
 */
struct led_effect {
	/** Sequence of LED color changes. It is defined by subsequent steps. */
	const struct led_effect_step *steps;

	/** Number of steps for the given effect. */
	uint16_t step_count;

	/** The number of times the effect should repeat. 0 is forever. */
	uint16_t repeat_count;
};

/** @brief LED effect with a single step.
 * @param _repeat_count Number of times the effect should repeat.
 * @param _step_count Number of steps in the effect
 * @param ... Steps of the effect.
 */
#define LED_EFFECT(_repeat_count, _step_count, ...) \
	{									\
		.steps = ((const struct led_effect_step[]) {			\
			__VA_ARGS__						\
		}),								\
		.step_count = _step_count,	\
		.repeat_count = _repeat_count,					\
	}


/** Create LED turned on effect initializer.
 *
 * LED color remains constant.
 *
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_ON(_color)						\
	LED_EFFECT(1, 1, LED_EFFECT_STEP_INSTANT(LED_MACRO_ARG_PASS(_color)))

/** Create LED turned off effect initializer.
 */
#define LED_EFFECT_LED_OFF() LED_EFFECT_LED_ON(LED_COLOR_OFF)

/** Create LED turned on then off effect initializer.
 *
 * LED color remains constant for a defined time, then switches off without transition.
 *
 * @param _duration Duration of the effect in milliseconds.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_ON_THEN_OFF(_duration, _color)						\
	LED_EFFECT(1, 2,									\
		LED_EFFECT_STEP_EASING_STEP_INSTANT(LED_MACRO_ARG_PASS(_color), _duration),	\
		LED_EFFECT_STEP_INSTANT(LED_COLOR_OFF)						\
	)

/** Create LED turned on for a brief period effect initializer.
 *
 * LED color remains constant for a defined time, then goes off linearly.
 *
 * @param _color      Selected LED color.
 * @param _on_time    Time LED will remain on in milliseconds.
 * @param _off_delay  Time in which LED will gradually switch to off
 *                    (in milliseconds).
 */
#define LED_EFFECT_LED_ON_GO_OFF(_color, _on_time, _off_delay)				   \
	LED_EFFECT(1, 2,								   \
		LED_EFFECT_STEP_EASING_STEP_INSTANT(LED_MACRO_ARG_PASS(_color), _on_time), \
		LED_EFFECT_STEP_EASING_LINEAR(LED_COLOR_OFF, _off_delay)		   \
	)

#define _LED_EFFECT_LED_COUNTER2_DELAY_STEP(i, _on_period, _off_period, ...) \
	LED_EFFECT_STEP_EASING_STEP_INSTANT(LED_MACRO_ARG_PASS(__VA_ARGS__), _on_period), \
	LED_EFFECT_STEP_EASING_STEP_INSTANT(LED_COLOR_OFF, _off_period),		\

/** Create LED counter effect initializer.
 *
 * LED counts the number of times the effect is repeated.
 *
 * @param _count    Number of times the effect should repeat. 0 is forever.
 * @param _delay	Delay before the effect starts.
 * @param _on_period	Period of time for a cycle.
 * @param _off_period   Period of time for which LED is off.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_COUNTER2_DELAY(_count, _delay, _on_period, _off_period, _color) \
	LED_EFFECT(1, 1 + (_count) * 2, \
		LED_EFFECT_STEP_EASING_STEP_INSTANT(LED_COLOR_OFF, _delay), \
		LISTIFY(_count, _LED_EFFECT_LED_COUNTER2_DELAY_STEP, (), \
			_on_period, _off_period, LED_MACRO_ARG_PASS(_color)) \
	)

/** Create LED counter effect initializer.
 *
 * LED counts the number of times the effect is repeated.
 *
 * @param _count    Number of times the effect should repeat. 0 is forever.
 * @param _on_period	Period of time for a cycle.
 * @param _off_period   Period of time for which LED is off.
 * @param _color	Selected LED color.
 * @param _easing   Easing function to be used.
 */
#define LED_EFFECT_LED_COUNTER2(_count, _on_period, _off_period, _easing, _color) \
	LED_EFFECT(_count, 3, \
		LED_EFFECT_STEP_INSTANT(LED_COLOR_OFF), \
		LED_EFFECT_STEP(LED_MACRO_ARG_PASS(_color), _on_period, \
			LED_MACRO_ARG_PASS(_easing)), \
		LED_EFFECT_STEP_EASING_STEP_INSTANT(LED_COLOR_OFF, _off_period), \
	)

/** Create LED breathing effect initializer.
 *
 * LED color is smoothly, gradually changed between the LED turned off
 * and the selected color.
 *
 * @param _count    Number of times the effect should repeat. 0 is forever.
 * @param _on_period	Period of time for a breath.
 * @param _off_period   Period of time for which LED is off.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_BREATH_COUNTER2(_count, _on_period, _off_period, _color) \
	LED_EFFECT_LED_COUNTER2(_count, _on_period, _off_period, \
		LED_MACRO_ARG_PASS(LED_EFFECT_EASING_SIN_START_ZERO(LED_EFFECT_OUTPUT_MAX)), \
		LED_MACRO_ARG_PASS(_color))

/** Create LED breathing effect initializer.
 *
 * LED color is smoothly, gradually changed between the LED turned off
 * and the selected color.
 *
 * @param _count    Number of times the effect should repeat. 0 is forever.
 * @param _period	Period of time for a breath.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_BREATH_COUNTER(_count, _period, _color) \
	LED_EFFECT_LED_BREATH_COUNTER2(_count, _period, 0, \
		LED_MACRO_ARG_PASS(_color))

/** Create LED breathing effect initializer.
 *
 * LED color is smoothly, gradually changed between the LED turned off
 * and the selected color.
 *
 * @param _on_period	Period of time for a cycle.
 * @param _off_period   Period of time for which LED is off.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_BREATH2(_on_period, _off_period, _color) \
	LED_EFFECT_LED_BREATH_COUNTER2(0, _on_period, _off_period, LED_MACRO_ARG_PASS(_color))

/** Create LED breathing effect initializer.
 *
 * LED color is smoothly, gradually changed between the LED turned off
 * and the selected color using sinusoidal easing function.
 *
 * @param _period	Period of time for single substep.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_BREATH_SIN(_period, _color) \
	LED_EFFECT_LED_BREATH_COUNTER(0, _period, \
		LED_MACRO_ARG_PASS(_color))

/** Create LED breathing effect initializer, with linear easing.
 *
 * LED color is smoothly, gradually changed between the LED turned off
 * and the selected color over _period time, stays at that color for
 * _period time, changes back to off over _period time, and stays off
 * for _period time.
 *
 * @param _period	Period of time for each step.
 * @param _color	Selected LED color.
 */
#define LED_EFFECT_LED_BREATH(_period, _color) \
	LED_EFFECT(0, 4, \
		LED_EFFECT_STEP_EASING_LINEAR(LED_MACRO_ARG_PASS(_color), _period), \
		LED_EFFECT_STEP_EASING_STEP_INSTANT(LED_MACRO_ARG_PASS(_color), _period), \
		LED_EFFECT_STEP_EASING_LINEAR(LED_COLOR_OFF, _period), \
		LED_EFFECT_STEP_EASING_STEP_INSTANT(LED_COLOR_OFF, _period))

/** Create LED blinking effect initializer with two periods as arguments.
 *
 * LED color is periodically changed between the selected color and the LED
 * turned off.
 * This macro takes two periods: for on and off time.
 * It counts up to the given number of times.
 *
 * @param _count	Number of times the effect should repeat. 0 is forever.
 * @param _period_on	Period of time for which LED is on.
 * @param _period_off	Period of time for which LED is off.
 * @param _color	Selected LED color.
 *
 * @sa LED_EFFECT_LED_BLINK
 */
#define LED_EFFECT_LED_BLINK2_COUNTER(_count, _period_on, _period_off, _color) \
	LED_EFFECT_LED_COUNTER2(_count, _period_on, _period_off, \
		LED_MACRO_ARG_PASS(LED_EFFECT_EASING_STEP_INSTANT()), \
		LED_MACRO_ARG_PASS(_color))

/** Create LED blinking effect initializer with one period given
 *
 * LED color is periodically changed between the selected color and the LED
 * turned off.
 * The same time is used for both: on and off time.
 *
 * @param _count	Number of times the effect should repeat. 0 is forever.
 * @param _period	Period of time between LED color switches.
 * @param _color	Selected LED color.
 *
 * @sa LED_EFFECT_LED_BLINK
 */
#define LED_EFFECT_LED_BLINK_COUNTER(_count, _period, _color) \
	LED_EFFECT_LED_BLINK2_COUNTER(_count, _period, _period, LED_MACRO_ARG_PASS(_color))

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
	LED_EFFECT_LED_COUNTER2(0, _period_on, _period_off, \
		LED_MACRO_ARG_PASS(LED_EFFECT_EASING_STEP_INSTANT()), \
		LED_MACRO_ARG_PASS(_color))

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
	LED_EFFECT_LED_BLINK2(_period, _period, LED_MACRO_ARG_PASS(_color))

/** Create LED blinking effect initializer with two periods as arguments, with delayed on
 *
 * LED color is periodically changed between the selected color and the LED
 * turned off.
 * This macro takes two periods: for on and off time.
 * The LED is off initially for _period_off time.
 *
 * @param _count	Number of times the effect should repeat. 0 is forever.
 * @param _on_time	Period of time for which LED is on.
 * @param _off_time	Period of time for which LED is off.
 * @param _color	Selected LED color.
 *
 * @sa LED_EFFECT_LED_BLINK
 */
#define LED_EFFECT_BLINK2_DELAY(_count, _on_time, _off_time, _color) \
	LED_EFFECT(_count, 3, \
		LED_EFFECT_STEP_INSTANT(LED_COLOR_OFF), \
		LED_EFFECT_STEP_EASING_STEP_DELAYED(LED_MACRO_ARG_PASS(_color), _off_time), \
		LED_EFFECT_STEP_EASING_STEP_DELAYED(LED_COLOR_OFF, _on_time), \
	)

/** Create LED fade effect initializer.
 *
 * LED color fades from start color to end color linearly.
 *
 * @param _fade_time	Time in milliseconds for the fade effect.
 * @param _start_color	Starting LED color.
 * @param _end_color	Ending LED color.
 */
#define LED_EFFECT_FADE(_fade_time, _start_color, _end_color) \
	LED_EFFECT(1, 2, \
		LED_EFFECT_STEP_INSTANT(LED_MACRO_ARG_PASS(_start_color)), \
		LED_EFFECT_STEP_EASING_LINEAR(LED_MACRO_ARG_PASS(_end_color), \
			_fade_time) \
	)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LED_EFFECT_H_ */
