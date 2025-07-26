/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mpsl_clock_ctrl_cb.h
 *
 * @defgroup mpsl_clock_ctrl_cb MPSL Clock Control Notification Callback API
 *
 * @brief Callback API for MPSL clock control event notifications.
 * @{
 */

#ifndef MPSL_CLOCK_CTRL_CB_H__
#define MPSL_CLOCK_CTRL_CB_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MPSL_CLOCK_CTRL_CALLBACK)

typedef enum {
	MPSL_CLOCK_CTRL_EVENT_HFCLK_REQUESTED,
	MPSL_CLOCK_CTRL_EVENT_HFCLK_RELEASED,
} mpsl_clock_ctrl_event_t;

/**
 * @brief Callback type for MPSL clock control events.
 *
 * @param event The clock control event.
 */
typedef void (*mpsl_clock_ctrl_cb_t)(mpsl_clock_ctrl_event_t event);

/**
 * @brief Register a callback for clock control events.
 *
 * @note Only one callback can be registered at a time.
 * @note The callback will be invoked from a high priority context within MPSL,
 * so it must not block or call non-ISR-safe APIs.
 *
 * @param cb Callback function to register or @c NULL to unregister.
 * @return @c 0 on success, @c -EBUSY if already registered.
 */
int mpsl_clock_ctrl_cb_register(mpsl_clock_ctrl_cb_t cb);

#endif /* CONFIG_MPSL_CLOCK_CTRL_CALLBACK */

#ifdef __cplusplus
}
#endif

#endif /* MPSL_CLOCK_CTRL_CB_H__ */

/**@} */
