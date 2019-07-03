/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _LED_EFFECT_H_
#define _LED_EFFECT_H_

/**
 * @brief LED Effect
 * @defgroup led_effect LED Effect
 * @{
 */

#include <zephyr/types.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct led_color {
	u8_t c[CONFIG_DESKTOP_LED_COLOR_COUNT];
};

struct led_effect_step {
	struct led_color color;
	u16_t substep_count;
	u16_t substep_time;
};

struct led_effect {
	const struct led_effect_step *steps;
	u16_t step_count;
	bool loop_forever;
};


#if CONFIG_DESKTOP_LED_COLOR_COUNT == 1
	#define LED_COLOR(_brightness) {	\
			.c = {_brightness}	\
		}
	#define LED_NOCOLOR() {			\
			.c = {0}		\
		}
#elif CONFIG_DESKTOP_LED_COLOR_COUNT == 3
	#define LED_COLOR(_r, _g, _b) {		\
			.c = {_r, _g, _b}	\
	}
	#define LED_NOCOLOR() {			\
			.c = {0, 0, 0}		\
	}
#else
	#error "Unsupported color count"
#endif

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

#define LED_EFFECT_LED_OFF() LED_EFFECT_LED_ON(LED_NOCOLOR())

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

#define _BREATH_SUBSTEPS 15
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


#define LED_CLOCK_BLINK_PERIOD 200
#define LED_CLOCK_SLEEP_PERIOD 1000
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

/* UTIL_LISTIFY accepts just one additional argument - period is defined
 * separately.
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
