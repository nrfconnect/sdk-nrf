/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <lwm2m_carrier.h>
#include <zephyr.h>

void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", (unsigned int)err);
}

void print_err(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_error_t *err =
		(lwm2m_carrier_event_error_t *)evt->data;

	static const char *strerr[] = {
		[LWM2M_CARRIER_ERROR_NO_ERROR] =
			"No error",
		[LWM2M_CARRIER_ERROR_BOOTSTRAP] =
			"Bootstrap error",
		[LWM2M_CARRIER_ERROR_CONNECT_FAIL] =
			"Failed to connect to the LTE network",
		[LWM2M_CARRIER_ERROR_DISCONNECT_FAIL] =
			"Failed to disconnect from the LTE network",
	};

	__ASSERT(PART_OF_ARRAY(strerr[err->code]),
		 "Unhandled liblwm2m_carrier error");

	printk("%s, reason %d\n", strerr[err->code], err->value);
}

int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	switch (event->type) {
	case LWM2M_CARRIER_EVENT_BSDLIB_INIT:
		printk("LWM2M_CARRIER_EVENT_BSDLIB_INIT\n");
		break;
	case LWM2M_CARRIER_EVENT_CONNECTING:
		printk("LWM2M_CARRIER_EVENT_CONNECTING\n");
		break;
	case LWM2M_CARRIER_EVENT_CONNECTED:
		printk("LWM2M_CARRIER_EVENT_CONNECTED\n");
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECTING:
		printk("LWM2M_CARRIER_EVENT_DISCONNECTING\n");
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECTED:
		printk("LWM2M_CARRIER_EVENT_DISCONNECTED\n");
		break;
	case LWM2M_CARRIER_EVENT_READY:
		printk("LWM2M_CARRIER_EVENT_READY\n");
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		printk("LWM2M_CARRIER_EVENT_FOTA_START\n");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		printk("LWM2M_CARRIER_EVENT_REBOOT\n");
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		printk("LWM2M_CARRIER_EVENT_ERROR\n");
		print_err(event);
		break;
	}

	return 0;
}

void main(void)
{
	printk("LWM2M Carrier library sample.\n");
}
