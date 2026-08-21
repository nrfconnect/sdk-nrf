/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing lock management function declarations for the nRF71 driver.
 */

#ifndef __LOCK_MGMT_H__
#define __LOCK_MGMT_H__

/**
 * @brief Allocate a mutex from the control memory pool.
 *
 * @return Pointer to the mutex on success, NULL on failure.
 */
void *nrf_wifi_lock_alloc(void);

/**
 * @brief Free a mutex allocated by @ref nrf_wifi_lock_alloc.
 *
 * @param lock Pointer to the mutex to free. No operation if NULL.
 */
void nrf_wifi_lock_free(void *lock);

/**
 * @brief Initialize a mutex.
 *
 * @param lock Pointer to the mutex to initialize.
 */
void nrf_wifi_lock_init(void *lock);

/**
 * @brief Acquire a mutex.
 *
 * @param lock Pointer to the mutex to acquire.
 */
void nrf_wifi_lock_take(void *lock);

/**
 * @brief Release a mutex.
 *
 * @param lock Pointer to the mutex to release.
 */
void nrf_wifi_lock_rel(void *lock);

/**
 * @brief Acquire a mutex from interrupt context.
 *
 * @param lock Pointer to the mutex to acquire.
 * @param flags Unused; retained for API compatibility.
 */
void nrf_wifi_lock_irq_take(void *lock, unsigned long *flags);

/**
 * @brief Release a mutex from interrupt context.
 *
 * @param lock Pointer to the mutex to release.
 * @param flags Unused; retained for API compatibility.
 */
void nrf_wifi_lock_irq_rel(void *lock, unsigned long *flags);

#endif /* __LOCK_MGMT_H__ */
