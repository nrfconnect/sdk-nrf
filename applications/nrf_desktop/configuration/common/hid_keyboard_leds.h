/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HID_KEYBOARD_LEDS_H_
#define _HID_KEYBOARD_LEDS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LED_UNAVAILABLE 0xFF

/** @brief LEDs represented by subsequent bits in HID keyboard LED report. */
enum {
	HID_KEYBOARD_LEDS_NUM_LOCK,
	HID_KEYBOARD_LEDS_CAPS_LOCK,
	HID_KEYBOARD_LEDS_SCROLL_LOCK,
	HID_KEYBOARD_LEDS_COMPOSE,
	HID_KEYBOARD_LEDS_KANA,

	HID_KEYBOARD_LEDS_COUNT
};

#ifdef __cplusplus
}
#endif

#endif /* _HID_KEYBOARD_LEDS_H_ */
