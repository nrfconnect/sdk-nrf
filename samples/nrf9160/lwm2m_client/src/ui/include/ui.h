/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

#ifdef CONFIG_UI_LED_USE_PWM

#define UI_LED_ON_PERIOD_NORMAL		500
#define UI_LED_OFF_PERIOD_NORMAL	5000
#define UI_LED_ON_PERIOD_ERROR		500
#define UI_LED_OFF_PERIOD_ERROR		500

#define UI_LED_MAX			50

#define UI_LED_COLOR_OFF		LED_COLOR(0, 0, 0)
#define UI_LED_COLOR_RED		LED_COLOR(UI_LED_MAX, 0, 0)
#define UI_LED_COLOR_GREEN		LED_COLOR(0, UI_LED_MAX, 0)
#define UI_LED_COLOR_BLUE		LED_COLOR(0, 0, UI_LED_MAX)
#define UI_LED_COLOR_WHITE		LED_COLOR(UI_LED_MAX, UI_LED_MAX,      \
						  UI_LED_MAX)
#define UI_LED_COLOR_YELLOW		LED_COLOR(UI_LED_MAX, UI_LED_MAX, 0)
#define UI_LED_COLOR_CYAN		LED_COLOR(0, UI_LED_MAX, UI_LED_MAX)
#define UI_LED_COLOR_PURPLE		LED_COLOR(UI_LED_MAX, 0, UI_LED_MAX)

#define UI_LTE_DISCONNECTED_COLOR	UI_LED_COLOR_OFF
#define UI_LTE_CONNECTING_COLOR		UI_LED_COLOR_BLUE
#define UI_LTE_CONNECTED_COLOR		UI_LED_COLOR_BLUE
#define UI_CLOUD_CONNECTING_COLOR	UI_LED_COLOR_BLUE
#define UI_CLOUD_CONNECTED_COLOR	UI_LED_COLOR_GREEN
#define UI_CLOUD_PAIRING_COLOR		UI_LED_COLOR_YELLOW
#define UI_ACCEL_CALIBRATING_COLOR	UI_LED_COLOR_PURPLE
#define UI_LED_ERROR_CLOUD_COLOR	UI_LED_COLOR_RED
#define UI_LED_ERROR_MODEM_REC_COLOR	UI_LED_COLOR_RED
#define UI_LED_ERROR_MODEM_IRREC_COLOR	UI_LED_COLOR_RED
#define UI_LED_ERROR_LTE_LC_COLOR	UI_LED_COLOR_RED
#define UI_LED_ERROR_UNKNOWN_COLOR	UI_LED_COLOR_WHITE

#else

#define UI_LED_ON_PERIOD_NORMAL		500
#define UI_LED_OFF_PERIOD_NORMAL	500

#endif /* CONFIG_UI_LED_USE_PWM */

/**@brief UI LED state pattern definitions. */
enum ui_led_pattern {
#ifdef CONFIG_UI_LED_USE_PWM
	UI_LTE_DISCONNECTED,
	UI_LTE_CONNECTING,
	UI_LTE_CONNECTED,
	UI_CLOUD_CONNECTING,
	UI_CLOUD_CONNECTED,
	UI_CLOUD_PAIRING,
	UI_ACCEL_CALIBRATING,
	UI_LED_ERROR_CLOUD,
	UI_LED_ERROR_MODEM_REC,
	UI_LED_ERROR_MODEM_IRREC,
	UI_LED_ERROR_LTE_LC,
	UI_LED_ERROR_UNKNOWN,
#else /* LED patterns without using PWM. */
	UI_LTE_DISCONNECTED		= UI_LED_ON(0),
	UI_LTE_CONNECTING		= UI_LED_BLINK(DK_LED3_MSK),
	UI_LTE_CONNECTED		= UI_LED_ON(DK_LED3_MSK),
	UI_CLOUD_CONNECTING		= UI_LED_BLINK(DK_LED4_MSK),
	UI_CLOUD_CONNECTED		= UI_LED_ON(DK_LED4_MSK),
	UI_CLOUD_PAIRING		= UI_LED_BLINK(DK_LED3_MSK | DK_LED4_MSK),
	UI_ACCEL_CALIBRATING		= UI_LED_ON(DK_LED1_MSK | DK_LED3_MSK),
	UI_LED_ERROR_CLOUD		= UI_LED_BLINK(DK_LED1_MSK | DK_LED4_MSK),
	UI_LED_ERROR_MODEM_REC		= UI_LED_BLINK(DK_LED1_MSK | DK_LED3_MSK),
	UI_LED_ERROR_MODEM_IRREC	= UI_LED_BLINK(DK_ALL_LEDS_MSK),
	UI_LED_ERROR_LTE_LC		= UI_LED_BLINK(DK_LED1_MSK | DK_LED2_MSK),
	UI_LED_ERROR_UNKNOWN		= UI_LED_ON(DK_ALL_LEDS_MSK)
#endif /* CONFIG_UI_LED_USE_PWM */
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
		uint32_t button;
	};
};

/**
 * @brief UI callback handler type definition.
 *
 * @param evt Pointer to event struct.
 */
typedef void (*ui_callback_t)(struct ui_evt *evt);

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
 *	  LED pattern is preserved. Only has effect if the specified LED is not
 *	  controlled by PWM.
 *
 * @param led LED number to be controlled.
 * @param value 0 turns the LED off, a non-zero value turns the LED on.
 */
void ui_led_set_state(uint32_t led, uint8_t value);

/**
 * @brief Gets the LED pattern.
 *
 * @return Current LED pattern.
 */
enum ui_led_pattern ui_led_get_pattern(void);

/**
 * @brief Sets the LED RGB color.
 *
 * @param red Red, in range 0 - 255.
 * @param green Green, in range 0 - 255.
 * @param blue Blue, in range 0 - 255.
 *
 * @return 0 on success or negative error value on failure.
 */
int ui_led_set_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Get the state of a button.
 *
 * @param button Button number.
 *
 * @return 1 if button is active, 0 if it's inactive.
 */
bool ui_button_is_active(uint32_t button);

/**
 * @brief Set the buzzer frequency.
 *
 * @param frequency Frequency. If set to 0, the buzzer is disabled.
 *		    The frequency is limited to the range 100 - 10 000 Hz.
 * @param intensity Intensity of the buzzer output. If set to 0, the buzzer is
 *		    disabled.
 *		    The frequency is limited to the range 0 - 100 %.
 *
 * @return 0 on success or negative error value on failure.
 */
int ui_buzzer_set_frequency(uint32_t frequency, uint8_t intensity);

#ifdef __cplusplus
}
#endif

#endif /* UI_H__ */
