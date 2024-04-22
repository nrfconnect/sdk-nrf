/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hid_keyboard_leds.h"

/* This configuration file is included only once from hid_state module and holds
 * information about LEDs associated with HID keyboard LEDs report.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} hid_keyboard_leds_def_include_once;

/* On the nRF54L15 SoC, you can only use the **GPIO1** port for PWM hardware peripheral output.
 * Because of that, the PDK PCA10156 has the following limitations:
 *
 * - On the PDK revision v0.2.1, **LED 1** cannot be used for PWM output.
 * - On the PDK revision v0.3.0, **LED 0** and **LED 2** cannot be used for PWM output.
 *
 * You can still use these LEDs with the PWM LED driver, but you must set the LED color to
 * ``LED_COLOR(255, 255, 255)`` or ``LED_COLOR(0, 0, 0)``. This ensures the PWM peripheral is not
 * used for the mentioned LEDs.
 */
static const struct led_effect keyboard_led_on = LED_EFFECT_LED_ON(LED_COLOR(255, 255, 255));
static const struct led_effect keyboard_led_off = LED_EFFECT_LED_OFF();

/* Map HID keyboard LEDs to application LED IDs. */
static const uint8_t keyboard_led_map[] = {
	[HID_KEYBOARD_LEDS_NUM_LOCK] = 2,
	[HID_KEYBOARD_LEDS_CAPS_LOCK] = LED_UNAVAILABLE,
	[HID_KEYBOARD_LEDS_SCROLL_LOCK] = LED_UNAVAILABLE,
	[HID_KEYBOARD_LEDS_COMPOSE] = LED_UNAVAILABLE,
	[HID_KEYBOARD_LEDS_KANA] = LED_UNAVAILABLE,
};
