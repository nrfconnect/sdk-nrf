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
	{ KEY_ID(0x00, 0x01), 0x0014, IN_REPORT_KEYBOARD_KEYS }, /* Q */
	{ KEY_ID(0x00, 0x02), 0x001A, IN_REPORT_KEYBOARD_KEYS }, /* W */
	{ KEY_ID(0x00, 0x03), 0x0008, IN_REPORT_KEYBOARD_KEYS }, /* E */
	{ KEY_ID(0x00, 0x04), 0x0015, IN_REPORT_KEYBOARD_KEYS }, /* R */
	{ KEY_ID(0x00, 0x05), 0x0018, IN_REPORT_KEYBOARD_KEYS }, /* U */
	{ KEY_ID(0x00, 0x06), 0x000C, IN_REPORT_KEYBOARD_KEYS }, /* I */
	{ KEY_ID(0x00, 0x07), 0x0012, IN_REPORT_KEYBOARD_KEYS }, /* O */
	{ KEY_ID(0x00, 0x08), 0x0013, IN_REPORT_KEYBOARD_KEYS }, /* P */
	{ KEY_ID(0x00, 0x09), 0x002C, IN_REPORT_KEYBOARD_KEYS }, /* space */
	{ KEY_ID(0x00, 0x0B), 0x005F, IN_REPORT_KEYBOARD_KEYS }, /* keypad 7 */
	{ KEY_ID(0x00, 0x0C), 0x0060, IN_REPORT_KEYBOARD_KEYS }, /* keypad 8 */
	{ KEY_ID(0x00, 0x0D), 0x0061, IN_REPORT_KEYBOARD_KEYS }, /* keypad 9 */
	{ KEY_ID(0x00, 0x0E), 0x0057, IN_REPORT_KEYBOARD_KEYS }, /* keypad + */
	{ KEY_ID(0x01, 0x01), 0x002B, IN_REPORT_KEYBOARD_KEYS }, /* tab */
	{ KEY_ID(0x01, 0x02), 0x0039, IN_REPORT_KEYBOARD_KEYS }, /* capslock */
	{ KEY_ID(0x01, 0x03), 0x003C, IN_REPORT_KEYBOARD_KEYS }, /* f3 */
	{ KEY_ID(0x01, 0x04), 0x0017, IN_REPORT_KEYBOARD_KEYS }, /* T */
	{ KEY_ID(0x01, 0x05), 0x001C, IN_REPORT_KEYBOARD_KEYS }, /* Y */
	{ KEY_ID(0x01, 0x06), 0x0030, IN_REPORT_KEYBOARD_KEYS }, /* ] */
	{ KEY_ID(0x01, 0x07), 0x0040, IN_REPORT_KEYBOARD_KEYS }, /* f7 */
	{ KEY_ID(0x01, 0x08), 0x002F, IN_REPORT_KEYBOARD_KEYS }, /* [ */
	{ KEY_ID(0x01, 0x0A), 0x002A, IN_REPORT_KEYBOARD_KEYS }, /* backspace */
	{ KEY_ID(0x01, 0x0B), 0x005C, IN_REPORT_KEYBOARD_KEYS }, /* keypad 4 */
	{ KEY_ID(0x01, 0x0C), 0x005D, IN_REPORT_KEYBOARD_KEYS }, /* keypad 5 */
	{ KEY_ID(0x01, 0x0D), 0x005E, IN_REPORT_KEYBOARD_KEYS }, /* keypad 6 */
	{ KEY_ID(0x01, 0x0F), 0x00E1, IN_REPORT_KEYBOARD_KEYS }, /* left shift */
	{ KEY_ID(0x01, 0x10), 0x00E3, IN_REPORT_KEYBOARD_KEYS }, /* left gui */
	{ KEY_ID(0x02, 0x01), 0x0004, IN_REPORT_KEYBOARD_KEYS }, /* A */
	{ KEY_ID(0x02, 0x02), 0x0016, IN_REPORT_KEYBOARD_KEYS }, /* S */
	{ KEY_ID(0x02, 0x03), 0x0007, IN_REPORT_KEYBOARD_KEYS }, /* D */
	{ KEY_ID(0x02, 0x04), 0x0009, IN_REPORT_KEYBOARD_KEYS }, /* F */
	{ KEY_ID(0x02, 0x05), 0x000D, IN_REPORT_KEYBOARD_KEYS }, /* J */
	{ KEY_ID(0x02, 0x06), 0x000E, IN_REPORT_KEYBOARD_KEYS }, /* K */
	{ KEY_ID(0x02, 0x07), 0x000F, IN_REPORT_KEYBOARD_KEYS }, /* L */
	{ KEY_ID(0x02, 0x08), 0x0033, IN_REPORT_KEYBOARD_KEYS }, /* ; */
	{ KEY_ID(0x02, 0x0A), 0x0031, IN_REPORT_KEYBOARD_KEYS }, /* \ */
	{ KEY_ID(0x02, 0x0B), 0x0059, IN_REPORT_KEYBOARD_KEYS }, /* keypad 1 */
	{ KEY_ID(0x02, 0x0C), 0x005A, IN_REPORT_KEYBOARD_KEYS }, /* keypad 2 */
	{ KEY_ID(0x02, 0x0D), 0x005B, IN_REPORT_KEYBOARD_KEYS }, /* keypad 3 */
	{ KEY_ID(0x02, 0x0E), 0x0058, IN_REPORT_KEYBOARD_KEYS }, /* keypad enter */
	{ KEY_ID(0x02, 0x0F), 0x00E5, IN_REPORT_KEYBOARD_KEYS }, /* right shift */
	{ KEY_ID(0x03, 0x01), 0x0029, IN_REPORT_KEYBOARD_KEYS }, /* esc */
	{ KEY_ID(0x03, 0x03), 0x003D, IN_REPORT_KEYBOARD_KEYS }, /* f4 */
	{ KEY_ID(0x03, 0x04), 0x000A, IN_REPORT_KEYBOARD_KEYS }, /* G */
	{ KEY_ID(0x03, 0x05), 0x000B, IN_REPORT_KEYBOARD_KEYS }, /* H */
	{ KEY_ID(0x03, 0x06), 0x003F, IN_REPORT_KEYBOARD_KEYS }, /* f6 */
	{ KEY_ID(0x03, 0x08), 0x0034, IN_REPORT_KEYBOARD_KEYS }, /* ' */
	{ KEY_ID(0x03, 0x09), 0x00E2, IN_REPORT_KEYBOARD_KEYS }, /* left alt */
	{ KEY_ID(0x03, 0x0A), 0x0044, IN_REPORT_KEYBOARD_KEYS }, /* f11 */
	{ KEY_ID(0x03, 0x0C), 0x0062, IN_REPORT_KEYBOARD_KEYS }, /* keypad 0 */
	{ KEY_ID(0x03, 0x0D), 0x0063, IN_REPORT_KEYBOARD_KEYS }, /* keypad . */
	{ KEY_ID(0x03, 0x0E), 0x0050, IN_REPORT_KEYBOARD_KEYS }, /* arrow left */
	{ KEY_ID(0x03, 0x11), 0x00EA, IN_REPORT_CONSUMER_CTRL }, /* volume down */
	{ KEY_ID(0x04, 0x00), 0x00E4, IN_REPORT_KEYBOARD_KEYS }, /* right ctrl */
	{ KEY_ID(0x04, 0x01), 0x001D, IN_REPORT_KEYBOARD_KEYS }, /* Z */
	{ KEY_ID(0x04, 0x02), 0x001B, IN_REPORT_KEYBOARD_KEYS }, /* X */
	{ KEY_ID(0x04, 0x03), 0x0006, IN_REPORT_KEYBOARD_KEYS }, /* C */
	{ KEY_ID(0x04, 0x04), 0x0019, IN_REPORT_KEYBOARD_KEYS }, /* V */
	{ KEY_ID(0x04, 0x05), 0x0010, IN_REPORT_KEYBOARD_KEYS }, /* M */
	{ KEY_ID(0x04, 0x06), 0x0036, IN_REPORT_KEYBOARD_KEYS }, /* , */
	{ KEY_ID(0x04, 0x07), 0x0037, IN_REPORT_KEYBOARD_KEYS }, /* . */
	{ KEY_ID(0x04, 0x0A), 0x0028, IN_REPORT_KEYBOARD_KEYS }, /* enter */
	{ KEY_ID(0x04, 0x0B), 0x0053, IN_REPORT_KEYBOARD_KEYS }, /* num lock */
	{ KEY_ID(0x04, 0x0C), 0x0054, IN_REPORT_KEYBOARD_KEYS }, /* keypad / */
	{ KEY_ID(0x04, 0x0D), 0x0055, IN_REPORT_KEYBOARD_KEYS }, /* keypad * */
	{ KEY_ID(0x05, 0x04), 0x0005, IN_REPORT_KEYBOARD_KEYS }, /* B */
	{ KEY_ID(0x05, 0x05), 0x0011, IN_REPORT_KEYBOARD_KEYS }, /* N */
	{ KEY_ID(0x05, 0x07), 0x0065, IN_REPORT_KEYBOARD_KEYS }, /* Keyboard Application */
	{ KEY_ID(0x05, 0x08), 0x0038, IN_REPORT_KEYBOARD_KEYS }, /* / */
	{ KEY_ID(0x05, 0x09), 0x00E6, IN_REPORT_KEYBOARD_KEYS }, /* right alt */
	{ KEY_ID(0x05, 0x0A), 0x0045, IN_REPORT_KEYBOARD_KEYS }, /* f12 */
	{ KEY_ID(0x05, 0x0B), 0x0051, IN_REPORT_KEYBOARD_KEYS }, /* arrow down */
	{ KEY_ID(0x05, 0x0C), 0x004F, IN_REPORT_KEYBOARD_KEYS }, /* arrow right */
	{ KEY_ID(0x05, 0x0D), 0x0056, IN_REPORT_KEYBOARD_KEYS }, /* keypad - */
	{ KEY_ID(0x05, 0x0E), 0x0052, IN_REPORT_KEYBOARD_KEYS }, /* arrow up */
	{ KEY_ID(0x05, 0x11), 0x00E9, IN_REPORT_CONSUMER_CTRL }, /* volume up */
	{ KEY_ID(0x06, 0x00), 0x00E0, IN_REPORT_KEYBOARD_KEYS }, /* left ctrl */
	{ KEY_ID(0x06, 0x01), 0x0035, IN_REPORT_KEYBOARD_KEYS }, /* ~ */
	{ KEY_ID(0x06, 0x02), 0x003A, IN_REPORT_KEYBOARD_KEYS }, /* f1 */
	{ KEY_ID(0x06, 0x03), 0x003B, IN_REPORT_KEYBOARD_KEYS }, /* f2 */
	{ KEY_ID(0x06, 0x04), 0x0022, IN_REPORT_KEYBOARD_KEYS }, /* 5 */
	{ KEY_ID(0x06, 0x05), 0x0023, IN_REPORT_KEYBOARD_KEYS }, /* 6 */
	{ KEY_ID(0x06, 0x06), 0x002E, IN_REPORT_KEYBOARD_KEYS }, /* = */
	{ KEY_ID(0x06, 0x07), 0x0041, IN_REPORT_KEYBOARD_KEYS }, /* f8 */
	{ KEY_ID(0x06, 0x08), 0x002D, IN_REPORT_KEYBOARD_KEYS }, /* - */
	{ KEY_ID(0x06, 0x0A), 0x0042, IN_REPORT_KEYBOARD_KEYS }, /* f9 */
	{ KEY_ID(0x06, 0x0B), 0x004C, IN_REPORT_KEYBOARD_KEYS }, /* delete */
	{ KEY_ID(0x06, 0x0C), 0x0049, IN_REPORT_KEYBOARD_KEYS }, /* insert */
	{ KEY_ID(0x06, 0x0D), 0x004B, IN_REPORT_KEYBOARD_KEYS }, /* page up */
	{ KEY_ID(0x06, 0x0E), 0x004A, IN_REPORT_KEYBOARD_KEYS }, /* home */
	{ KEY_ID(0x06, 0x11), 0x00E2, IN_REPORT_CONSUMER_CTRL }, /* mute */
	{ KEY_ID(0x07, 0x00), 0x003E, IN_REPORT_KEYBOARD_KEYS }, /* f5 */
	{ KEY_ID(0x07, 0x01), 0x001E, IN_REPORT_KEYBOARD_KEYS }, /* 1 */
	{ KEY_ID(0x07, 0x02), 0x001F, IN_REPORT_KEYBOARD_KEYS }, /* 2 */
	{ KEY_ID(0x07, 0x03), 0x0020, IN_REPORT_KEYBOARD_KEYS }, /* 3 */
	{ KEY_ID(0x07, 0x04), 0x0021, IN_REPORT_KEYBOARD_KEYS }, /* 4 */
	{ KEY_ID(0x07, 0x05), 0x0024, IN_REPORT_KEYBOARD_KEYS }, /* 7 */
	{ KEY_ID(0x07, 0x06), 0x0025, IN_REPORT_KEYBOARD_KEYS }, /* 8 */
	{ KEY_ID(0x07, 0x07), 0x0026, IN_REPORT_KEYBOARD_KEYS }, /* 9 */
	{ KEY_ID(0x07, 0x08), 0x0027, IN_REPORT_KEYBOARD_KEYS }, /* 0 */
	{ KEY_ID(0x07, 0x0A), 0x0043, IN_REPORT_KEYBOARD_KEYS }, /* f10 */
	{ KEY_ID(0x07, 0x0D), 0x004E, IN_REPORT_KEYBOARD_KEYS }, /* page down */
	{ KEY_ID(0x07, 0x0E), 0x004D, IN_REPORT_KEYBOARD_KEYS }, /* end */

	{ FN_KEY_ID(0x01, 0x03), 0x018A, IN_REPORT_CONSUMER_CTRL }, /* e-mail */
	{ FN_KEY_ID(0x03, 0x03), 0x0192, IN_REPORT_CONSUMER_CTRL }, /* calculator */
	{ FN_KEY_ID(0x03, 0x0A), 0x00CD, IN_REPORT_CONSUMER_CTRL }, /* play/pause */
	{ FN_KEY_ID(0x05, 0x0A), 0x00B5, IN_REPORT_CONSUMER_CTRL }, /* next track */
	{ FN_KEY_ID(0x06, 0x02), 0x0082, IN_REPORT_SYSTEM_CTRL },   /* sleep */
	{ FN_KEY_ID(0x06, 0x03), 0x0196, IN_REPORT_CONSUMER_CTRL }, /* internet */
	{ FN_KEY_ID(0x06, 0x0A), 0x021F, IN_REPORT_CONSUMER_CTRL }, /* find */
	{ FN_KEY_ID(0x06, 0x0C), 0x0046, IN_REPORT_KEYBOARD_KEYS }, /* prt scr */
	{ FN_KEY_ID(0x06, 0x0D), 0x0047, IN_REPORT_KEYBOARD_KEYS }, /* scroll lock */
	{ FN_KEY_ID(0x06, 0x0E), 0x0048, IN_REPORT_KEYBOARD_KEYS }, /* pause break */
	{ FN_KEY_ID(0x07, 0x0A), 0x00B6, IN_REPORT_CONSUMER_CTRL }, /* previous track */
};
