/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hid_keymap.h"
#include <caf/key_id.h>

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
 * https://www.usb.org/sites/default/files/hut1_12.pdf
 */
static const struct hid_keymap hid_keymap[] = {
	{ KEY_ID(0x00, 0x00), 0x0004, REPORT_ID_KEYBOARD_KEYS }, /* A */
	{ KEY_ID(0x00, 0x01), 0x0005, REPORT_ID_KEYBOARD_KEYS }, /* B */
	{ KEY_ID(0x00, 0x02), 0x00E1, REPORT_ID_KEYBOARD_KEYS }, /* left shift */

	{ KEY_ID(0x00, 0x04), 0x0004, REPORT_ID_KEYBOARD_KEYS }, /* A */
	{ KEY_ID(0x00, 0x05), 0x0005, REPORT_ID_KEYBOARD_KEYS }, /* B */
	{ KEY_ID(0x00, 0x06), 0x0006, REPORT_ID_KEYBOARD_KEYS }, /* C */
	{ KEY_ID(0x00, 0x07), 0x0007, REPORT_ID_KEYBOARD_KEYS }, /* D */
	{ KEY_ID(0x00, 0x08), 0x0008, REPORT_ID_KEYBOARD_KEYS }, /* E */
	{ KEY_ID(0x00, 0x09), 0x0009, REPORT_ID_KEYBOARD_KEYS }, /* F */
	{ KEY_ID(0x00, 0x0A), 0x000A, REPORT_ID_KEYBOARD_KEYS }, /* G */
	{ KEY_ID(0x00, 0x0B), 0x000B, REPORT_ID_KEYBOARD_KEYS }, /* H */
	{ KEY_ID(0x00, 0x0C), 0x000C, REPORT_ID_KEYBOARD_KEYS }, /* I */
	{ KEY_ID(0x00, 0x0D), 0x000D, REPORT_ID_KEYBOARD_KEYS }, /* J */
	{ KEY_ID(0x00, 0x0E), 0x000E, REPORT_ID_KEYBOARD_KEYS }, /* K */
	{ KEY_ID(0x00, 0x0F), 0x000F, REPORT_ID_KEYBOARD_KEYS }, /* L */
	{ KEY_ID(0x00, 0x10), 0x0010, REPORT_ID_KEYBOARD_KEYS }, /* M */
	{ KEY_ID(0x00, 0x11), 0x0011, REPORT_ID_KEYBOARD_KEYS }, /* N */
	{ KEY_ID(0x00, 0x12), 0x0012, REPORT_ID_KEYBOARD_KEYS }, /* O */
	{ KEY_ID(0x00, 0x13), 0x0013, REPORT_ID_KEYBOARD_KEYS }, /* P */
	{ KEY_ID(0x00, 0x14), 0x0014, REPORT_ID_KEYBOARD_KEYS }, /* Q */
	{ KEY_ID(0x00, 0x15), 0x0015, REPORT_ID_KEYBOARD_KEYS }, /* R */
	{ KEY_ID(0x00, 0x16), 0x0016, REPORT_ID_KEYBOARD_KEYS }, /* S */
	{ KEY_ID(0x00, 0x17), 0x0017, REPORT_ID_KEYBOARD_KEYS }, /* T */
	{ KEY_ID(0x00, 0x18), 0x0018, REPORT_ID_KEYBOARD_KEYS }, /* U */
	{ KEY_ID(0x00, 0x19), 0x0019, REPORT_ID_KEYBOARD_KEYS }, /* V */
	{ KEY_ID(0x00, 0x1A), 0x001A, REPORT_ID_KEYBOARD_KEYS }, /* W */
	{ KEY_ID(0x00, 0x1B), 0x001B, REPORT_ID_KEYBOARD_KEYS }, /* X */
	{ KEY_ID(0x00, 0x1C), 0x001C, REPORT_ID_KEYBOARD_KEYS }, /* Y */
	{ KEY_ID(0x00, 0x1D), 0x001D, REPORT_ID_KEYBOARD_KEYS }, /* Z */

	{ KEY_ID(0x00, 0x2C), 0x002C, REPORT_ID_KEYBOARD_KEYS }, /* spacebar */
};
