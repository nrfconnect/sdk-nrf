/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
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
	{ 0x0000, 0x05, TARGET_REPORT_MOUSE }, /* Additional Mouse Button */
	{ 0x0001, 0x04, TARGET_REPORT_MOUSE }, /* Additional Mouse Button */
	{ 0x0102, 0x07, TARGET_REPORT_MOUSE }, /* Additional Mouse Button */
	{ 0x0103, 0x08, TARGET_REPORT_MOUSE }, /* Additional Mouse Button */
	{ 0x0200, 0x03, TARGET_REPORT_MOUSE }, /* Middle Mouse Button */
	{ 0x0201, 0x01, TARGET_REPORT_MOUSE }, /* Left Mouse Button */
	{ 0x0300, 0x02, TARGET_REPORT_MOUSE }, /* Right Mouse Button */
};
