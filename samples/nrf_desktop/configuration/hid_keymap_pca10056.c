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
	/*
	 * Intentionally empty.
	 * Should be filled to add keyboard/mouse buttons support to PCA10056.
	 */
};

const size_t hid_keymap_size = ARRAY_SIZE(hid_keymap);
