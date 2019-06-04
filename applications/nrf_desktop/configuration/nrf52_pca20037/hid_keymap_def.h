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
	{ KEY_ID(0x00, 0x01), 0x14, IN_REPORT_KEYBOARD_KEYS }, /* Q */
	{ KEY_ID(0x00, 0x02), 0x1A, IN_REPORT_KEYBOARD_KEYS }, /* W */
	{ KEY_ID(0x00, 0x03), 0x08, IN_REPORT_KEYBOARD_KEYS }, /* E */
	{ KEY_ID(0x00, 0x04), 0x15, IN_REPORT_KEYBOARD_KEYS }, /* R */
	{ KEY_ID(0x00, 0x05), 0x18, IN_REPORT_KEYBOARD_KEYS }, /* U */
	{ KEY_ID(0x00, 0x06), 0x0C, IN_REPORT_KEYBOARD_KEYS }, /* I */
	{ KEY_ID(0x00, 0x07), 0x12, IN_REPORT_KEYBOARD_KEYS }, /* O */
	{ KEY_ID(0x00, 0x08), 0x13, IN_REPORT_KEYBOARD_KEYS }, /* P */
	{ KEY_ID(0x00, 0x09), 0x2C, IN_REPORT_KEYBOARD_KEYS }, /* space */
	{ KEY_ID(0x00, 0x0B), 0x5F, IN_REPORT_KEYBOARD_KEYS }, /* keypad 7 */
	{ KEY_ID(0x00, 0x0C), 0x60, IN_REPORT_KEYBOARD_KEYS }, /* keypad 8 */
	{ KEY_ID(0x00, 0x0D), 0x61, IN_REPORT_KEYBOARD_KEYS }, /* keypad 9 */
	{ KEY_ID(0x00, 0x0E), 0x57, IN_REPORT_KEYBOARD_KEYS }, /* keypad + */
	{ KEY_ID(0x01, 0x01), 0x2B, IN_REPORT_KEYBOARD_KEYS }, /* tab */
	{ KEY_ID(0x01, 0x02), 0x39, IN_REPORT_KEYBOARD_KEYS }, /* capslock */
	{ KEY_ID(0x01, 0x03), 0x3C, IN_REPORT_KEYBOARD_KEYS }, /* f3 */
	{ KEY_ID(0x01, 0x04), 0x17, IN_REPORT_KEYBOARD_KEYS }, /* T */
	{ KEY_ID(0x01, 0x05), 0x1C, IN_REPORT_KEYBOARD_KEYS }, /* Y */
	{ KEY_ID(0x01, 0x06), 0x30, IN_REPORT_KEYBOARD_KEYS }, /* ] */
	{ KEY_ID(0x01, 0x07), 0x40, IN_REPORT_KEYBOARD_KEYS }, /* f7 */
	{ KEY_ID(0x01, 0x08), 0x2F, IN_REPORT_KEYBOARD_KEYS }, /* [ */
	{ KEY_ID(0x01, 0x0A), 0x2A, IN_REPORT_KEYBOARD_KEYS }, /* backspace */
	{ KEY_ID(0x01, 0x0B), 0x5C, IN_REPORT_KEYBOARD_KEYS }, /* keypad 4 */
	{ KEY_ID(0x01, 0x0C), 0x5D, IN_REPORT_KEYBOARD_KEYS }, /* keypad 5 */
	{ KEY_ID(0x01, 0x0D), 0x5E, IN_REPORT_KEYBOARD_KEYS }, /* keypad 6 */
	{ KEY_ID(0x01, 0x0F), 0xE1, IN_REPORT_KEYBOARD_KEYS }, /* left shift */
	{ KEY_ID(0x01, 0x10), 0xE3, IN_REPORT_KEYBOARD_KEYS }, /* left gui */
	{ KEY_ID(0x02, 0x01), 0x04, IN_REPORT_KEYBOARD_KEYS }, /* A */
	{ KEY_ID(0x02, 0x02), 0x16, IN_REPORT_KEYBOARD_KEYS }, /* S */
	{ KEY_ID(0x02, 0x03), 0x07, IN_REPORT_KEYBOARD_KEYS }, /* D */
	{ KEY_ID(0x02, 0x04), 0x09, IN_REPORT_KEYBOARD_KEYS }, /* F */
	{ KEY_ID(0x02, 0x05), 0x0D, IN_REPORT_KEYBOARD_KEYS }, /* J */
	{ KEY_ID(0x02, 0x06), 0x0E, IN_REPORT_KEYBOARD_KEYS }, /* K */
	{ KEY_ID(0x02, 0x07), 0x0F, IN_REPORT_KEYBOARD_KEYS }, /* L */
	{ KEY_ID(0x02, 0x08), 0x33, IN_REPORT_KEYBOARD_KEYS }, /* ; */
	{ KEY_ID(0x02, 0x0A), 0x31, IN_REPORT_KEYBOARD_KEYS }, /* \ */
	{ KEY_ID(0x02, 0x0B), 0x59, IN_REPORT_KEYBOARD_KEYS }, /* keypad 1 */
	{ KEY_ID(0x02, 0x0C), 0x5A, IN_REPORT_KEYBOARD_KEYS }, /* keypad 2 */
	{ KEY_ID(0x02, 0x0D), 0x5B, IN_REPORT_KEYBOARD_KEYS }, /* keypad 3 */
	{ KEY_ID(0x02, 0x0E), 0x58, IN_REPORT_KEYBOARD_KEYS }, /* keypad enter */
	{ KEY_ID(0x02, 0x0F), 0xE5, IN_REPORT_KEYBOARD_KEYS }, /* right shift */
	{ KEY_ID(0x03, 0x01), 0x29, IN_REPORT_KEYBOARD_KEYS }, /* esc */
	{ KEY_ID(0x03, 0x03), 0x3D, IN_REPORT_KEYBOARD_KEYS }, /* f4 */
	{ KEY_ID(0x03, 0x04), 0x0A, IN_REPORT_KEYBOARD_KEYS }, /* G */
	{ KEY_ID(0x03, 0x05), 0x0B, IN_REPORT_KEYBOARD_KEYS }, /* H */
	{ KEY_ID(0x03, 0x06), 0x3F, IN_REPORT_KEYBOARD_KEYS }, /* f6 */
	{ KEY_ID(0x03, 0x08), 0x34, IN_REPORT_KEYBOARD_KEYS }, /* ' */
	{ KEY_ID(0x03, 0x09), 0xE2, IN_REPORT_KEYBOARD_KEYS }, /* left alt */
	{ KEY_ID(0x03, 0x0A), 0x44, IN_REPORT_KEYBOARD_KEYS }, /* f11 */
	{ KEY_ID(0x03, 0x0C), 0x62, IN_REPORT_KEYBOARD_KEYS }, /* keypad 0 */
	{ KEY_ID(0x03, 0x0D), 0x63, IN_REPORT_KEYBOARD_KEYS }, /* keypad . */
	{ KEY_ID(0x03, 0x0E), 0x50, IN_REPORT_KEYBOARD_KEYS }, /* arrow left */
	{ KEY_ID(0x03, 0x11), 0x81, IN_REPORT_KEYBOARD_KEYS }, /* volume down */
	{ KEY_ID(0x04, 0x00), 0xE4, IN_REPORT_KEYBOARD_KEYS }, /* right ctrl */
	{ KEY_ID(0x04, 0x01), 0x1D, IN_REPORT_KEYBOARD_KEYS }, /* Z */
	{ KEY_ID(0x04, 0x02), 0x1B, IN_REPORT_KEYBOARD_KEYS }, /* X */
	{ KEY_ID(0x04, 0x03), 0x06, IN_REPORT_KEYBOARD_KEYS }, /* C */
	{ KEY_ID(0x04, 0x04), 0x19, IN_REPORT_KEYBOARD_KEYS }, /* V */
	{ KEY_ID(0x04, 0x05), 0x10, IN_REPORT_KEYBOARD_KEYS }, /* M */
	{ KEY_ID(0x04, 0x06), 0x36, IN_REPORT_KEYBOARD_KEYS }, /* , */
	{ KEY_ID(0x04, 0x07), 0x37, IN_REPORT_KEYBOARD_KEYS }, /* . */
	{ KEY_ID(0x04, 0x0A), 0x28, IN_REPORT_KEYBOARD_KEYS }, /* enter */
	{ KEY_ID(0x04, 0x0B), 0x53, IN_REPORT_KEYBOARD_KEYS }, /* num lock */
	{ KEY_ID(0x04, 0x0C), 0x54, IN_REPORT_KEYBOARD_KEYS }, /* keypad / */
	{ KEY_ID(0x04, 0x0D), 0x55, IN_REPORT_KEYBOARD_KEYS }, /* keypad * */
	{ KEY_ID(0x05, 0x04), 0x05, IN_REPORT_KEYBOARD_KEYS }, /* B */
	{ KEY_ID(0x05, 0x05), 0x11, IN_REPORT_KEYBOARD_KEYS }, /* N */
	{ KEY_ID(0x05, 0x07), 0xE7, IN_REPORT_KEYBOARD_KEYS }, /* right gui */
	{ KEY_ID(0x05, 0x08), 0x38, IN_REPORT_KEYBOARD_KEYS }, /* / */
	{ KEY_ID(0x05, 0x09), 0xE6, IN_REPORT_KEYBOARD_KEYS }, /* right alt */
	{ KEY_ID(0x05, 0x0A), 0x45, IN_REPORT_KEYBOARD_KEYS }, /* f12 */
	{ KEY_ID(0x05, 0x0B), 0x51, IN_REPORT_KEYBOARD_KEYS }, /* arrow down */
	{ KEY_ID(0x05, 0x0C), 0x4F, IN_REPORT_KEYBOARD_KEYS }, /* arrow right */
	{ KEY_ID(0x05, 0x0D), 0x56, IN_REPORT_KEYBOARD_KEYS }, /* keypad - */
	{ KEY_ID(0x05, 0x0E), 0x52, IN_REPORT_KEYBOARD_KEYS }, /* arrow up */
	{ KEY_ID(0x05, 0x11), 0x80, IN_REPORT_KEYBOARD_KEYS }, /* volume up */
	{ KEY_ID(0x06, 0x00), 0xE0, IN_REPORT_KEYBOARD_KEYS }, /* left ctrl */
	{ KEY_ID(0x06, 0x01), 0x35, IN_REPORT_KEYBOARD_KEYS }, /* ~ */
	{ KEY_ID(0x06, 0x02), 0x3A, IN_REPORT_KEYBOARD_KEYS }, /* f1 */
	{ KEY_ID(0x06, 0x03), 0x3B, IN_REPORT_KEYBOARD_KEYS }, /* f2 */
	{ KEY_ID(0x06, 0x04), 0x22, IN_REPORT_KEYBOARD_KEYS }, /* 5 */
	{ KEY_ID(0x06, 0x05), 0x23, IN_REPORT_KEYBOARD_KEYS }, /* 6 */
	{ KEY_ID(0x06, 0x06), 0x2E, IN_REPORT_KEYBOARD_KEYS }, /* = */
	{ KEY_ID(0x06, 0x07), 0x41, IN_REPORT_KEYBOARD_KEYS }, /* f8 */
	{ KEY_ID(0x06, 0x08), 0x2D, IN_REPORT_KEYBOARD_KEYS }, /* - */
	{ KEY_ID(0x06, 0x0A), 0x42, IN_REPORT_KEYBOARD_KEYS }, /* f9 */
	{ KEY_ID(0x06, 0x0B), 0x4C, IN_REPORT_KEYBOARD_KEYS }, /* delete */
	{ KEY_ID(0x06, 0x0C), 0x49, IN_REPORT_KEYBOARD_KEYS }, /* insert */
	{ KEY_ID(0x06, 0x0D), 0x4B, IN_REPORT_KEYBOARD_KEYS }, /* page up */
	{ KEY_ID(0x06, 0x0E), 0x4A, IN_REPORT_KEYBOARD_KEYS }, /* home */
	{ KEY_ID(0x06, 0x11), 0x7F, IN_REPORT_KEYBOARD_KEYS }, /* mute */
	{ KEY_ID(0x07, 0x00), 0x3E, IN_REPORT_KEYBOARD_KEYS }, /* f5 */
	{ KEY_ID(0x07, 0x01), 0x1E, IN_REPORT_KEYBOARD_KEYS }, /* 1 */
	{ KEY_ID(0x07, 0x02), 0x1F, IN_REPORT_KEYBOARD_KEYS }, /* 2 */
	{ KEY_ID(0x07, 0x03), 0x20, IN_REPORT_KEYBOARD_KEYS }, /* 3 */
	{ KEY_ID(0x07, 0x04), 0x21, IN_REPORT_KEYBOARD_KEYS }, /* 4 */
	{ KEY_ID(0x07, 0x05), 0x24, IN_REPORT_KEYBOARD_KEYS }, /* 7 */
	{ KEY_ID(0x07, 0x06), 0x25, IN_REPORT_KEYBOARD_KEYS }, /* 8 */
	{ KEY_ID(0x07, 0x07), 0x26, IN_REPORT_KEYBOARD_KEYS }, /* 9 */
	{ KEY_ID(0x07, 0x08), 0x27, IN_REPORT_KEYBOARD_KEYS }, /* 0 */
	{ KEY_ID(0x07, 0x0A), 0x43, IN_REPORT_KEYBOARD_KEYS }, /* f10 */
	{ KEY_ID(0x07, 0x0D), 0x4E, IN_REPORT_KEYBOARD_KEYS }, /* page down */
	{ KEY_ID(0x07, 0x0E), 0x4D, IN_REPORT_KEYBOARD_KEYS }, /* end */

	{ FN_KEY_ID(0x01, 0x03), 0x18A, IN_REPORT_MPLAYER }, /* e-mail */
	{ FN_KEY_ID(0x03, 0x03), 0x192, IN_REPORT_MPLAYER }, /* calculator */
	{ FN_KEY_ID(0x03, 0x0A), 0xCD,  IN_REPORT_MPLAYER }, /* play/pause */
	{ FN_KEY_ID(0x05, 0x0A), 0xB5,  IN_REPORT_MPLAYER }, /* next track */
	{ FN_KEY_ID(0x06, 0x02), 0x32,  IN_REPORT_MPLAYER }, /* sleep */
	{ FN_KEY_ID(0x06, 0x03), 0x192, IN_REPORT_MPLAYER }, /* internet */
	{ FN_KEY_ID(0x06, 0x0A), 0x221, IN_REPORT_MPLAYER }, /* find */
	{ FN_KEY_ID(0x06, 0x0C), 0x46,  IN_REPORT_KEYBOARD_KEYS }, /* prt scr */
	{ FN_KEY_ID(0x06, 0x0D), 0x47,  IN_REPORT_KEYBOARD_KEYS }, /* scroll lock */
	{ FN_KEY_ID(0x06, 0x0E), 0x48,  IN_REPORT_KEYBOARD_KEYS }, /* pause break */
	{ FN_KEY_ID(0x07, 0x0A), 0xB6,  IN_REPORT_MPLAYER }, /* previous track */
};
