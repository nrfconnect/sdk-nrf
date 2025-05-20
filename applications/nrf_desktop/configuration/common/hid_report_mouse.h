/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HID_REPORT_MOUSE_H_
#define _HID_REPORT_MOUSE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Mouse report uses 5 bytes:
 *  8 bits - pressed buttons bitmask
 *  8 bits - wheel rotation
 * 12 bits - x movement
 * 12 bits - y movement
 */
#define REPORT_SIZE_MOUSE		5 /* Bytes */

/* Mouse boot report uses 3 bytes:
 * 8 bits - pressed buttons bitmask
 * 8 bits - x movement
 * 8 bits - y movement
 */
#define REPORT_SIZE_MOUSE_BOOT		3 /* Bytes */

/* Report mask marks which bytes should are absolute and should be stored. */
#define REPORT_MASK_MOUSE		{0x01}


#define USAGE_PAGE_MOUSE_XY		0x01
#define USAGE_PAGE_MOUSE_WHEEL		0x01
#define USAGE_PAGE_KEYBOARD		0x07
#define USAGE_PAGE_LEDS			0x08
#define USAGE_PAGE_MOUSE_BUTTONS	0x09

#define MOUSE_REPORT_WHEEL_MIN		(-0x7F)
#define MOUSE_REPORT_WHEEL_MAX		(0x7F)
#define MOUSE_REPORT_XY_MIN		(-0x07ff)
#define MOUSE_REPORT_XY_MAX		(0x07ff)
#define MOUSE_REPORT_XY_MIN_BOOT	(-0x80)
#define MOUSE_REPORT_XY_MAX_BOOT	(0x7f)
#define MOUSE_REPORT_BUTTON_COUNT_MAX	8

enum {
	MOUSE_REPORT_AXIS_X,
	MOUSE_REPORT_AXIS_Y,
	MOUSE_REPORT_AXIS_WHEEL,
	MOUSE_REPORT_AXIS_COUNT
};


#define REPORT_MAP_MOUSE(report_id)					\
									\
	/* Report: Mouse */						\
	0x09, 0x01,       /* Usage (Pointer) */				\
	0xA1, 0x00,       /* Collection (Physical) */			\
									\
	0x85, report_id,    /* Report ID */				\
									\
	0x05, USAGE_PAGE_MOUSE_BUTTONS,					\
	0x19, 0x01,         /* Usage Minimum (1) */			\
	0x29, 0x08,         /* Usage Maximum (8) */			\
	0x15, 0x00,         /* Logical Minimum (0) */			\
	0x25, 0x01,         /* Logical Maximum (1) */			\
	0x75, 0x01,         /* Report Size (1) */			\
	0x95, MOUSE_REPORT_BUTTON_COUNT_MAX, /* Report Count */		\
	0x81, 0x02,         /* Input (Data, Variable, Absolute) */	\
									\
	0x05, USAGE_PAGE_MOUSE_WHEEL,					\
	0x09, 0x38,         /* Usage (Wheel) */				\
	0x15, 0x81,         /* Logical Minimum (-127) */		\
	0x25, 0x7F,         /* Logical Maximum (127) */			\
	0x75, 0x08,         /* Report Size (8) */			\
	0x95, 0x01,         /* Report Count (1) */			\
	0x81, 0x06,         /* Input (Data, Variable, Relative) */	\
									\
	0x05, USAGE_PAGE_MOUSE_XY,					\
	0x09, 0x30,         /* Usage (X) */				\
	0x09, 0x31,         /* Usage (Y) */				\
	0x16, 0x01, 0xF8,   /* Logical Maximum (2047) */		\
	0x26, 0xFF, 0x07,   /* Logical Minimum (-2047) */		\
	0x75, 0x0C,         /* Report Size (12) */			\
	0x95, 0x02,         /* Report Count (2) */			\
	0x81, 0x06,         /* Input (Data, Variable, Relative) */	\
									\
	0xC0              /* End Collection (Physical) */

#ifdef __cplusplus
}
#endif

#endif /* _HID_REPORT_MOUSE_H_ */
