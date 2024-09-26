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
#ifndef NRF_SECURITY_MUTEXES_H
#define NRF_SECURITY_MUTEXES_H

#include <stdint.h>

#if defined(CONFIG_MULTITHREADING) && !defined(__NRF_TFM__)

#include <zephyr/kernel.h>

typedef enum {
	NRF_SECURITY_MUTEX_FLAGS_NONE = 0,
	NRF_SECURITY_MUTEX_FLAGS_INITIALIZED = 1 << 0,
} NRF_SECURITY_MUTEX_FLAGS;

typedef struct { 
	uint32_t 	flags;
	struct k_mutex 	mutex; 
} mbedtls_threading_mutex_t;

#define NRF_SECURITY_MUTEX_DEFINE(mutex_name) mbedtls_threading_mutex_t mutex_name = {0}
#else
/* The uint32_t here is just a placeholder, this mutex type is not used */
typedef uint32_t mbedtls_threading_mutex_t;
#define NRF_SECURITY_MUTEX_DEFINE(mutex_name) mbedtls_threading_mutex_t mutex_name;

#endif

/**
 * @brief Initialize a mutex.
 *
 * @param[in] mutex The mutex to initialized.
 */
void nrf_security_mutex_init(mbedtls_threading_mutex_t * mutex);

/**
 * @brief Free a mutex.
 *
 * @param[in] mutex The mutex to free.
 */
void nrf_security_mutex_free(mbedtls_threading_mutex_t * mutex);

/**
 * @brief Lock a mutex.
 *
 * @param[in] mutex The mutex to lock.
 *
 * @return Returns 0 if the mutex was successfully locked; otherwise, it returns
 *         an error code from the k_mutex_lock() function.
 */
int nrf_security_mutex_lock(mbedtls_threading_mutex_t * mutex);

/**
 * @brief Unlock a mutex.
 *
 * @param[in] mutex The mutex to unlock.
 *
 * @return Returns 0 if the mutex was successfully unlocked; otherwise, it returns
 *         an error code from the k_mutex_unlock() function.
 */
int nrf_security_mutex_unlock(mbedtls_threading_mutex_t * mutex);

#endif /* NRF_SECURITY_MUTEXES_H */
