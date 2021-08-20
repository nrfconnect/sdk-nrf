/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr.h>
#include <lwm2m_carrier.h>

#define LWM2M_CARRIER_THREAD_STACK_SIZE 4096
#define LWLM2_CARRIER_THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

__weak int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	ARG_UNUSED(event);

	return 0;
}

static lwm2m_carrier_config_t config;

void lwm2m_carrier_thread_run(void)
{
	int err;

#ifdef CONFIG_LWM2M_CARRIER_CERTIFICATION_MODE
	config.certification_mode = CONFIG_LWM2M_CARRIER_CERTIFICATION_MODE;
#endif
#ifdef CONFIG_LWM2M_CARRIER_USE_CUSTOM_URI
	config.server_uri = CONFIG_LWM2M_CARRIER_CUSTOM_URI;
#ifdef CONFIG_LWM2M_CARRIER_IS_SERVER_BOOTSTRAP
	config.is_bootstrap_server = true;
#else
	config.server_lifetime = CONFIG_LWM2M_CARRIER_SERVER_LIFETIME;
#endif
#endif /* CONFIG_LWM2M_CARRIER_USE_CUSTOM_URI */
#ifdef CONFIG_LWM2M_CARRIER_USE_CUSTOM_PSK
	config.psk = CONFIG_LWM2M_CARRIER_CUSTOM_PSK;
#endif

#ifdef CONFIG_LWM2M_CARRIER_USE_CUSTOM_APN
	config.apn = CONFIG_LWM2M_CARRIER_CUSTOM_APN;
#endif

#ifndef CONFIG_LWM2M_CARRIER_BOOTSTRAP_SMARTCARD
	config.disable_bootstrap_from_smartcard = true;
#endif

	err = lwm2m_carrier_init(&config);

	if (err != 0) {
		printk("Failed to initialize the LwM2M carrier library. Error %d\n", err);
		return;
	}

	lwm2m_carrier_run();
}

K_THREAD_DEFINE(lwm2m_carrier_thread, LWM2M_CARRIER_THREAD_STACK_SIZE,
		lwm2m_carrier_thread_run, NULL, NULL, NULL,
		LWLM2_CARRIER_THREAD_PRIORITY, 0, 0);
