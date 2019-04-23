/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _HID_REPORT_DESC_H_
#define _HID_REPORT_DESC_H_

#include <stddef.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REPORT_SIZE_MOUSE		5 /* bytes */
#define REPORT_SIZE_MOUSE_BOOT		3 /* bytes */
#define REPORT_SIZE_KEYBOARD_KEYS	8 /* bytes */
#define REPORT_SIZE_KEYBOARD_LEDS	1 /* bytes */
#define REPORT_SIZE_MPLAYER		1 /* bytes */
#define REPORT_SIZE_USER_CONFIG		28 /* bytes */

#define USAGE_PAGE_MOUSE_XY		0x01
#define USAGE_PAGE_MOUSE_WHEEL		0x01
#define USAGE_PAGE_KEYBOARD		0x07
#define USAGE_PAGE_LEDS			0x08
#define USAGE_PAGE_MOUSE_BUTTONS	0x09
#define USAGE_PAGE_MPLAYER		0x0C

#define REPORT_MOUSE_WHEEL_MIN		(-0x7F)
#define REPORT_MOUSE_WHEEL_MAX		(0x7F)
#define REPORT_MOUSE_XY_MIN		(-0x07ff)
#define REPORT_MOUSE_XY_MAX		(0x07ff)
#define REPORT_MOUSE_XY_MIN_BOOT	(-0x80)
#define REPORT_MOUSE_XY_MAX_BOOT	(0x7f)

#define KEYBOARD_REPORT_LAST_KEY	0x65 /* Keyboard Application */
#define KEYBOARD_REPORT_FIRST_MODIFIER	0xE0 /* Keyboard Left Ctrl */
#define KEYBOARD_REPORT_LAST_MODIFIER	0xE7 /* Keyboard Right GUI */

enum report_id {
	REPORT_ID_RESERVED,
	REPORT_ID_MOUSE,
	REPORT_ID_KEYBOARD_KEYS,
	REPORT_ID_KEYBOARD_LEDS,
	REPORT_ID_MPLAYER,
	REPORT_ID_USER_CONFIG,

	REPORT_ID_COUNT
};

extern const u8_t hid_report_desc[];
extern const size_t hid_report_desc_size;

#ifdef __cplusplus
}
#endif

#endif /* _HID_REPORT_DESC_H_ */
