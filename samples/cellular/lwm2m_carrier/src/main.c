/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>

#ifdef CONFIG_LWM2M_CARRIER
#include <lwm2m_carrier.h>
#include "carrier_certs.h"

NRF_MODEM_LIB_ON_INIT(main_init_hook, on_modem_lib_init, NULL);

static void lte_event_handler(const struct lte_lc_evt *const evt)
{
	/* This event handler is not in use here. */
	ARG_UNUSED(evt);
}

static void on_modem_lib_init(int ret, void *ctx)
{
	int err;
	static bool m_first_init = true;

	ARG_UNUSED(ctx);

	if (ret != 0) {
		return;
	}

	/* Let the application write the credentials first and then bring the link up. */
	if (m_first_init) {
		err = carrier_cert_provision();
		if (err) {
			printk("failed to provision CA certificates\n");
		}

#ifdef LWM2M_CARRIER_SETTINGS
		if (!lwm2m_settings_enable_custom_server_config_get()) {
			err = carrier_psk_provision();
			if (err) {
				printk("failed to provision PSK\n");
			}
		}
#else
		err = carrier_psk_provision();
		if (err) {
			printk("failed to provision PSK\n");
		}
#endif /* LWM2M_CARRIER_SETTINGS */

		lte_lc_register_handler(lte_event_handler);

		m_first_init = false;
	}
}

void print_err(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_error_t *err = evt->data.error;

	static const char *strerr[] = {
		[LWM2M_CARRIER_ERROR_NO_ERROR] =
			"No error",
		[LWM2M_CARRIER_ERROR_BOOTSTRAP] =
			"Bootstrap error",
		[LWM2M_CARRIER_ERROR_LTE_LINK_UP_FAIL] =
			"Failed to connect to the LTE network",
		[LWM2M_CARRIER_ERROR_LTE_LINK_DOWN_FAIL] =
			"Failed to disconnect from the LTE network",
		[LWM2M_CARRIER_ERROR_FOTA_FAIL] =
			"Modem firmware update failed",
		[LWM2M_CARRIER_ERROR_CONFIGURATION] =
			"Illegal object configuration detected",
		[LWM2M_CARRIER_ERROR_INIT] =
			"Initialization failure",
		[LWM2M_CARRIER_ERROR_RUN] =
			"Configuration failure",
	};

	__ASSERT(PART_OF_ARRAY(strerr[err->type]),
		 "Unhandled liblwm2m_carrier error");

	printk("%s, reason %d\n", strerr[err->type], err->value);
}

void print_deferred(const lwm2m_carrier_event_t *evt)
{
	const lwm2m_carrier_event_deferred_t *def = evt->data.deferred;

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
		[LWM2M_CARRIER_DEFERRED_SERVICE_UNAVAILABLE] =
			"Server in maintenance mode",
		[LWM2M_CARRIER_DEFERRED_SIM_MSISDN] =
			"Waiting for SIM MSISDN",
	};

	__ASSERT(PART_OF_ARRAY(strdef[def->reason]),
		 "Unhandled deferred reason");

	printk("Reason: %s, timeout: %d seconds\n", strdef[def->reason],
		def->timeout);
}

int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	int err = 0;

	switch (event->type) {
	case LWM2M_CARRIER_EVENT_LTE_LINK_UP:
		printk("LWM2M_CARRIER_EVENT_LTE_LINK_UP\n");
		err = lte_lc_connect_async(NULL);
		break;
	case LWM2M_CARRIER_EVENT_LTE_LINK_DOWN:
		printk("LWM2M_CARRIER_EVENT_LTE_LINK_DOWN\n");
		err = lte_lc_offline();
		break;
	case LWM2M_CARRIER_EVENT_LTE_POWER_OFF:
		printk("LWM2M_CARRIER_EVENT_LTE_POWER_OFF\n");
		err = lte_lc_power_off();
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		printk("LWM2M_CARRIER_EVENT_BOOTSTRAPPED\n");
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
	case LWM2M_CARRIER_EVENT_FOTA_SUCCESS:
		printk("LWM2M_CARRIER_EVENT_FOTA_SUCCESS\n");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		printk("LWM2M_CARRIER_EVENT_REBOOT\n");
		break;
	case LWM2M_CARRIER_EVENT_MODEM_INIT:
		printk("LWM2M_CARRIER_EVENT_MODEM_INIT\n");
		err = nrf_modem_lib_init();
		break;
	case LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN:
		printk("LWM2M_CARRIER_EVENT_MODEM_SHUTDOWN\n");
		err = nrf_modem_lib_shutdown();
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		printk("LWM2M_CARRIER_EVENT_ERROR\n");
		print_err(event);
		break;
	}

	return err;
}
#endif /* CONFIG_LWM2M_CARRIER */

int main(void)
{
	printk("LWM2M Carrier library sample.\n");

	int err = nrf_modem_lib_init();

	if (err < 0) {
		printk("Failed to initialize modem library\n");
	}

	/* Connect to the LTE network. */
	lte_lc_connect_async(NULL);

	return 0;
}
