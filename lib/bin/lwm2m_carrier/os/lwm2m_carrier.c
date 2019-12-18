/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <zephyr.h>
#include <lwm2m_carrier.h>

#ifdef CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_URI
#include <bootstrap_psk.h>
#endif

#define LWM2M_CARRIER_THREAD_STACK_SIZE 8192
#define LWLM2_CARRIER_THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

__weak void lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	ARG_UNUSED(event);
}

static lwm2m_carrier_config_t config;

void lwm2m_carrier_thread_run(void)
{
	int err;

#ifdef CONFIG_LWM2M_CARRIER_USE_CUSTOM_BOOTSTRAP_URI
	config.bootstrap_uri = CONFIG_LWM2M_CARRIER_CUSTOM_BOOTSTRAP_URI;
	config.psk = (char *)bootstrap_psk;
	config.psk_length = sizeof(bootstrap_psk);
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
		LWLM2_CARRIER_THREAD_PRIORITY, 0, K_NO_WAIT);
