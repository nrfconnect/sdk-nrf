/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HID_KEYMAP_H_
#define _HID_KEYMAP_H_

#include <stddef.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/types.h>

#include "hid_report_desc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief HID map entry. */
struct hid_keymap {
	uint16_t key_id;	/**< Key HW id. */
	uint16_t usage_id;	/**< Assigned usage. */
	uint8_t report_id;	/**< Id of the target report. */
};

#ifdef __cplusplus
}
#endif

#endif /* _HID_KEYMAP_H_ */
