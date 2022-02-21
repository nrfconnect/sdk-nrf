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
		err = lte_lc_init_and_connect_async(lte_event_handler);
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

void lwm2m_carrier_thread_run(void)
{
	int err;
	lwm2m_carrier_config_t config = {0};

#ifdef CONFIG_LWM2M_CARRIER_CERTIFICATION_MODE
	config.certification_mode = CONFIG_LWM2M_CARRIER_CERTIFICATION_MODE;
#endif

#ifndef CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD
	config.disable_bootstrap_from_smartcard = true;
#endif

#ifdef CONFIG_LWM2M_CARRIER_USE_CUSTOM_URI
	config.server_uri = CONFIG_LWM2M_CARRIER_CUSTOM_URI;

#ifdef CONFIG_LWM2M_CARRIER_IS_SERVER_BOOTSTRAP
	config.is_bootstrap_server = true;
#else
	config.server_lifetime = CONFIG_LWM2M_CARRIER_SERVER_LIFETIME;
#endif

#endif /* CONFIG_LWM2M_CARRIER_USE_CUSTOM_URI */

	config.psk = CONFIG_LWM2M_CARRIER_CUSTOM_PSK;
	config.apn = CONFIG_LWM2M_CARRIER_CUSTOM_APN;
	config.manufacturer = CONFIG_LWM2M_CARRIER_DEVICE_MANUFACTURER;
	config.model_number = CONFIG_LWM2M_CARRIER_DEVICE_MODEL_NUMBER;
	config.device_type = CONFIG_LWM2M_CARRIER_DEVICE_TYPE;
	config.hardware_version = CONFIG_LWM2M_CARRIER_DEVICE_HARDWARE_VERSION;
	config.software_version = CONFIG_LWM2M_CARRIER_DEVICE_SOFTWARE_VERSION;

#ifdef CONFIG_LWM2M_CARRIER_LG_UPLUS
	config.service_code = CONFIG_LWM2M_CARRIER_LG_UPLUS_SERVICE_CODE;
#endif

#ifdef CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT
	config.session_idle_timeout = CONFIG_LWM2M_CARRIER_SESSION_IDLE_TIMEOUT;
#endif

	/* Initialize the LwM2M carrier library.
	 *
	 * Note: can also pass NULL to initialize with default settings
	 * (no certification mode or additional settings required by operator)
	 */
	err = lwm2m_carrier_init(&config);

	if (err != 0) {
		printk("Failed to initialize the LwM2M carrier library. Error %d\n", err);
		return;
	}

	/* Will exit only on non-recoverable errors. */
	lwm2m_carrier_run();
}

K_THREAD_DEFINE(lwm2m_carrier_thread, LWM2M_CARRIER_THREAD_STACK_SIZE,
		lwm2m_carrier_thread_run, NULL, NULL, NULL,
		LWM2M_CARRIER_THREAD_PRIORITY, 0, 0);
