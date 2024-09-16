/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdbool.h>
#include "nrf_security_events.h"

#if defined(__NRF_TFM__)
#include "cmsis.h"
#include "utilities.h"
#else
#include <zephyr/kernel.h>
#endif

#if defined(CONFIG_EVENTS) && !defined(__NRF_TFM__)

void nrf_security_event_init(nrf_security_event_t event)
{
	k_event_init(event);
}

uint32_t nrf_security_event_wait(nrf_security_event_t event, uint32_t events_mask)
{
	/* Don't clear the events before waiting and wait forever */
	return k_event_wait(event, events_mask, false, K_FOREVER);
}

void nrf_security_event_clear(nrf_security_event_t event, uint32_t events_mask)
{
	k_event_clear(event, events_mask);
}

uint32_t nrf_security_event_set(nrf_security_event_t event, uint32_t events_mask)
{
	return k_event_set(event, events_mask);
}

#else

void nrf_security_event_init(nrf_security_event_t event)
{
	*event = 0;
}

uint32_t nrf_security_event_wait(nrf_security_event_t event, uint32_t events_mask)
{
	/* Wait while we have none of the subscribed events */
	while ((*event & events_mask) == 0) {
		__WFE();
	}

	return *event & events_mask;
}

void nrf_security_event_clear(nrf_security_event_t event, uint32_t events_mask)
{
	*event &= ~events_mask;
}

uint32_t nrf_security_event_set(nrf_security_event_t event, uint32_t events_mask)
{
	uint32_t prev = *event & events_mask;

	*event |= events_mask;

	return prev;
}

#endif
