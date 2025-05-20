/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NET_CORE_MONITOR_H_
#define NET_CORE_MONITOR_H_

/**
 * @file
 * @defgroup net_core_monitor Network Core Monitor API
 * @{
 * @brief API for the Network Core Monitor.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Network core monitor event types, used to signal the application. */
enum ncm_event_type {
	/** Event triggered when a network core reset occurs.
	 *  The ``reset_reas`` variable holds the reason for the reset.
	 *  It is rewritten from the RESET.RESETREAS register.
	 */
	NCM_EVT_NET_CORE_RESET,

	/** Event triggered when the network core is not responding. */
	NCM_EVT_NET_CORE_FREEZE
};

/** @brief Event handler that is called by the Network core monitor library when an event occurs.
 *
 * @note This function should be defined by the application.
 *       Otherwise, `__weak` definition will be called and it prints information about the event.
 *
 * @param[out] event       Event occurring.
 * @param[out] reset_reas  Reason for network core reset.
 *                         When the NCM_EVT_NET_CORE_RESET event was triggered the variable is set.
 *                         This value is rewritten from the network core's RESET.RESETREAS register.
 */
extern void ncm_net_core_event_handler(enum ncm_event_type event, uint32_t reset_reas);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NET_CORE_MONITOR_H_ */
