/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdbool.h>

#include <errno.h>
#include <stdbool.h>
#include "nrf_security_mutexes.h"

#if !defined(__NRF_TFM__)
#include <zephyr/kernel.h>
#endif

#if defined(CONFIG_MULTITHREADING) && !defined(__NRF_TFM__)

void nrf_security_mutex_init(mbedtls_threading_mutex_t * mutex)
{
	if((mutex->flags & NRF_SECURITY_MUTEX_FLAGS_INITIALIZED) != 0) {
		k_mutex_init(&mutex->mutex);
	}
}

void nrf_security_mutex_free(mbedtls_threading_mutex_t * mutex)
{
	(void)mutex;
}

int nrf_security_mutex_lock(mbedtls_threading_mutex_t * mutex)
{
	if((mutex->flags & NRF_SECURITY_MUTEX_FLAGS_INITIALIZED) != 0) {
		return k_mutex_lock(&mutex->mutex, K_FOREVER);
	} else {
		return -EINVAL;
	}
}

int nrf_security_mutex_unlock(mbedtls_threading_mutex_t * mutex)
{
	if((mutex->flags & NRF_SECURITY_MUTEX_FLAGS_INITIALIZED) != 0) {
		return k_mutex_unlock(&mutex->mutex);
	} else {
		return -EINVAL;
	}
}

#else

void nrf_security_mutex_init(mbedtls_threading_mutex_t * mutex)
{
	(void)mutex;
}

void nrf_security_mutex_free(mbedtls_threading_mutex_t * mutex)
{
	(void)mutex;
}

int nrf_security_mutex_lock(mbedtls_threading_mutex_t * mutex)
{
	(void)mutex;
	return 0;
}

int nrf_security_mutex_unlock(mbedtls_threading_mutex_t * mutex)
{
	(void)mutex;
	return 0;
}
#endif
