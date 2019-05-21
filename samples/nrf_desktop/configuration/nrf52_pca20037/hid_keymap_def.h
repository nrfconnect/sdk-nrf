/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "hid_keymap.h"
#include "key_id.h"

/* This configuration file is included only once from hid_state module and holds
 * information about mapping between buttons and generated reports.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} hid_keymap_def_include_once;

/*
 * HID keymap. The Consumer Control keys are defined in section 15 of
 * the HID Usage Tables document under the following URL:
 * http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 */
static const struct hid_keymap hid_keymap[] = {
	{ KEY_ID(0x00, 0x01), 0x14, TARGET_REPORT_KEYBOARD }, /* Q */
	{ KEY_ID(0x00, 0x02), 0x1A, TARGET_REPORT_KEYBOARD }, /* W */
	{ KEY_ID(0x00, 0x03), 0x08, TARGET_REPORT_KEYBOARD }, /* E */
	{ KEY_ID(0x00, 0x04), 0x15, TARGET_REPORT_KEYBOARD }, /* R */
	{ KEY_ID(0x00, 0x05), 0x18, TARGET_REPORT_KEYBOARD }, /* U */
	{ KEY_ID(0x00, 0x06), 0x0C, TARGET_REPORT_KEYBOARD }, /* I */
	{ KEY_ID(0x00, 0x07), 0x12, TARGET_REPORT_KEYBOARD }, /* O */
	{ KEY_ID(0x00, 0x08), 0x13, TARGET_REPORT_KEYBOARD }, /* P */
	{ KEY_ID(0x00, 0x09), 0x2C, TARGET_REPORT_KEYBOARD }, /* space */
	{ KEY_ID(0x00, 0x0B), 0x5F, TARGET_REPORT_KEYBOARD }, /* keypad 7 */
	{ KEY_ID(0x00, 0x0C), 0x60, TARGET_REPORT_KEYBOARD }, /* keypad 8 */
	{ KEY_ID(0x00, 0x0D), 0x61, TARGET_REPORT_KEYBOARD }, /* keypad 9 */
	{ KEY_ID(0x00, 0x0E), 0x57, TARGET_REPORT_KEYBOARD }, /* keypad + */
	{ KEY_ID(0x01, 0x01), 0x2B, TARGET_REPORT_KEYBOARD }, /* tab */
	{ KEY_ID(0x01, 0x02), 0x39, TARGET_REPORT_KEYBOARD }, /* capslock */
	{ KEY_ID(0x01, 0x03), 0x3C, TARGET_REPORT_KEYBOARD }, /* f3 */
	{ KEY_ID(0x01, 0x04), 0x17, TARGET_REPORT_KEYBOARD }, /* T */
	{ KEY_ID(0x01, 0x05), 0x1C, TARGET_REPORT_KEYBOARD }, /* Y */
	{ KEY_ID(0x01, 0x06), 0x30, TARGET_REPORT_KEYBOARD }, /* ] */
	{ KEY_ID(0x01, 0x07), 0x40, TARGET_REPORT_KEYBOARD }, /* f7 */
	{ KEY_ID(0x01, 0x08), 0x2F, TARGET_REPORT_KEYBOARD }, /* [ */
	{ KEY_ID(0x01, 0x0A), 0x2A, TARGET_REPORT_KEYBOARD }, /* backspace */
	{ KEY_ID(0x01, 0x0B), 0x5C, TARGET_REPORT_KEYBOARD }, /* keypad 4 */
	{ KEY_ID(0x01, 0x0C), 0x5D, TARGET_REPORT_KEYBOARD }, /* keypad 5 */
	{ KEY_ID(0x01, 0x0D), 0x5E, TARGET_REPORT_KEYBOARD }, /* keypad 6 */
	{ KEY_ID(0x01, 0x0F), 0xE1, TARGET_REPORT_KEYBOARD }, /* left shift */
	{ KEY_ID(0x01, 0x10), 0xE3, TARGET_REPORT_KEYBOARD }, /* left gui */
	{ KEY_ID(0x02, 0x01), 0x04, TARGET_REPORT_KEYBOARD }, /* A */
	{ KEY_ID(0x02, 0x02), 0x16, TARGET_REPORT_KEYBOARD }, /* S */
	{ KEY_ID(0x02, 0x03), 0x07, TARGET_REPORT_KEYBOARD }, /* D */
	{ KEY_ID(0x02, 0x04), 0x09, TARGET_REPORT_KEYBOARD }, /* F */
	{ KEY_ID(0x02, 0x05), 0x0D, TARGET_REPORT_KEYBOARD }, /* J */
	{ KEY_ID(0x02, 0x06), 0x0E, TARGET_REPORT_KEYBOARD }, /* K */
	{ KEY_ID(0x02, 0x07), 0x0F, TARGET_REPORT_KEYBOARD }, /* L */
	{ KEY_ID(0x02, 0x08), 0x33, TARGET_REPORT_KEYBOARD }, /* ; */
	{ KEY_ID(0x02, 0x0A), 0x31, TARGET_REPORT_KEYBOARD }, /* \ */
	{ KEY_ID(0x02, 0x0B), 0x59, TARGET_REPORT_KEYBOARD }, /* keypad 1 */
	{ KEY_ID(0x02, 0x0C), 0x5A, TARGET_REPORT_KEYBOARD }, /* keypad 2 */
	{ KEY_ID(0x02, 0x0D), 0x5B, TARGET_REPORT_KEYBOARD }, /* keypad 3 */
	{ KEY_ID(0x02, 0x0E), 0x58, TARGET_REPORT_KEYBOARD }, /* keypad enter */
	{ KEY_ID(0x02, 0x0F), 0xE5, TARGET_REPORT_KEYBOARD }, /* right shift */
	{ KEY_ID(0x03, 0x01), 0x29, TARGET_REPORT_KEYBOARD }, /* esc */
	{ KEY_ID(0x03, 0x03), 0x3D, TARGET_REPORT_KEYBOARD }, /* f4 */
	{ KEY_ID(0x03, 0x04), 0x0A, TARGET_REPORT_KEYBOARD }, /* G */
	{ KEY_ID(0x03, 0x05), 0x0B, TARGET_REPORT_KEYBOARD }, /* H */
	{ KEY_ID(0x03, 0x06), 0x3F, TARGET_REPORT_KEYBOARD }, /* f6 */
	{ KEY_ID(0x03, 0x08), 0x34, TARGET_REPORT_KEYBOARD }, /* ' */
	{ KEY_ID(0x03, 0x09), 0xE2, TARGET_REPORT_KEYBOARD }, /* left alt */
	{ KEY_ID(0x03, 0x0A), 0x44, TARGET_REPORT_KEYBOARD }, /* f11 */
	{ KEY_ID(0x03, 0x0C), 0x62, TARGET_REPORT_KEYBOARD }, /* keypad 0 */
	{ KEY_ID(0x03, 0x0D), 0x63, TARGET_REPORT_KEYBOARD }, /* keypad . */
	{ KEY_ID(0x03, 0x0E), 0x50, TARGET_REPORT_KEYBOARD }, /* arrow left */
	{ KEY_ID(0x03, 0x11), 0x81, TARGET_REPORT_KEYBOARD }, /* volume down */
	{ KEY_ID(0x04, 0x00), 0xE4, TARGET_REPORT_KEYBOARD }, /* right ctrl */
	{ KEY_ID(0x04, 0x01), 0x1D, TARGET_REPORT_KEYBOARD }, /* Z */
	{ KEY_ID(0x04, 0x02), 0x1B, TARGET_REPORT_KEYBOARD }, /* X */
	{ KEY_ID(0x04, 0x03), 0x06, TARGET_REPORT_KEYBOARD }, /* C */
	{ KEY_ID(0x04, 0x04), 0x19, TARGET_REPORT_KEYBOARD }, /* V */
	{ KEY_ID(0x04, 0x05), 0x10, TARGET_REPORT_KEYBOARD }, /* M */
	{ KEY_ID(0x04, 0x06), 0x36, TARGET_REPORT_KEYBOARD }, /* , */
	{ KEY_ID(0x04, 0x07), 0x37, TARGET_REPORT_KEYBOARD }, /* . */
	{ KEY_ID(0x04, 0x0A), 0x28, TARGET_REPORT_KEYBOARD }, /* enter */
	{ KEY_ID(0x04, 0x0B), 0x53, TARGET_REPORT_KEYBOARD }, /* num lock */
	{ KEY_ID(0x04, 0x0C), 0x54, TARGET_REPORT_KEYBOARD }, /* keypad / */
	{ KEY_ID(0x04, 0x0D), 0x55, TARGET_REPORT_KEYBOARD }, /* keypad * */
	{ KEY_ID(0x05, 0x04), 0x05, TARGET_REPORT_KEYBOARD }, /* B */
	{ KEY_ID(0x05, 0x05), 0x11, TARGET_REPORT_KEYBOARD }, /* N */
	{ KEY_ID(0x05, 0x07), 0xE7, TARGET_REPORT_KEYBOARD }, /* right gui */
	{ KEY_ID(0x05, 0x08), 0x38, TARGET_REPORT_KEYBOARD }, /* / */
	{ KEY_ID(0x05, 0x09), 0xE6, TARGET_REPORT_KEYBOARD }, /* right alt */
	{ KEY_ID(0x05, 0x0A), 0x45, TARGET_REPORT_KEYBOARD }, /* f12 */
	{ KEY_ID(0x05, 0x0B), 0x51, TARGET_REPORT_KEYBOARD }, /* arrow down */
	{ KEY_ID(0x05, 0x0C), 0x4F, TARGET_REPORT_KEYBOARD }, /* arrow right */
	{ KEY_ID(0x05, 0x0D), 0x56, TARGET_REPORT_KEYBOARD }, /* keypad - */
	{ KEY_ID(0x05, 0x0E), 0x52, TARGET_REPORT_KEYBOARD }, /* arrow up */
	{ KEY_ID(0x05, 0x11), 0x80, TARGET_REPORT_KEYBOARD }, /* volume up */
	{ KEY_ID(0x06, 0x00), 0xE0, TARGET_REPORT_KEYBOARD }, /* left ctrl */
	{ KEY_ID(0x06, 0x01), 0x35, TARGET_REPORT_KEYBOARD }, /* ~ */
	{ KEY_ID(0x06, 0x02), 0x3A, TARGET_REPORT_KEYBOARD }, /* f1 */
	{ KEY_ID(0x06, 0x03), 0x3B, TARGET_REPORT_KEYBOARD }, /* f2 */
	{ KEY_ID(0x06, 0x04), 0x22, TARGET_REPORT_KEYBOARD }, /* 5 */
	{ KEY_ID(0x06, 0x05), 0x23, TARGET_REPORT_KEYBOARD }, /* 6 */
	{ KEY_ID(0x06, 0x06), 0x2E, TARGET_REPORT_KEYBOARD }, /* = */
	{ KEY_ID(0x06, 0x07), 0x41, TARGET_REPORT_KEYBOARD }, /* f8 */
	{ KEY_ID(0x06, 0x08), 0x2D, TARGET_REPORT_KEYBOARD }, /* - */
	{ KEY_ID(0x06, 0x0A), 0x42, TARGET_REPORT_KEYBOARD }, /* f9 */
	{ KEY_ID(0x06, 0x0B), 0x4C, TARGET_REPORT_KEYBOARD }, /* delete */
	{ KEY_ID(0x06, 0x0C), 0x49, TARGET_REPORT_KEYBOARD }, /* insert */
	{ KEY_ID(0x06, 0x0D), 0x4B, TARGET_REPORT_KEYBOARD }, /* page up */
	{ KEY_ID(0x06, 0x0E), 0x4A, TARGET_REPORT_KEYBOARD }, /* home */
	{ KEY_ID(0x06, 0x11), 0x7F, TARGET_REPORT_KEYBOARD }, /* mute */
	{ KEY_ID(0x07, 0x00), 0x3E, TARGET_REPORT_KEYBOARD }, /* f5 */
	{ KEY_ID(0x07, 0x01), 0x1E, TARGET_REPORT_KEYBOARD }, /* 1 */
	{ KEY_ID(0x07, 0x02), 0x1F, TARGET_REPORT_KEYBOARD }, /* 2 */
	{ KEY_ID(0x07, 0x03), 0x20, TARGET_REPORT_KEYBOARD }, /* 3 */
	{ KEY_ID(0x07, 0x04), 0x21, TARGET_REPORT_KEYBOARD }, /* 4 */
	{ KEY_ID(0x07, 0x05), 0x24, TARGET_REPORT_KEYBOARD }, /* 7 */
	{ KEY_ID(0x07, 0x06), 0x25, TARGET_REPORT_KEYBOARD }, /* 8 */
	{ KEY_ID(0x07, 0x07), 0x26, TARGET_REPORT_KEYBOARD }, /* 9 */
	{ KEY_ID(0x07, 0x08), 0x27, TARGET_REPORT_KEYBOARD }, /* 0 */
	{ KEY_ID(0x07, 0x0A), 0x43, TARGET_REPORT_KEYBOARD }, /* f10 */
	{ KEY_ID(0x07, 0x0D), 0x4E, TARGET_REPORT_KEYBOARD }, /* page down */
	{ KEY_ID(0x07, 0x0E), 0x4D, TARGET_REPORT_KEYBOARD }, /* end */

	{ FN_KEY_ID(0x01, 0x03), 0x18A, TARGET_REPORT_MPLAYER }, /* e-mail */
	{ FN_KEY_ID(0x03, 0x03), 0x192, TARGET_REPORT_MPLAYER }, /* calculator */
	{ FN_KEY_ID(0x03, 0x0A), 0xCD,  TARGET_REPORT_MPLAYER }, /* play/pause */
	{ FN_KEY_ID(0x05, 0x0A), 0xB5,  TARGET_REPORT_MPLAYER }, /* next track */
	{ FN_KEY_ID(0x06, 0x02), 0x32,  TARGET_REPORT_MPLAYER }, /* sleep */
	{ FN_KEY_ID(0x06, 0x03), 0x192, TARGET_REPORT_MPLAYER }, /* internet */
	{ FN_KEY_ID(0x06, 0x0A), 0x221, TARGET_REPORT_MPLAYER }, /* find */
	{ FN_KEY_ID(0x06, 0x0C), 0x46,  TARGET_REPORT_KEYBOARD }, /* prt scr */
	{ FN_KEY_ID(0x06, 0x0D), 0x47,  TARGET_REPORT_KEYBOARD }, /* scroll lock */
	{ FN_KEY_ID(0x06, 0x0E), 0x48,  TARGET_REPORT_KEYBOARD }, /* pause break */
	{ FN_KEY_ID(0x07, 0x0A), 0xB6,  TARGET_REPORT_MPLAYER }, /* previous track */
};
