/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
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
	{ KEY_ID(0, 0), 0x01, REPORT_ID_MOUSE }, /* Left Mouse Button */
	{ KEY_ID(0, 1), 0x02, REPORT_ID_MOUSE }, /* Right Mouse Button */
	{ KEY_ID(0, 2), 0x03, REPORT_ID_MOUSE }, /* Middle Mouse Button */
	{ KEY_ID(0, 3), 0x04, REPORT_ID_MOUSE }, /* Additional Mouse Button */
	{ KEY_ID(0, 4), 0x05, REPORT_ID_MOUSE }, /* Additional Mouse Button */
	{ KEY_ID(0, 5), 0x06, REPORT_ID_MOUSE }, /* Additional Mouse Button */
	{ KEY_ID(0, 6), 0x07, REPORT_ID_MOUSE }, /* Additional Mouse Button */
};
