/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "hid_keymap.h"

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
	{ 0, 0x01, TARGET_REPORT_MOUSE }, /* Left Mouse Button */
	{ 1, 0x02, TARGET_REPORT_MOUSE }, /* Right Mouse Button */
	{ 2, 0x03, TARGET_REPORT_MOUSE }, /* Middle Mouse Button */
	{ 3, 0x04, TARGET_REPORT_MOUSE }, /* Additional Mouse Button */
	{ 4, 0x05, TARGET_REPORT_MOUSE }, /* Additional Mouse Button */
	{ 5, 0x06, TARGET_REPORT_MOUSE }, /* Additional Mouse Button */
	{ 6, 0x07, TARGET_REPORT_MOUSE }, /* Additional Mouse Button */
};
