/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include "hid_report_desc.h"

const uint8_t hid_report_desc[] = {

#if CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT
	0x05, 0x01,     /* Usage Page (Generic Desktop) */
	0x09, 0x02,     /* Usage (Mouse) */
	0xA1, 0x01,     /* Collection (Application) */
	REPORT_MAP_MOUSE(REPORT_ID_MOUSE),
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
	REPORT_MAP_USER_CONFIG(REPORT_ID_USER_CONFIG),
#if CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT
	REPORT_MAP_USER_CONFIG_OUT(REPORT_ID_USER_CONFIG_OUT),
#endif
#endif
	0xC0,           /* End Collection (Application) */
#endif

#if CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT
	0x05, 0x01,     /* Usage Page (Generic Desktop) */
	0x09, 0x06,     /* Usage (Keyboard) */
	0xA1, 0x01,     /* Collection (Application) */
	REPORT_MAP_KEYBOARD(REPORT_ID_KEYBOARD_KEYS, REPORT_ID_KEYBOARD_LEDS),
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE && !CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT
	REPORT_MAP_USER_CONFIG(REPORT_ID_USER_CONFIG),
#if CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT
	REPORT_MAP_USER_CONFIG_OUT(REPORT_ID_USER_CONFIG_OUT),
#endif
#endif
	0xC0,           /* End Collection (Application) */
#endif

#if CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT
	0x05, 0x01,     /* Usage page (Generic Desktop) */
	0x09, 0x80,	/* Usage (System Control) */
	0xA1, 0x01,     /* Collection (Application) */
	REPORT_MAP_SYSTEM_CTRL(REPORT_ID_SYSTEM_CTRL),
	0xC0,           /* End Collection (Application) */
#endif

#if CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT
	0x05, 0x0C,	/* Usage page (Consumer Control) */
	0x09, 0x01,     /* Usage (Consumer Control) */
	0xA1, 0x01,     /* Collection (Application) */
	REPORT_MAP_CONSUMER_CTRL(REPORT_ID_CONSUMER_CTRL),
	0xC0,           /* End Collection (Application) */
#endif
};

const size_t hid_report_desc_size = sizeof(hid_report_desc);
