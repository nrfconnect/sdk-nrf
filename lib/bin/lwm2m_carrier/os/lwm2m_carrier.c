/*
 * Copyright (c) 2019-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <lwm2m_carrier.h>
#include <modem/lte_lc.h>

#define LWM2M_CARRIER_THREAD_STACK_SIZE 4096
#define LWM2M_CARRIER_THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

static void lte_event_handler(const struct lte_lc_evt *const evt)
{
	/* This event handler is not in use here. */
	ARG_UNUSED(evt);
}

__weak int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	int err;

	/* Minimal event handling required by the LwM2M carrier library.
	 * The application may replace this handler to have better control of the LTE link.
	 */
	switch (event->type) {
	case LWM2M_CARRIER_EVENT_INIT:
		err = lte_lc_init();
		lte_lc_register_handler(lte_event_handler);
		break;
	case LWM2M_CARRIER_EVENT_LTE_LINK_UP:
		err = lte_lc_connect_async(NULL);
		break;
	case LWM2M_CARRIER_EVENT_LTE_LINK_DOWN:
		err = lte_lc_offline();
		break;
	case LWM2M_CARRIER_EVENT_LTE_POWER_OFF:
		err = lte_lc_power_off();
		break;
	default:
		break;
	}

	return 0;
}

__weak int lwm2m_carrier_custom_init(lwm2m_carrier_config_t *config)
{
	/* This function is not in use here. */
	ARG_UNUSED(config);

	return 0;
}

void lwm2m_carrier_thread_run(void)
{
	int err;

	err = lwm2m_carrier_init();
	if (err != 0) {
		printk("Failed to initialize the LwM2M carrier library. Error %d\n", err);
		return;
	}

	lwm2m_carrier_config_t config = {0};

#ifdef CONFIG_LWM2M_CARRIER_GENERIC
	config.carriers_enabled |= LWM2M_CARRIER_GENERIC;
#endif

#ifdef CONFIG_LWM2M_CARRIER_VERIZON
	config.carriers_enabled |= LWM2M_CARRIER_VERIZON;
#endif

#ifdef CONFIG_LWM2M_CARRIER_ATT
	config.carriers_enabled |= LWM2M_CARRIER_ATT;
#endif

#ifdef CONFIG_LWM2M_CARRIER_LG_UPLUS
	config.carriers_enabled |= LWM2M_CARRIER_LG_UPLUS;
#endif

#ifdef CONFIG_LWM2M_CARRIER_T_MOBILE
	config.carriers_enabled |= LWM2M_CARRIER_T_MOBILE;
#endif

#ifndef CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD
	config.disable_bootstrap_from_smartcard = true;
#endif

	config.server_uri = CONFIG_LWM2M_CARRIER_CUSTOM_URI;

#ifdef CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER
	config.is_bootstrap_server = true;
#else
	config.server_lifetime = CONFIG_LWM2M_CARRIER_SERVER_LIFETIME;
#endif

	config.server_sec_tag = CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG;
	config.apn = CONFIG_LWM2M_CARRIER_CUSTOM_APN;
	config.pdn_type = CONFIG_LWM2M_CARRIER_PDN_TYPE;
	config.coap_con_interval = CONFIG_LWM2M_CARRIER_COAP_CON_INTERVAL;
	config.server_binding = CONFIG_LWM2M_SERVER_BINDING;
	config.manufacturer = CONFIG_LWM2M_CARRIER_DEVICE_MANUFACTURER;
	config.model_number = CONFIG_LWM2M_CARRIER_DEVICE_MODEL_NUMBER;
	config.device_type = CONFIG_LWM2M_CARRIER_DEVICE_TYPE;
	config.hardware_version = CONFIG_LWM2M_CARRIER_DEVICE_HARDWARE_VERSION;
	config.software_version = CONFIG_LWM2M_CARRIER_DEVICE_SOFTWARE_VERSION;

#ifdef CONFIG_LWM2M_CARRIER_LG_UPLUS
	config.lg_uplus.service_code = CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE;

	if (IS_ENABLED(CONFIG_LWM2M_CARRIER_LG_UPLUS_2DID)) {
		config.lg_uplus.device_serial_no_type =
			LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_2DID;
	} else {
		config.lg_uplus.device_serial_no_type =
			LWM2M_CARRIER_LG_UPLUS_DEVICE_SERIAL_NO_IMEI;
	}
#endif

#ifdef CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT
	config.session_idle_timeout = CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT;
#endif

	err = lwm2m_carrier_custom_init(&config);
	if (err != 0) {
		printk("Failed to initialize custom config settings. Error %d\n", err);
		return;
	}

	/* Run the LwM2M carrier library.
	 *
	 * Note: can also pass NULL to initialize with default settings
	 * (no certification mode or additional settings required by operator)
	 */
	err = lwm2m_carrier_run(&config);
	if (err != 0) {
		printk("Failed to run the LwM2M carrier library. Error %d\n", err);
	}
}

K_THREAD_DEFINE(lwm2m_carrier_thread, LWM2M_CARRIER_THREAD_STACK_SIZE,
		lwm2m_carrier_thread_run, NULL, NULL, NULL,
		LWM2M_CARRIER_THREAD_PRIORITY, 0, 0);
