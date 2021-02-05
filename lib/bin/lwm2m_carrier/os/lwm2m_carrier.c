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
#ifdef CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_URI
	config.bootstrap_uri = CONFIG_LWM2M_CARRIER_CUSTOM_BOOTSTRAP_URI;
#endif
#ifdef CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_PSK
	config.psk = CONFIG_LWM2M_CARRIER_CUSTOM_BOOTSTRAP_PSK;
#endif

	err = lwm2m_carrier_init(&config);
	__ASSERT(err == 0, "Failed to initialize LwM2M carrier library");

	if (err != 0) {
		return;
	}

	lwm2m_carrier_run();
	CODE_UNREACHABLE;
}

K_THREAD_DEFINE(lwm2m_carrier_thread, LWM2M_CARRIER_THREAD_STACK_SIZE,
		lwm2m_carrier_thread_run, NULL, NULL, NULL,
		LWLM2_CARRIER_THREAD_PRIORITY, 0, 0);
