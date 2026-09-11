/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RTFW_HID_INTERNAL_H_
#define RTFW_HID_INTERNAL_H_

#include <stdint.h>
#include <zephyr/sys/util.h>
#include <rtfw/rtfw.h>

#define RTFW_HID_COMMAND_CONFIGURE 1U
#define RTFW_HID_EVENT_EDGE        (RTFW_EVENT_TYPE_USER_BASE + 0x100U)

struct rtfw_hid_config {
	uint32_t enabled;
};

BUILD_ASSERT(sizeof(struct rtfw_hid_config) <=
	     CONFIG_RTFW_COMMAND_DATA_SIZE,
	     "RTFW command payload is too small for the HID configuration");

int rtfw_hid_command_handler(const struct rtfw_command *command,
			     void *user_data);
void rtfw_hid_fastpath_handler(void *user_data);
void rtfw_hid_pend_source_irq(void *user_data);
void rtfw_hid_fastpath_init(void);

#endif /* RTFW_HID_INTERNAL_H_ */
