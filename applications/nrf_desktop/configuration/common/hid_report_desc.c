/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include "hid_report_desc.h"

#define USER_CONFIG_FEATURE_REPORT(id, size)				\
	0x05, 0x01,       /* Usage Page (Generic Desktop) */		\
	0xA1, 0x01,       /* Collection (Application) */		\
	0x85, id,							\
	0x09, 0x05,       /* Usage (Vendor Defined) */			\
	0x15, 0x00,       /* Logical Minimum (0) */			\
	0x25, 0xFF,       /* Logical Maximum (255) */			\
	0x75, 0x08,       /* Report Size (8) */				\
	0x95, size,       /* Report Count */				\
	0xB1, 0x02,       /* Feature (Data, Variable, Absolute) */	\
	0xC0              /* End Collection (Application) */


const u8_t hid_report_desc[] = {
#if CONFIG_DESKTOP_HID_MOUSE
	/* Usage page */
	0x05, 0x01,     /* Usage Page (Generic Desktop) */
	0x09, 0x02,     /* Usage (Mouse) */

	0xA1, 0x01,     /* Collection (Application) */

	/* Report: Mouse */
	0x09, 0x01,       /* Usage (Pointer) */
	0xA1, 0x00,       /* Collection (Physical) */
	0x85, REPORT_ID_MOUSE,

	0x05, USAGE_PAGE_MOUSE_BUTTONS,
	0x19, 0x01,         /* Usage Minimum (1) */
	0x29, 0x08,         /* Usage Maximum (8) */
	0x15, 0x00,         /* Logical Minimum (0) */
	0x25, 0x01,         /* Logical Maximum (1) */
	0x75, 0x01,         /* Report Size (1) */
	0x95, 0x08,         /* Report Count (8) */
	0x81, 0x02,         /* Input (Data, Variable, Absolute) */

	0x05, USAGE_PAGE_MOUSE_WHEEL,
	0x09, 0x38,         /* Usage (Wheel) */
	0x15, 0x81,         /* Logical Minimum (-127) */
	0x25, 0x7F,         /* Logical Maximum (127) */
	0x75, 0x08,         /* Report Size (8) */
	0x95, 0x01,         /* Report Count (1) */
	0x81, 0x06,         /* Input (Data, Variable, Relative) */

	0x05, USAGE_PAGE_MOUSE_XY,
	0x09, 0x30,         /* Usage (X) */
	0x09, 0x31,         /* Usage (Y) */
	0x16, 0x01, 0xF8,   /* Logical Maximum (2047) */
	0x26, 0xFF, 0x07,   /* Logical Minimum (-2047) */
	0x75, 0x0C,         /* Report Size (12) */
	0x95, 0x02,         /* Report Count (2) */
	0x81, 0x06,         /* Input (Data, Variable, Relative) */

	0xC0,             /* End Collection (Physical) */
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
	USER_CONFIG_FEATURE_REPORT(REPORT_ID_USER_CONFIG, REPORT_SIZE_USER_CONFIG),
#endif
	0xC0,           /* End Collection (Application) */
#endif

#if CONFIG_DESKTOP_HID_KEYBOARD
	/* Usage page - Keyboard */
	0x05, 0x01,     /* Usage Page (Generic Desktop) */
	0x09, 0x06,     /* Usage (Keyboard) */

	0xA1, 0x01,     /* Collection (Application) */

	/* Report: Keyboard Keys (input) */
	0x85, REPORT_ID_KEYBOARD_KEYS,

	/* Keyboard - Modifiers */
	0x05, USAGE_PAGE_KEYBOARD,
	0x19, KEYBOARD_REPORT_FIRST_MODIFIER, /* Usage Minimum */
	0x29, KEYBOARD_REPORT_LAST_MODIFIER,  /* Usage Maximum */
	0x15, 0x00,       /* Logical Minimum (0) */
	0x25, 0x01,       /* Logical Maximum (1) */
	0x75, 0x01,       /* Report Size (1) */
	0x95, 0x08,       /* Report Count (8) */
	0x81, 0x02,       /* Input (Data, Variable, Absolute) */

	/* Keyboard - Reserved */
	0x75, 0x08,       /* Report Size (8) */
	0x95, 0x01,       /* Report Count (1) */
	0x81, 0x01,       /* Input (Constant) */

	/* Keyboard - Keys */
	0x05, USAGE_PAGE_KEYBOARD,
	0x19, 0x00,       /* Usage Minimum (0) */
	0x29, KEYBOARD_REPORT_LAST_KEY, /* Usage Maximum */
	0x15, 0x00,       /* Logical Minimum (0) */
	0x25, KEYBOARD_REPORT_LAST_KEY, /* Logical Maximum */
	0x75, 0x08,       /* Report Size (8) */
	0x95, 0x06,       /* Report Count (6) */
	0x81, 0x00,       /* Input (Data, Array) */

	/* Report: Keyboard LEDS (output) */
	0x85, REPORT_ID_KEYBOARD_LEDS,

	/* Keyboard - LEDs */
	0x05, USAGE_PAGE_LEDS,
	0x19, 0x01,       /* Usage Minimum (1) */
	0x29, 0x05,       /* Usage Maximum (5) */
	0x95, 0x05,       /* Report Count (5) */
	0x75, 0x01,       /* Report Size (1) */
	0x91, 0x02,       /* Output (Data, Variable, Absolute) */

	/* Keyboard - LEDs padding */
	0x95, 0x01,       /* Report Count (1) */
	0x75, 0x03,       /* Report Size (3) (padding) */
	0x91, 0x01,       /* Output (Constant, Array, Absolute) */

#if (CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE && !CONFIG_DESKTOP_HID_MOUSE)
	USER_CONFIG_FEATURE_REPORT(REPORT_ID_USER_CONFIG, REPORT_SIZE_USER_CONFIG),
#endif
	0xC0,           /* End Collection (Application) */
#endif

#if CONFIG_DESKTOP_HID_CONSUMER_CTRL
	/* Usage page - Consumer Control */
	0x05, USAGE_PAGE_CONSUMER_CTRL,
	0x09, 0x01,     /* Usage (Consumer Control) */

	0xA1, 0x01,     /* Collection (Application) */

	0x85, REPORT_ID_CONSUMER_CTRL,
	0x15, 0x00,       /* Logical minimum (0) */
	0x25, 0x01,       /* Logical maximum (1) */
	0x75, 0x01,       /* Report Size (1) */
	0x95, 0x02,       /* Report Count (2) */

	0x09, 0xEA,       /* Usage (Volume Decrement)		BIT 0  */
	0x09, 0xE9,       /* Usage (Volume Increment)		BIT 1  */
	0x81, 0x02,	  /* Input (Data,Variable,Absolute) */

	0x95, 0x09,       /* Report Count (9) */
	0x09, 0xE2,	  /* Usage (Mute)			BIT 2  */

	0x09, 0xCD,       /* Usage (Play/Pause)			BIT 3  */
	0x09, 0xB5,       /* Usage (Scan Next Track)		BIT 4  */
	0x09, 0xB6,       /* Usage (Scan Previous Track)	BIT 5  */

	0x09, 0x32,	  /* Usage (Sleep)			BIT 6  */

	0x0A, 0x1F, 0x02, /* Usage (AC Find)			BIT 7  */
	0x0A, 0x92, 0x01, /* Usage (AL Calculator)		BIT 8  */
	0x0A, 0x8A, 0x01, /* Usage (AL Email Reader)		BIT 9  */
	0x0A, 0x96, 0x01, /* Usage (AL Internet Browser)	BIT 10 */
	0x81, 0x06,	  /* Input (Data,Variable,Relative) */

	0x95, 0x05,       /* Report Count (5) (padding) */
	0x81, 0x01,       /* Input (Constant) */

	0xC0            /* End Collection */
#endif
};

const size_t hid_report_desc_size = sizeof(hid_report_desc);
