/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _LED_CONTROL_H_
#define _LED_CONTROL_H_


enum led_pattern {LED_DISABLED, LED_IDLE, LED_WAITING, LED_FAILURE, LED_SUCCESS};

/**
 * @brief Show an LED pattern for a specified number of animation frames, then go idle.
 *
 * @param frames - The number of frames for which the pattern should be visible.
		 - Negative if forever.
 * @param pattern - The LED pattern to show.
 */
void start_led_pattern(int frames, enum led_pattern pattern);

/**
 * @brief Show an LED pattern for a few seconds, then go idle.
 *
 * @param pattern - The LED pattern to show
 */
void short_led_pattern(enum led_pattern pattern);

/**
 * @brief Show an LED pattern until further notice
 *
 * @param pattern - The LED pattern to show
 */
void long_led_pattern(enum led_pattern pattern);

/**
 * @brief Stop whatever LED pattern is currently being shown.
 */
void stop_led_pattern(void);

/**
 * @brief The LED animation thread function.
 * Run the LED management/animation process.
 */
void led_animation_thread_fn(void);

#endif /* _LED_CONTROL_H_ */
