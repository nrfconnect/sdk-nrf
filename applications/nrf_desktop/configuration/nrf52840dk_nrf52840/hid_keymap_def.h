/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
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
	/* Intentionally left empty. nRF52840 DK by default uses all 4 buttons
	 * only to control generating motion.
	 */
};
