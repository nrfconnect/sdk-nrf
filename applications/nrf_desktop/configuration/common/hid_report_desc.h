/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _HID_REPORT_DESC_H_
#define _HID_REPORT_DESC_H_

#include <stddef.h>
#include <zephyr/types.h>
#include <toolchain/common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REPORT_SIZE_MOUSE		5 /* bytes */
#define REPORT_SIZE_MOUSE_BOOT		3 /* bytes */
#define REPORT_SIZE_KEYBOARD_KEYS	8 /* bytes */
#define REPORT_SIZE_KEYBOARD_LEDS	1 /* bytes */
#define REPORT_SIZE_CTRL		2 /* bytes */
#define REPORT_SIZE_USER_CONFIG		29 /* bytes */

#define USAGE_PAGE_MOUSE_XY		0x01
#define USAGE_PAGE_MOUSE_WHEEL		0x01
#define USAGE_PAGE_KEYBOARD		0x07
#define USAGE_PAGE_LEDS			0x08
#define USAGE_PAGE_MOUSE_BUTTONS	0x09
#define USAGE_PAGE_CONSUMER_CTRL	0x0C

#define MOUSE_REPORT_WHEEL_MIN		(-0x7F)
#define MOUSE_REPORT_WHEEL_MAX		(0x7F)
#define MOUSE_REPORT_XY_MIN		(-0x07ff)
#define MOUSE_REPORT_XY_MAX		(0x07ff)
#define MOUSE_REPORT_XY_MIN_BOOT	(-0x80)
#define MOUSE_REPORT_XY_MAX_BOOT	(0x7f)
#define MOUSE_REPORT_BUTTON_COUNT_MAX	8

#define KEYBOARD_REPORT_LAST_KEY	0x65 /* Keyboard Application */
#define KEYBOARD_REPORT_FIRST_MODIFIER	0xE0 /* Keyboard Left Ctrl */
#define KEYBOARD_REPORT_LAST_MODIFIER	0xE7 /* Keyboard Right GUI */
#define KEYBOARD_REPORT_KEY_COUNT_MAX	6

#define CTRL_REPORT_KEY_COUNT_MAX	1

#define REPORT_ID_TO_IN_REPORT(report_id) (report_id - 1)
#define IN_REPORT_TO_REPORT_ID(in_report) (in_report + 1)

#define IN_REPORT_LIST		\
	X(MOUSE)		\
	X(KEYBOARD_KEYS)	\
	X(SYSTEM_CTRL)		\
	X(CONSUMER_CTRL)

#define OUT_REPORT_LIST		\
	X(KEYBOARD_LEDS)

#define FEATURE_REPORT_LIST	\
	X(USER_CONFIG)

enum in_report {
#define X(name) _CONCAT(IN_REPORT_, name),
	IN_REPORT_LIST
#undef X

	IN_REPORT_COUNT
};

enum out_report {
#define X(name) _CONCAT(OUT_REPORT_, name),
	OUT_REPORT_LIST
#undef X

	OUT_REPORT_COUNT
};

enum feature_report {
#define X(name) _CONCAT(FEATURE_REPORT_, name),
	FEATURE_REPORT_LIST
#undef X

	FEATURE_REPORT_COUNT
};


enum report_id {
	REPORT_ID_RESERVED,

#define X(name) _CONCAT(REPORT_ID_, name),
	IN_REPORT_LIST
	OUT_REPORT_LIST
	FEATURE_REPORT_LIST
#undef X

	REPORT_ID_COUNT
};

enum report_mode {
	REPORT_MODE_PROTOCOL,
	REPORT_MODE_BOOT,

	REPORT_MODE_COUNT
};


extern const u8_t hid_report_desc[];
extern const size_t hid_report_desc_size;

#ifdef __cplusplus
}
#endif

#endif /* _HID_REPORT_DESC_H_ */
