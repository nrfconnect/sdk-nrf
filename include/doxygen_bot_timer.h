/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file doxygen_bot_timer.h
 * @brief Timer module for Doxygen PR reviewer bot verification.
 *
 * @defgroup doxygen_bot_timer Doxygen bot timer test module
 * @{
 */

#ifndef DOXYGEN_BOT_TIMER_H
#define DOXYGEN_BOT_TIMER_H

#include <stdint.h>

typedef void (*doxy_timer_callback_t)(void);

/**
 * @brief Start a one-shot timer.
 *
 * @param timeout_ms Timeout duration in milliseconds.
 *
 * \return 0 on success, negative errno code on failure.
 */
int doxy_timer_start(uint32_t timeout_ms);

/**
 * @brief Stop the running timer.
 *
 * Cancels any pending timer callback
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_timer_stop(void);

/**
 * @brief Get remaining time on the active timer.
 *
 * @param [in] remaining_ms Pointer to store remaining milliseconds.
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_timer_get_remaining(uint32_t *remaining_ms);

/**
 * @brief Register a timer expiry callback.
 *
 * Sets the function that will be called when timer expires.
 *
 * @param callback Function to regiter as timer callback.
 *
 * @retval 0 on success.
 * @retval negative errno code on failure.
 */
int doxy_timer_set_callback(doxy_timer_callback_t callback);

/** @} */

#endif /* DOXYGEN_BOT_TIMER_H */
