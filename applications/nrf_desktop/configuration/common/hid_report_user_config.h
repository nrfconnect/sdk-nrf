/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _HID_REPORT_USER_CONFIG_H_
#define _HID_REPORT_USER_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define REPORT_SIZE_USER_CONFIG                29 /* bytes */


#define REPORT_MAP_USER_CONFIG(report_id)				\
									\
	0x85, report_id,						\
	0x06, 0x00, 0xff, /* Usage Page (Vendor Defined 0xFF00) */	\
	0x0A, 0x01, 0xff, /* Usage (0xFF01) */				\
	0x15, 0x00,       /* Logical Minimum (0) */			\
	0x25, 0xFF,       /* Logical Maximum (255) */			\
	0x75, 0x08,       /* Report Size (8) */				\
	0x95, REPORT_SIZE_USER_CONFIG,       /* Report Count */		\
	0xB1, 0x02        /* Feature (Data, Variable, Absolute) */	\

#ifdef __cplusplus
}
#endif

#endif /* _HID_REPORT_USER_CONFIG_H_ */
