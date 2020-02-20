/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file
 *
 * @brief   User interface module.
 *
 * Module that handles user interaction through button, RGB LED and buzzer.
 */

#ifndef UI_H__
#define UI_H__

#include <zephyr.h>
#include <dk_buttons_and_leds.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_BUTTON_1			1
#define UI_BUTTON_2			2
#define UI_SWITCH_1			3
#define UI_SWITCH_2			4

#define UI_LED_1			1
#define UI_LED_2			2
#define UI_LED_3			3
#define UI_LED_4			4

#define UI_LED_ON(x)			(x)
#define UI_LED_BLINK(x)			((x) << 8)
#define UI_LED_GET_ON(x)		((x) & 0xFF)
#define UI_LED_GET_BLINK(x)		(((x) >> 8) & 0xFF)

#define UI_LED_ON_PERIOD_NORMAL		500
#define UI_LED_OFF_PERIOD_NORMAL	500

/**@brief UI LED state pattern definitions. */
enum ui_led_pattern {
	UI_LTE_DISCONNECTED	= UI_LED_ON(0),
	UI_LTE_CONNECTING	= UI_LED_BLINK(DK_LED3_MSK),
	UI_LTE_CONNECTED	= UI_LED_ON(DK_LED3_MSK),
	UI_CLOUD_CONNECTING	= UI_LED_BLINK(DK_LED4_MSK),
	UI_CLOUD_CONNECTED	= UI_LED_ON(DK_LED4_MSK),
	UI_CLOUD_PAIRING	= UI_LED_BLINK(DK_LED3_MSK | DK_LED4_MSK),
	UI_LED_ERROR_CLOUD	= UI_LED_BLINK(DK_LED1_MSK | DK_LED4_MSK),
	UI_LED_ERROR_BSD_REC	= UI_LED_BLINK(DK_LED1_MSK | DK_LED3_MSK),
	UI_LED_ERROR_BSD_IRREC	= UI_LED_BLINK(DK_ALL_LEDS_MSK),
	UI_LED_ERROR_LTE_LC	= UI_LED_BLINK(DK_LED1_MSK | DK_LED2_MSK),
	UI_LED_ERROR_UNKNOWN	= UI_LED_ON(DK_ALL_LEDS_MSK)
};

/**@brief UI event types. */
enum ui_evt_type {
	UI_EVT_BUTTON_ACTIVE,
	UI_EVT_BUTTON_INACTIVE
};

/**@brief UI event structure. */
struct ui_evt {
	enum ui_evt_type type;

	union {
		u32_t button;
	};
};

/**
 * @brief UI callback handler type definition.
 *
 * @param evt Pointer to event struct.
 */
typedef void (*ui_callback_t)(struct ui_evt evt);

/**
 * @brief Initializes the user interface module.
 *
 * @param cb UI callback handler. Can be NULL to disable callbacks.
 *
 * @return 0 on success or negative error value on failure.
 */
int ui_init(ui_callback_t cb);

/**
 * @brief Sets the LED pattern.
 *
 * @param pattern LED pattern.
 */
void ui_led_set_pattern(enum ui_led_pattern pattern);

/**
 * @brief Sets a LED's state. Only the one LED is affected, the rest of the
 *	  LED pattern is preserved.
 *
 * @param led LED number to be controlled.
 * @param value 0 turns the LED off, a non-zero value turns the LED on.
 */
void ui_led_set_state(u32_t led, u8_t value);

/**
 * @brief Gets the LED pattern.
 *
 * @return Current LED pattern.
 */
enum ui_led_pattern ui_led_get_pattern(void);

/**
 * @brief Get the state of a button.
 *
 * @param button Button number.
 *
 * @return 1 if button is active, 0 if it's inactive.
 */
bool ui_button_is_active(u32_t button);

#ifdef __cplusplus
}
#endif

#endif /* UI_H__ */
