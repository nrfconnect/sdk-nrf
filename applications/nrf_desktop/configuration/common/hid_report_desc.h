/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HID_REPORT_DESC_H_
#define _HID_REPORT_DESC_H_

#include <stddef.h>
#include <zephyr/types.h>
#include <toolchain/common.h>
#include <sys/util.h>

#include "hid_report_mouse.h"
#include "hid_report_keyboard.h"
#include "hid_report_system_ctrl.h"
#include "hid_report_consumer_ctrl.h"
#include "hid_report_user_config.h"


#ifdef __cplusplus
extern "C" {
#endif

enum report_id {
	REPORT_ID_RESERVED,

	REPORT_ID_MOUSE,
	REPORT_ID_KEYBOARD_KEYS,
	REPORT_ID_SYSTEM_CTRL,
	REPORT_ID_CONSUMER_CTRL,

	REPORT_ID_KEYBOARD_LEDS,

	REPORT_ID_USER_CONFIG,
	REPORT_ID_USER_CONFIG_OUT,

	REPORT_ID_VENDOR_IN,
	REPORT_ID_VENDOR_OUT,

	REPORT_ID_BOOT_MOUSE,
	REPORT_ID_BOOT_KEYBOARD,

	REPORT_ID_COUNT
};

static const uint8_t input_reports[] = {
	REPORT_ID_MOUSE,
	REPORT_ID_KEYBOARD_KEYS,
	REPORT_ID_SYSTEM_CTRL,
	REPORT_ID_CONSUMER_CTRL,
	REPORT_ID_BOOT_MOUSE,
	REPORT_ID_BOOT_KEYBOARD,
};


extern const uint8_t hid_report_desc[];
extern const size_t hid_report_desc_size;

#ifdef __cplusplus
}
#endif

#endif /* _HID_REPORT_DESC_H_ */
