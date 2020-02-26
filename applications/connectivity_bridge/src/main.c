/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <event_manager.h>

#define MODULE main
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define USB_SERIALNUMBER_TEMPLATE "PCA20035_%04X%08X"

static u8_t usb_serial_str[] = "PCA20035_12PLACEHLDRS";

/* Overriding weak function to set iSerialNumber at runtime. */
u8_t *usb_update_sn_string_descriptor(void)
{
	snprintk(usb_serial_str, sizeof(usb_serial_str), USB_SERIALNUMBER_TEMPLATE,
				(uint32_t)(NRF_FICR->DEVICEADDR[1] & 0x0000FFFF)|0x0000C000,
				(uint32_t)NRF_FICR->DEVICEADDR[0]);

	return usb_serial_str;
}

void main(void)
{
	if (event_manager_init()) {
		LOG_ERR("Event manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}
}
