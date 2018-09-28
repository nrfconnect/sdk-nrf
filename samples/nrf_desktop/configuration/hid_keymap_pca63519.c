/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <misc/util.h>

#include "hid_keymap.h"

/*
 * HID keymap. The Consumer Control keys are defined in section 15 of
 * the HID Usage Tables document under the following URL:
 * http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 */
const struct hid_keymap hid_keymap[] = {
	{ 0x00, 0x193, TARGET_REPORT_MPLAYER },      /* Consumer Control: AL A/V Capture/Playback */
	{ 0x01, 0x27,  TARGET_REPORT_KEYBOARD },     /* Keyboard '0' and ')' */
	{ 0x02, 0x2A,  TARGET_REPORT_KEYBOARD },     /* Keyboard Backspace */
	{ 0x03, 0x24,  TARGET_REPORT_KEYBOARD },     /* Keyboard '7' and '&' */
	{ 0x04, 0x25,  TARGET_REPORT_KEYBOARD },     /* Keyboard '8' and '*' */
	{ 0x05, 0x26,  TARGET_REPORT_KEYBOARD },     /* Keyboard '9' and '(' */
	{ 0x06, 0x21,  TARGET_REPORT_KEYBOARD },     /* Keyboard '4' and '$' */
	{ 0x07, 0x22,  TARGET_REPORT_KEYBOARD },     /* Keyboard '5' and '%' */
	{ 0x10, 0x23,  TARGET_REPORT_KEYBOARD },     /* Keyboard '6' and '^' */
	{ 0x11, 0x1E,  TARGET_REPORT_KEYBOARD },     /* Keyboard '1' and '!' */
	{ 0x12, 0x1F,  TARGET_REPORT_KEYBOARD },     /* Keyboard '2' and '@' */
	{ 0x13, 0x20,  TARGET_REPORT_KEYBOARD },     /* Keyboard '3' and '#' */
	{ 0x20, 0x221, TARGET_REPORT_MPLAYER },      /* Consumer Control: AC Search */
	{ 0x22, 0xEA,  TARGET_REPORT_MPLAYER },      /* Consumer Control: Volume Decrement */
	{ 0x24, 0x4E,  TARGET_REPORT_KEYBOARD },     /* Keyboard Page Down */
	{ 0x25, 0xE9,  TARGET_REPORT_MPLAYER },      /* Consumer Control: Volume Increment */
	{ 0x27, 0x4B,  TARGET_REPORT_KEYBOARD },     /* Keyboard Page Up */
	{ 0x30, 0x01,  TARGET_REPORT_MOUSE_BUTTON }, /* Keyboard: Left Mouse Button emulation */
	{ 0x31, 0x51,  TARGET_REPORT_KEYBOARD },     /* Keyboard Down Arrow */
	{ 0x32, 0x02,  TARGET_REPORT_MOUSE_BUTTON }, /* Keyboard: Right Mouse Button emulation */
	{ 0x33, 0x50,  TARGET_REPORT_KEYBOARD },     /* Keyboard Left Arrow */
	{ 0x34, 0x28,  TARGET_REPORT_KEYBOARD },     /* Keyboard Enter */
	{ 0x35, 0x4F,  TARGET_REPORT_KEYBOARD },     /* Keyboard Right Arrow */
	{ 0x40, 0x52,  TARGET_REPORT_KEYBOARD },     /* Keyboard Up Arrow */
	{ 0x42, 0xCD,  TARGET_REPORT_MPLAYER },      /* Consumer Control: Play/Pause */
	{ 0x43, 0xB6,  TARGET_REPORT_MPLAYER },      /* Consumer Control: Scan Prev Track */
	{ 0x44, 0xB5,  TARGET_REPORT_MPLAYER },      /* Consumer Control: Scan Next Track */
	{ 0x45, 0xB7,  TARGET_REPORT_MPLAYER },      /* Consumer Control: Stop */
	{ 0x46, 0x7F,  TARGET_REPORT_KEYBOARD },     /* Keyboard Mute */
};

const size_t hid_keymap_size = ARRAY_SIZE(hid_keymap);
