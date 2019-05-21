/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _HID_KEYMAP_H_
#define _HID_KEYMAP_H_

#include <stddef.h>
#include <toolchain/common.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Target report list. */
#define TARGET_REPORT_LIST	\
	X(MOUSE)		\
	X(KEYBOARD)		\
	X(MPLAYER)

/** @brief Target reports. */
enum target_report {
#define X(name) _CONCAT(TARGET_REPORT_, name),
	TARGET_REPORT_LIST
#undef X

	TARGET_REPORT_COUNT
};


/** @brief HID map entry. */
struct hid_keymap {
	u16_t			key_id;		/**< Key HW id. */
	u16_t			usage_id;	/**< Assigned usage. */
	enum target_report	target_report;	/**< Id of the target report. */
};

#ifdef __cplusplus
}
#endif

#endif /* _HID_KEYMAP_H_ */
