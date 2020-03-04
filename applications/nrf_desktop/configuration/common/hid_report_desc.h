/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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

#define REPORT_ID_TO_IN_REPORT(report_id) (report_id - 1)
#define IN_REPORT_TO_REPORT_ID(in_report) (in_report + 1)

#define IN_REPORT_LIST		\
	X(MOUSE)		\
	X(KEYBOARD_KEYS)	\
	X(SYSTEM_CTRL)		\
	X(CONSUMER_CTRL)

#define OUT_REPORT_LIST		\
	X(KEYBOARD_LEDS)

#define FEATURE_REPORT_LIST	\
	X(USER_CONFIG)

enum in_report {
#define X(name) _CONCAT(IN_REPORT_, name),
	IN_REPORT_LIST
#undef X

	IN_REPORT_COUNT
};

enum out_report {
#define X(name) _CONCAT(OUT_REPORT_, name),
	OUT_REPORT_LIST
#undef X

	OUT_REPORT_COUNT
};

enum feature_report {
#define X(name) _CONCAT(FEATURE_REPORT_, name),
	FEATURE_REPORT_LIST
#undef X

	FEATURE_REPORT_COUNT
};


enum report_id {
	REPORT_ID_RESERVED,

#define X(name) _CONCAT(REPORT_ID_, name),
	IN_REPORT_LIST
	OUT_REPORT_LIST
	FEATURE_REPORT_LIST
#undef X

	REPORT_ID_COUNT
};

enum report_mode {
	REPORT_MODE_PROTOCOL,
	REPORT_MODE_BOOT,

	REPORT_MODE_COUNT
};


extern const u8_t hid_report_desc[];
extern const size_t hid_report_desc_size;

#ifdef __cplusplus
}
#endif

#endif /* _HID_REPORT_DESC_H_ */
