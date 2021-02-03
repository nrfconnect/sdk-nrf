/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *
 * @brief   LED module.
 *
 * Module that handles LED behaviour.
 */

#ifndef LED_H__
#define LED_H__

#include <zephyr.h>
#include <dk_buttons_and_leds.h>

#include "led_effect.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LED_1 1
#define LED_2 2
#define LED_3 3
#define LED_4 4

#define LED_ON(x)		(x)
#define LED_BLINK(x)		((x) << 8)
#define LED_GET_ON(x)		((x)&0xFF)
#define LED_GET_BLINK(x)	(((x) >> 8) & 0xFF)

#if defined(CONFIG_LED_USE_PWM)

#define LED_ON_PERIOD_NORMAL	500
#define LED_OFF_PERIOD_NORMAL	3500
#define LED_ON_PERIOD_ERROR	500
#define LED_OFF_PERIOD_ERROR	500
#define LED_ON_PERIOD_SHORT	350
#define LED_OFF_PERIOD_LONG	7000

#define LED_MAX 50

#define LED_COLOR_OFF		LED_COLOR(0, 0, 0)
#define LED_COLOR_RED		LED_COLOR(LED_MAX, 0, 0)
#define LED_COLOR_GREEN		LED_COLOR(0, LED_MAX, 0)
#define LED_COLOR_BLUE		LED_COLOR(0, 0, LED_MAX)
#define LED_COLOR_YELLOW	LED_COLOR(LED_MAX, LED_MAX, 0)
#define LED_COLOR_CYAN		LED_COLOR(0, LED_MAX, LED_MAX)
#define LED_COLOR_PURPLE	LED_COLOR(LED_MAX, 0, LED_MAX)

#define LED_LTE_CONNECTING_COLOR	LED_COLOR_YELLOW
#define LED_CLOUD_PUBLISHING_COLOR	LED_COLOR_GREEN
#define LED_GPS_SEARCHING_COLOR		LED_COLOR_PURPLE
#define LED_ACTIVE_MODE_COLOR		LED_COLOR_CYAN
#define LED_PASSIVE_MODE_COLOR		LED_COLOR_BLUE
#define LED_ERROR_SYSTEM_FAULT_COLOR	LED_COLOR_RED

#else

#define LED_ON_PERIOD_NORMAL	500
#define LED_OFF_PERIOD_NORMAL	500
#define LED_OFF_PERIOD_LONG	3500

#endif /* CONFIG_LED_USE_PWM */

/**@brief LED state pattern definitions. */
enum led_pattern {
#if defined(CONFIG_LED_USE_PWM)
	LED_LTE_CONNECTING,
	LED_CLOUD_PUBLISHING,
	LED_GPS_SEARCHING,
	LED_ACTIVE_MODE,
	LED_PASSIVE_MODE,
	LED_ERROR_SYSTEM_FAULT,
#else /* LED patterns without using PWM. */
	LED_LTE_CONNECTING = LED_BLINK(DK_LED1_MSK),
	LED_GPS_SEARCHING = LED_BLINK(DK_LED2_MSK),
	LED_CLOUD_PUBLISHING = LED_BLINK(DK_LED3_MSK),
	LED_ACTIVE_MODE = LED_BLINK(DK_LED4_MSK),
	LED_PASSIVE_MODE = LED_BLINK(DK_LED4_MSK),
	LED_ERROR_SYSTEM_FAULT = LED_BLINK(DK_ALL_LEDS_MSK),
#endif /* CONFIG_LED_USE_PWM */
};

/**
 * @brief Sets the LED pattern.
 *
 * @param pattern LED pattern.
 */
void led_set_pattern(enum led_pattern pattern);

/**
 * @brief Initialize LED library.
 *
 * @return 0 on success or negative error value on failure.
 */
int led_init(void);

/**
 * @brief Gets the LED pattern.
 *
 * @return Current LED pattern.
 */
enum led_pattern led_get_pattern(void);

/**
 * @brief Sets the LED RGB color.
 *
 * @param red Red, in range 0 - 255.
 * @param green Green, in range 0 - 255.
 * @param blue Blue, in range 0 - 255.
 *
 * @return 0 on success or negative error value on failure.
 */
int led_set_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Stops leds
 */
void led_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_H__ */
