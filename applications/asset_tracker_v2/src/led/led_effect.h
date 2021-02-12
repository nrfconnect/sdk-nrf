/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LED_EFFECT_H__
#define LED_EFFECT_H__

/**
 * @brief LED Effect
 * @defgroup led_effect LED Effect
 * @{
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct led_color {
	uint8_t c[3];
};

struct led_effect_step {
	struct led_color color;
	uint16_t substep_cnt;
	uint16_t substep_time;
};

struct led_effect {
	struct led_effect_step *steps;
	uint16_t step_cnt;
	bool loop_forever;
};

#define LED_COLOR(_r, _g, _b)                                                  \
	{                                                                      \
		.c = { _r, _g, _b }                                            \
	}
#define LED_NOCOLOR()                                                          \
	{                                                                      \
		.c = { 0, 0, 0 }                                               \
	}

#define LED_EFFECT_LED_ON(_color)                                              \
	{                                                                      \
		.steps = ((const struct led_effect_step[]){                    \
			{                                                      \
				.color = _color,                               \
				.substep_cnt = 1,                              \
				.substep_time = 0,                             \
			},                                                     \
		}),                                                            \
		.step_cnt = 1, .loop_forever = false,                          \
	}

#define LED_EFFECT_LED_OFF() LED_EFFECT_LED_ON(LED_NOCOLOR())

#define LED_EFFECT_LED_BLINK(_period, _color)                                  \
	{                                                                      \
		.steps = ((const struct led_effect_step[]){                    \
			{                                                      \
				.color = _color,                               \
				.substep_cnt = 1,                              \
				.substep_time = (_period),                     \
			},                                                     \
			{                                                      \
				.color = LED_NOCOLOR(),                        \
				.substep_cnt = 1,                              \
				.substep_time = (_period),                     \
			},                                                     \
		}),                                                            \
		.step_cnt = 2, .loop_forever = true,                           \
	}

#define _BREATH_SUBSTEPS 15
#define _BREATH_PAUSE_SUBSTEPS 1
#define LED_EFFECT_LED_BREATHE(_period, _pause, _color)                        \
	{                                                                      \
		.steps = ((struct led_effect_step[]){                          \
			{                                                      \
				.color = _color,                               \
				.substep_cnt = _BREATH_SUBSTEPS,               \
				.substep_time =                                \
					((_period + (_BREATH_SUBSTEPS - 1)) /  \
					 _BREATH_SUBSTEPS),                    \
			},                                                     \
			{                                                      \
				.color = _color,                               \
				.substep_cnt = 1,                              \
				.substep_time = _period,                       \
			},                                                     \
			{                                                      \
				.color = LED_NOCOLOR(),                        \
				.substep_cnt = _BREATH_SUBSTEPS,               \
				.substep_time =                                \
					((_period + (_BREATH_SUBSTEPS - 1)) /  \
					 _BREATH_SUBSTEPS),                    \
			},                                                     \
			{                                                      \
				.color = LED_NOCOLOR(),                        \
				.substep_cnt = _BREATH_PAUSE_SUBSTEPS,         \
				.substep_time = _pause,                        \
			},                                                     \
		}),                                                            \
		.step_cnt = 4, .loop_forever = true,                           \
	}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* LED_EFFECT_H_ */
