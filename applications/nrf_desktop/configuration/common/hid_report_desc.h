/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Header file HID report identifiers.
 */

#ifndef _HID_REPORT_DESC_H_
#define _HID_REPORT_DESC_H_

#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/sys/util.h>

#include "hid_report_mouse.h"
#include "hid_report_keyboard.h"
#include "hid_report_system_ctrl.h"
#include "hid_report_consumer_ctrl.h"
#include "hid_report_user_config.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HID Reports
 * @defgroup nrf_desktop_hid_reports HID Reports
 * @{
 */

/** @brief Identification numbers of HID reports. */
enum report_id {
	/** Reserved. */
	REPORT_ID_RESERVED,

	/** Mouse input report. */
	REPORT_ID_MOUSE,
	/** Keyboard input report. */
	REPORT_ID_KEYBOARD_KEYS,
	/** System control input report. */
	REPORT_ID_SYSTEM_CTRL,
	/** Consumer control input report. */
	REPORT_ID_CONSUMER_CTRL,

	/** Keyboard output report. */
	REPORT_ID_KEYBOARD_LEDS,

	/** Config channel feature report. */
	REPORT_ID_USER_CONFIG,
	/** Config channel output report. */
	REPORT_ID_USER_CONFIG_OUT,

	/** Reserved. */
	REPORT_ID_VENDOR_IN,
	/** Reserved. */
	REPORT_ID_VENDOR_OUT,

	/** Boot report - mouse. */
	REPORT_ID_BOOT_MOUSE,
	/** Boot report - keyboard. */
	REPORT_ID_BOOT_KEYBOARD,

	/** Number of reports. */
	REPORT_ID_COUNT
};

/** @brief Input reports map. */
static const uint8_t input_reports[] = {
	REPORT_ID_MOUSE,
	REPORT_ID_KEYBOARD_KEYS,
	REPORT_ID_SYSTEM_CTRL,
	REPORT_ID_CONSUMER_CTRL,
	REPORT_ID_BOOT_MOUSE,
	REPORT_ID_BOOT_KEYBOARD,
};

/** @brief Output reports map. */
static const uint8_t output_reports[] = {
	REPORT_ID_KEYBOARD_LEDS,
};


extern const uint8_t hid_report_desc[];
extern const size_t hid_report_desc_size;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _HID_REPORT_DESC_H_ */
