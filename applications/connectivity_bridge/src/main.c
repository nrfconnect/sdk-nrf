/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <app_event_manager.h>
#include <hw_id.h>

#define MODULE main
#include "module_state_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);

static uint8_t usb_serial_str[] = "THINGY91_12PLACEHLDRS";

/* Overriding weak function to set iSerialNumber at runtime. */
uint8_t *usb_update_sn_string_descriptor(void)
{
	char buf[HW_ID_LEN] = {0};

	if (!hw_id_get(buf, ARRAY_SIZE(buf))) {
		snprintk(usb_serial_str, sizeof(usb_serial_str), "THINGY91X_%s", buf);
	}

	return usb_serial_str;
}

int main(void)
{
	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}
	return 0;
}
