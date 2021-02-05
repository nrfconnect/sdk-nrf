/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifdef CONFIG_LWM2M_CARRIER
#include <lwm2m_carrier.h>
#endif /* CONFIG_LWM2M_CARRIER */
#include <zephyr.h>

#ifdef CONFIG_LWM2M_CARRIER
void nrf_modem_recoverable_error_handler(uint32_t err)
{
	printk("Modem library recoverable error: %u\n", (unsigned int)err);
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
		[LWM2M_CARRIER_ERROR_FOTA_PKG] =
			"Package refused from modem",
		[LWM2M_CARRIER_ERROR_FOTA_PROTO] =
			"Protocol error",
		[LWM2M_CARRIER_ERROR_FOTA_CONN] =
			"Connection to remote server failed",
		[LWM2M_CARRIER_ERROR_FOTA_CONN_LOST] =
			"Connection to remote server lost",
		[LWM2M_CARRIER_ERROR_FOTA_FAIL] =
			"Modem firmware update failed",
	};

	__ASSERT(PART_OF_ARRAY(strerr[err->code]),
		 "Unhandled liblwm2m_carrier error");

	printk("%s, reason %d\n", strerr[err->code], err->value);
}

void print_deferred(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_deferred_t *def =
		(lwm2m_carrier_event_deferred_t *)evt->data;

	static const char *strdef[] = {
		[LWM2M_CARRIER_DEFERRED_NO_REASON] =
			"No reason given",
		[LWM2M_CARRIER_DEFERRED_PDN_ACTIVATE] =
			"Failed to activate PDN",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_NO_ROUTE] =
			"No route to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_CONNECT] =
			"Failed to connect to bootstrap server",
		[LWM2M_CARRIER_DEFERRED_BOOTSTRAP_SEQUENCE] =
			"Bootstrap sequence not completed",
		[LWM2M_CARRIER_DEFERRED_SERVER_NO_ROUTE] =
			"No route to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_CONNECT] =
			"Failed to connect to server",
		[LWM2M_CARRIER_DEFERRED_SERVER_REGISTRATION] =
			"Server registration sequence not completed",
	};

	__ASSERT(PART_OF_ARRAY(strdef[def->reason]),
		 "Unhandled deferred reason");

	printk("Reason: %s, timeout: %d seconds\n", strdef[def->reason],
		def->timeout);
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
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		printk("LWM2M_CARRIER_EVENT_BOOTSTRAPPED\n");
		break;
	case LWM2M_CARRIER_EVENT_LTE_READY:
		printk("LWM2M_CARRIER_EVENT_LTE_READY\n");
		break;
	case LWM2M_CARRIER_EVENT_REGISTERED:
		printk("LWM2M_CARRIER_EVENT_REGISTERED\n");
		break;
	case LWM2M_CARRIER_EVENT_DEFERRED:
		printk("LWM2M_CARRIER_EVENT_DEFERRED\n");
		print_deferred(event);
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
#endif /* CONFIG_LWM2M_CARRIER */

void main(void)
{
	printk("LWM2M Carrier library sample.\n");
}
