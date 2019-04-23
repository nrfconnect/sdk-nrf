/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "hid_keymap.h"

/* This file must be included only once */
const struct {} hid_keymap_def_include_once;

/*
 * HID keymap. The Consumer Control keys are defined in section 15 of
 * the HID Usage Tables document under the following URL:
 * http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 */
static const struct hid_keymap hid_keymap[] = {
	{ 0x0001, 0x14, TARGET_REPORT_KEYBOARD }, /* Q */
	{ 0x0002, 0x1A, TARGET_REPORT_KEYBOARD }, /* W */
	{ 0x0003, 0x08, TARGET_REPORT_KEYBOARD }, /* E */
	{ 0x0004, 0x15, TARGET_REPORT_KEYBOARD }, /* R */
	{ 0x0005, 0x18, TARGET_REPORT_KEYBOARD }, /* U */
	{ 0x0006, 0x0C, TARGET_REPORT_KEYBOARD }, /* I */
	{ 0x0007, 0x12, TARGET_REPORT_KEYBOARD }, /* O */
	{ 0x0008, 0x13, TARGET_REPORT_KEYBOARD }, /* P */
	{ 0x0009, 0x2C, TARGET_REPORT_KEYBOARD }, /* space */
	{ 0x000B, 0x5F, TARGET_REPORT_KEYBOARD }, /* keypad 7 */
	{ 0x000C, 0x60, TARGET_REPORT_KEYBOARD }, /* keypad 8 */
	{ 0x000D, 0x61, TARGET_REPORT_KEYBOARD }, /* keypad 9 */
	{ 0x000E, 0x57, TARGET_REPORT_KEYBOARD }, /* keypad + */
	{ 0x0101, 0x2B, TARGET_REPORT_KEYBOARD }, /* tab */
	{ 0x0102, 0x39, TARGET_REPORT_KEYBOARD }, /* capslock */
	{ 0x0103, 0x3C, TARGET_REPORT_KEYBOARD }, /* f3 */
	{ 0x0104, 0x17, TARGET_REPORT_KEYBOARD }, /* T */
	{ 0x0105, 0x1C, TARGET_REPORT_KEYBOARD }, /* Y */
	{ 0x0106, 0x30, TARGET_REPORT_KEYBOARD }, /* ] */
	{ 0x0107, 0x40, TARGET_REPORT_KEYBOARD }, /* f7 */
	{ 0x0108, 0x2F, TARGET_REPORT_KEYBOARD }, /* [ */
	{ 0x010A, 0x2A, TARGET_REPORT_KEYBOARD }, /* backspace */
	{ 0x010B, 0x5C, TARGET_REPORT_KEYBOARD }, /* keypad 4 */
	{ 0x010C, 0x5D, TARGET_REPORT_KEYBOARD }, /* keypad 5 */
	{ 0x010D, 0x5E, TARGET_REPORT_KEYBOARD }, /* keypad 6 */
	{ 0x010F, 0xE1, TARGET_REPORT_KEYBOARD }, /* left shift */
	{ 0x0110, 0xE3, TARGET_REPORT_KEYBOARD }, /* left gui */
	{ 0x0201, 0x04, TARGET_REPORT_KEYBOARD }, /* A */
	{ 0x0202, 0x16, TARGET_REPORT_KEYBOARD }, /* S */
	{ 0x0203, 0x07, TARGET_REPORT_KEYBOARD }, /* D */
	{ 0x0204, 0x09, TARGET_REPORT_KEYBOARD }, /* F */
	{ 0x0205, 0x0D, TARGET_REPORT_KEYBOARD }, /* J */
	{ 0x0206, 0x0E, TARGET_REPORT_KEYBOARD }, /* K */
	{ 0x0207, 0x0F, TARGET_REPORT_KEYBOARD }, /* L */
	{ 0x0208, 0x33, TARGET_REPORT_KEYBOARD }, /* ; */
	{ 0x020A, 0x31, TARGET_REPORT_KEYBOARD }, /* \ */
	{ 0x020B, 0x59, TARGET_REPORT_KEYBOARD }, /* keypad 1 */
	{ 0x020C, 0x5A, TARGET_REPORT_KEYBOARD }, /* keypad 2 */
	{ 0x020D, 0x5B, TARGET_REPORT_KEYBOARD }, /* keypad 3 */
	{ 0x020E, 0x58, TARGET_REPORT_KEYBOARD }, /* keypad enter */
	{ 0x020F, 0xE5, TARGET_REPORT_KEYBOARD }, /* right shift */
	{ 0x0301, 0x29, TARGET_REPORT_KEYBOARD }, /* esc */
	{ 0x0303, 0x3D, TARGET_REPORT_KEYBOARD }, /* f4 */
	{ 0x0304, 0x0A, TARGET_REPORT_KEYBOARD }, /* G */
	{ 0x0305, 0x0B, TARGET_REPORT_KEYBOARD }, /* H */
	{ 0x0306, 0x3F, TARGET_REPORT_KEYBOARD }, /* f6 */
	{ 0x0308, 0x34, TARGET_REPORT_KEYBOARD }, /* ' */
	{ 0x0309, 0xE2, TARGET_REPORT_KEYBOARD }, /* left alt */
	{ 0x030A, 0x44, TARGET_REPORT_KEYBOARD }, /* f11 */
	{ 0x030C, 0x62, TARGET_REPORT_KEYBOARD }, /* keypad 0 */
	{ 0x030D, 0x63, TARGET_REPORT_KEYBOARD }, /* keypad . */
	{ 0x030E, 0x50, TARGET_REPORT_KEYBOARD }, /* arrow left */
	{ 0x0311, 0x81, TARGET_REPORT_KEYBOARD }, /* volume down */
	{ 0x0400, 0xE4, TARGET_REPORT_KEYBOARD }, /* right ctrl */
	{ 0x0401, 0x1D, TARGET_REPORT_KEYBOARD }, /* Z */
	{ 0x0402, 0x1B, TARGET_REPORT_KEYBOARD }, /* X */
	{ 0x0403, 0x06, TARGET_REPORT_KEYBOARD }, /* C */
	{ 0x0404, 0x19, TARGET_REPORT_KEYBOARD }, /* V */
	{ 0x0405, 0x10, TARGET_REPORT_KEYBOARD }, /* M */
	{ 0x0406, 0x36, TARGET_REPORT_KEYBOARD }, /* , */
	{ 0x0407, 0x37, TARGET_REPORT_KEYBOARD }, /* . */
	{ 0x040A, 0x28, TARGET_REPORT_KEYBOARD }, /* enter */
	{ 0x040B, 0x53, TARGET_REPORT_KEYBOARD }, /* num lock */
	{ 0x040C, 0x54, TARGET_REPORT_KEYBOARD }, /* keypad / */
	{ 0x040D, 0x55, TARGET_REPORT_KEYBOARD }, /* keypad * */
	{ 0x0504, 0x05, TARGET_REPORT_KEYBOARD }, /* B */
	{ 0x0505, 0x11, TARGET_REPORT_KEYBOARD }, /* N */
	{ 0x0507, 0xE7, TARGET_REPORT_KEYBOARD }, /* right gui */
	{ 0x0508, 0x38, TARGET_REPORT_KEYBOARD }, /* / */
	{ 0x0509, 0xE6, TARGET_REPORT_KEYBOARD }, /* right alt */
	{ 0x050A, 0x45, TARGET_REPORT_KEYBOARD }, /* f12 */
	{ 0x050B, 0x51, TARGET_REPORT_KEYBOARD }, /* arrow down */
	{ 0x050C, 0x4F, TARGET_REPORT_KEYBOARD }, /* arrow right */
	{ 0x050D, 0x56, TARGET_REPORT_KEYBOARD }, /* keypad - */
	{ 0x050E, 0x52, TARGET_REPORT_KEYBOARD }, /* arrow up */
	{ 0x0511, 0x80, TARGET_REPORT_KEYBOARD }, /* volume up */
	{ 0x0600, 0xE0, TARGET_REPORT_KEYBOARD }, /* left ctrl */
	{ 0x0601, 0x35, TARGET_REPORT_KEYBOARD }, /* ~ */
	{ 0x0602, 0x3A, TARGET_REPORT_KEYBOARD }, /* f1 */
	{ 0x0603, 0x3B, TARGET_REPORT_KEYBOARD }, /* f2 */
	{ 0x0604, 0x22, TARGET_REPORT_KEYBOARD }, /* 5 */
	{ 0x0605, 0x23, TARGET_REPORT_KEYBOARD }, /* 6 */
	{ 0x0606, 0x2E, TARGET_REPORT_KEYBOARD }, /* = */
	{ 0x0607, 0x41, TARGET_REPORT_KEYBOARD }, /* f8 */
	{ 0x0608, 0x2D, TARGET_REPORT_KEYBOARD }, /* - */
	{ 0x060A, 0x42, TARGET_REPORT_KEYBOARD }, /* f9 */
	{ 0x060B, 0x4C, TARGET_REPORT_KEYBOARD }, /* delete */
	{ 0x060C, 0x49, TARGET_REPORT_KEYBOARD }, /* insert */
	{ 0x060D, 0x4B, TARGET_REPORT_KEYBOARD }, /* page up */
	{ 0x060E, 0x4A, TARGET_REPORT_KEYBOARD }, /* home */
	{ 0x0611, 0x7F, TARGET_REPORT_KEYBOARD }, /* mute */
	{ 0x0700, 0x3E, TARGET_REPORT_KEYBOARD }, /* f5 */
	{ 0x0701, 0x1E, TARGET_REPORT_KEYBOARD }, /* 1 */
	{ 0x0702, 0x1F, TARGET_REPORT_KEYBOARD }, /* 2 */
	{ 0x0703, 0x20, TARGET_REPORT_KEYBOARD }, /* 3 */
	{ 0x0704, 0x21, TARGET_REPORT_KEYBOARD }, /* 4 */
	{ 0x0705, 0x24, TARGET_REPORT_KEYBOARD }, /* 7 */
	{ 0x0706, 0x25, TARGET_REPORT_KEYBOARD }, /* 8 */
	{ 0x0707, 0x26, TARGET_REPORT_KEYBOARD }, /* 9 */
	{ 0x0708, 0x27, TARGET_REPORT_KEYBOARD }, /* 0 */
	{ 0x070A, 0x43, TARGET_REPORT_KEYBOARD }, /* f10 */
	{ 0x070D, 0x4E, TARGET_REPORT_KEYBOARD }, /* page down */
	{ 0x070E, 0x4D, TARGET_REPORT_KEYBOARD }, /* end */
};
