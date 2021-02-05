/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HID_REPORT_KEYBOARD_H_
#define _HID_REPORT_KEYBOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Keyboard report uses 8 bytes for input and 1 byte for output.
 * Input bytes:
 *     8 bits - pressed modifier buttons bitmask
 *     8 bits - reserved
 * 6 x 8 bits - 6 slots for pressed key ids
 *
 * Output bytes:
 *     8 bits - active LED indicators bitmask
 */
#define REPORT_SIZE_KEYBOARD_KEYS	8 /* bytes */
#define REPORT_SIZE_KEYBOARD_LEDS	1 /* bytes */

/* Report mask marks which bytes should are absolute and should be stored. */
#define REPORT_MASK_KEYBOARD_KEYS	{} /* Store the whole report */


#define KEYBOARD_REPORT_LAST_KEY	0x65 /* Keyboard Application */
#define KEYBOARD_REPORT_FIRST_MODIFIER	0xE0 /* Keyboard Left Ctrl */
#define KEYBOARD_REPORT_LAST_MODIFIER	0xE7 /* Keyboard Right GUI */
#define KEYBOARD_REPORT_KEY_COUNT_MAX	6


#define REPORT_MAP_KEYBOARD(report_id_keys, report_id_leds)		\
									\
	/* Report: Keyboard Keys (input) */				\
	0x85, report_id_keys,						\
									\
	/* Keyboard - Modifiers */					\
	0x05, USAGE_PAGE_KEYBOARD,					\
	0x19, KEYBOARD_REPORT_FIRST_MODIFIER, /* Usage Minimum */	\
	0x29, KEYBOARD_REPORT_LAST_MODIFIER,  /* Usage Maximum */	\
	0x15, 0x00,       /* Logical Minimum (0) */			\
	0x25, 0x01,       /* Logical Maximum (1) */			\
	0x75, 0x01,       /* Report Size (1) */				\
	0x95, 0x08,       /* Report Count (8) */			\
	0x81, 0x02,       /* Input (Data, Variable, Absolute) */	\
									\
	/* Keyboard - Reserved */					\
	0x75, 0x08,       /* Report Size (8) */				\
	0x95, 0x01,       /* Report Count (1) */			\
	0x81, 0x01,       /* Input (Constant) */			\
									\
	/* Keyboard - Keys */						\
	0x05, USAGE_PAGE_KEYBOARD,					\
	0x19, 0x00,       /* Usage Minimum (0) */			\
	0x29, KEYBOARD_REPORT_LAST_KEY, /* Usage Maximum */		\
	0x15, 0x00,       /* Logical Minimum (0) */			\
	0x25, KEYBOARD_REPORT_LAST_KEY, /* Logical Maximum */		\
	0x75, 0x08,       /* Report Size (8) */				\
	0x95, KEYBOARD_REPORT_KEY_COUNT_MAX, /* Report Count */		\
	0x81, 0x00,       /* Input (Data, Array) */			\
									\
	/* Report: Keyboard LEDS (output) */				\
	0x85, report_id_leds,						\
									\
	/* Keyboard - LEDs */						\
	0x05, USAGE_PAGE_LEDS,						\
	0x19, 0x01,       /* Usage Minimum (1) */			\
	0x29, 0x05,       /* Usage Maximum (5) */			\
	0x15, 0x00,       /* Logical Minimum (0) */			\
	0x25, 0x01,       /* Logical Maximum (1) */			\
	0x95, 0x05,       /* Report Count (5) */			\
	0x75, 0x01,       /* Report Size (1) */				\
	0x91, 0x02,       /* Output (Data, Variable, Absolute) */	\
									\
	/* Keyboard - LEDs padding */					\
	0x95, 0x01,       /* Report Count (1) */			\
	0x75, 0x03,       /* Report Size (3) (padding) */		\
	0x91, 0x01        /* Output (Constant, Array, Absolute) */


#ifdef __cplusplus
}
#endif

#endif /* _HID_REPORT_KEYBOARD_H_ */
