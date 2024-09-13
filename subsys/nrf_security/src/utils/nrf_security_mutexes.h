/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file nrf_security_mutexes.h
 *
 * This mutex interface is inspired by Zephyr's mutex interface.
 * There exists two implementations here.
 * One is backed by Zephyr events APIs (k_mutex_*), and other assumes
 * there are no threads thus it always returns success without doing anything.
 */

#include <stdint.h>

#if defined(CONFIG_MULTITHREADING) && !defined(__NRF_TFM__)
typedef struct k_mutex *nrf_security_mutex_t;
#define NRF_SECURITY_MUTEX_DEFINE(mutex_name)                                                      \
	K_MUTEX_DEFINE(k_##mutex_name);                                                            \
	nrf_security_mutex_t mutex_name = &k_##mutex_name;

#else
/* The uint8_t here is just a placeholder, this mutex type is not used */
typedef uint8_t nrf_security_mutex_t;
#define NRF_SECURITY_MUTEX_DEFINE(mutex_name) nrf_security_mutex_t mutex_name;

#endif

/**
 * @brief Initialize a mutex.
 *
 * @param[in] mutex The mutex to initialized.
 */
void nrf_security_mutex_init(nrf_security_mutex_t mutex);

/**
 * @brief Free a mutex.
 *
 * @param[in] mutex The mutex to free.
 */
void nrf_security_mutex_free(nrf_security_mutex_t mutex);

/**
 * @brief Lock a mutex.
 *
 * @param[in] mutex The mutex to lock.
 *
 * @return Returns 0 if the mutex was successfully locked; otherwise, it returns
 *         an error code from the k_mutex_lock() function.
 */
int nrf_security_mutex_lock(nrf_security_mutex_t mutex);

/**
 * @brief Unlock a mutex.
 *
 * @param[in] mutex The mutex to unlock.
 *
 * @return Returns 0 if the mutex was successfully unlocked; otherwise, it returns
 *         an error code from the k_mutex_unlock() function.
 */
int nrf_security_mutex_unlock(nrf_security_mutex_t mutex);
