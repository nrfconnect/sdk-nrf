/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @defgroup audio_app_led Audio LED Control
 * @{
 * @brief LED control API for Audio applications.
 *
 * This module provides LED control functionality for user interface and status indication.
 */

#ifndef _LED_H_
#define _LED_H_

#include <stdint.h>

#define RED   0

#define GREEN 1

#define BLUE  2

#define GRN GREEN

#define BLU BLUE

/**
 * @brief LED color enumeration for RGB LED control.
 *
 * This enumeration defines the available colors for RGB LED control,
 * including individual colors and combinations for visual status indication.
 */
enum led_color {
	LED_COLOR_OFF,
	LED_COLOR_RED,
	LED_COLOR_GREEN,
	LED_COLOR_YELLOW,
	LED_COLOR_BLUE,
	LED_COLOR_MAGENTA,
	LED_COLOR_CYAN,
	LED_COLOR_WHITE,
	LED_COLOR_NUM,
};

#define LED_ON LED_COLOR_WHITE

#define LED_BLINK true

#define LED_SOLID false

/**
 * @brief Set the state of a given LED unit to blink.
 *
 * @note A led unit is defined as an RGB LED or a monochrome LED.
 *
 * @param led_unit	Selected LED unit. Defines are located in board.h.
 * @note		If the given LED unit is an RGB LED, color must be
 *			provided as a single vararg. See led_color.
 *			For monochrome LEDs, the vararg will be ignored.
 *			Using a LED unit assigned to another core will do nothing and return 0.
 * @return		0 on success.
 *			-EPERM if the module has not been initialized.
 *			-EINVAL if the color argument is illegal.
 *			Other errors from underlying drivers.
 */
int led_blink(uint8_t led_unit, ...);

/**
 * @brief Turn the given LED unit on.
 *
 * @note A led unit is defined as an RGB LED or a monochrome LED.
 *
 * @param led_unit	Selected LED unit. Defines are located in board.h.
 * @note		If the given LED unit is an RGB LED, color must be
 *			provided as a single vararg. See led_color.
 *			For monochrome LEDs, the vararg will be ignored.
 *			Using a LED unit assigned to another core will do nothing and return 0.
 * @return		0 on success.
 *			-EPERM if the module has not been initialized.
 *			-EINVAL if the color argument is illegal.
 *			Other errors from underlying drivers.
 */
int led_on(uint8_t led_unit, ...);

/**
 * @brief Set the state of a given LED unit to off.
 *
 * @note A led unit is defined as an RGB LED or a monochrome LED.
 *		Using a LED unit assigned to another core will do nothing and return 0.
 *
 * @param led_unit	Selected LED unit. Defines are located in board.h.
 * @return		0 on success.
 *			-EPERM if the module has not been initialized.
 *			-EINVAL if the color argument is illegal.
 *			Other errors from underlying drivers.
 */
int led_off(uint8_t led_unit);

/**
 * @brief Initialise the LED module.
 *
 * @note This will parse the .dts files and configure all LEDs.
 *
 * @return	0 on success.
 *		-EPERM if already initialized.
 *		-ENXIO if a LED is missing unit number in dts.
 *		-ENODEV if a LED is missing color identifier.
 */
int led_init(void);

/**
 * @}
 */

#endif /* _LED_H_ */
