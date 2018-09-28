/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */


#ifndef _HID_KEYMAP_H_
#define _HID_KEYMAP_H_

#include <stddef.h>

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum target_report {
	TARGET_REPORT_MOUSE_BUTTON,
	TARGET_REPORT_MOUSE_WHEEL,
	TARGET_REPORT_MOUSE_PAN,
	TARGET_REPORT_KEYBOARD,
	TARGET_REPORT_MPLAYER,

	TARGET_REPORT_COUNT,
};

struct hid_keymap {
	u16_t			key_id;
	u16_t			usage_id;
	enum target_report	target_report;
};

extern const struct hid_keymap	hid_keymap[];
extern const size_t		hid_keymap_size;

#ifdef __cplusplus
}
#endif

#endif /* _HID_KEYMAP_H_ */

