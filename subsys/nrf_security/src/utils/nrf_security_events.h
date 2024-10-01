/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>

#if !defined(__NRF_TFM__)
#include <zephyr/kernel.h>
#endif

/**
 * @file nrf_security_events.h
 *
 * This event interface is inspired by Zephyr's event interface.
 * There exists two implementations here.
 * One is backed by Zephyr events APIs (k_event_*), and the other assumes
 * there are no threads and uses cortex-M33 __WFE instructions.
 */

#if defined(CONFIG_EVENTS) && !defined(__NRF_TFM__)
typedef struct k_event *nrf_security_event_t;

#define NRF_SECURITY_EVENT_DEFINE(event_name)                                                      \
	K_EVENT_DEFINE(k_##event_name);                                                            \
	nrf_security_event_t event_name = &k_##event_name;

#else
typedef volatile uint32_t *nrf_security_event_t;

#define NRF_SECURITY_EVENT_DEFINE(event_name)                                                      \
	uint32_t volatile local_##event_name;                                                      \
	nrf_security_event_t event_name = &local_##event_name;

#endif

/**
 * @brief Initialize an event.
 *
 * @param[in] event The event to initialize.
 */
void nrf_security_event_init(nrf_security_event_t event);

/**
 * @brief Wait for an event.
 *
 * @param[in] event The event to wait for.
 * @param[in] events_mask The mask of events to wait for.
 *
 * @return The events that were triggered.
 */
uint32_t nrf_security_event_wait(nrf_security_event_t event, uint32_t events_mask);

/**
 * @brief Clear events.
 *
 * @param[in] event The event to clear.
 * @param[in] events_mask The mask of events to clear.
 */
void nrf_security_event_clear(nrf_security_event_t event, uint32_t events_mask);

/**
 * @brief Set events.
 *
 * @param[in] event The event object to update.
 * @param[in] events_mask The mask of events to set in the event object.
 *
 * @return The previous value of the event & events_mask.
 */
uint32_t nrf_security_event_set(nrf_security_event_t event, uint32_t events_mask);
