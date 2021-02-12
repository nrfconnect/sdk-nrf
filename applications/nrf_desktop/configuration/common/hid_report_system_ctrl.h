/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HID_REPORT_SYSTEM_CTRL_H_
#define _HID_REPORT_SYSTEM_CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define REPORT_SIZE_SYSTEM_CTRL		2 /* bytes */

#define REPORT_MASK_SYSTEM_CTRL		{} /* Store the whole report */

#define SYSTEM_CTRL_REPORT_KEY_COUNT_MAX	1


#define REPORT_MAP_SYSTEM_CTRL(report_id)				\
									\
	0x85, report_id,						\
	0x15, 0x00,       /* Logical minimum (0) */			\
	0x26, 0xFF, 0x03, /* Logical maximum (0x3FF) */			\
	0x19, 0x00,       /* Usage minimum (0) */			\
	0x2A, 0xFF, 0x03, /* Usage maximum (0x3FF) */			\
	0x75, 0x10,       /* Report Size (16) */			\
	0x95, SYSTEM_CTRL_REPORT_KEY_COUNT_MAX, /* Report Count */	\
	0x81, 0x00        /* Input (Data,Array,Absolute) */


#ifdef __cplusplus
}
#endif

#endif /* _HID_REPORT_SYSTEM_CTRL_H_ */
