/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mbedtls/threading.h>
#include <zephyr/kernel.h>

#define MBEDTLS_MUTEX_DEFINE(name)                                                \
	mbedtls_threading_mutex_t name = {                                        \
		.MBEDTLS_PRIVATE(mutex) =                                         \
			Z_MUTEX_INITIALIZER(name.MBEDTLS_PRIVATE(mutex)),         \
	}

MBEDTLS_MUTEX_DEFINE(mbedtls_threading_key_slot_mutex);
MBEDTLS_MUTEX_DEFINE(mbedtls_threading_psa_globaldata_mutex);
MBEDTLS_MUTEX_DEFINE(mbedtls_threading_psa_rngdata_mutex);
MBEDTLS_MUTEX_DEFINE(mbedtls_threading_heap_mutex);

void mbedtls_mutex_init(mbedtls_threading_mutex_t *mutex)
{
	k_mutex_init(&mutex->MBEDTLS_PRIVATE(mutex));
}

void mbedtls_mutex_free(mbedtls_threading_mutex_t *mutex)
{
}

int mbedtls_mutex_lock(mbedtls_threading_mutex_t *mutex)
{
	return k_mutex_lock(&mutex->MBEDTLS_PRIVATE(mutex), K_FOREVER);
}

int mbedtls_mutex_unlock(mbedtls_threading_mutex_t *mutex)
{
	return k_mutex_unlock(&mutex->MBEDTLS_PRIVATE(mutex));
}
