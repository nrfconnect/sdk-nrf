/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mbedtls/threading.h>
#include <zephyr/kernel.h>

#if !defined(CONFIG_HW_CC3XX)

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

#else

#include <nrf_cc3xx_platform_mutex.h>

/* These live in hw_cc3xx.c because they need to be initialized during SYS_INIT */
extern mbedtls_threading_mutex_t mbedtls_threading_key_slot_mutex;
extern mbedtls_threading_mutex_t mbedtls_threading_psa_globaldata_mutex;
extern mbedtls_threading_mutex_t mbedtls_threading_psa_rngdata_mutex;
extern mbedtls_threading_mutex_t mbedtls_threading_heap_mutex;

/*
 * When CC3XX is enabled, global mutex objects are provided by
 * nrf_cc3xx_platform_mutex_zephyr.c. Keep init/free as no-op wrappers and
 * expose real function symbols for lock/unlock to avoid resolving to the
 * CC3XX archive's data symbols with the same names.
 */
void mbedtls_mutex_init(mbedtls_threading_mutex_t *mutex)
{
	if (platform_mutex_apis.mutex_init_fn == NULL) {
		return;
	}

	platform_mutex_apis.mutex_init_fn(&mutex->MBEDTLS_PRIVATE(mutex));
}

void mbedtls_mutex_free(mbedtls_threading_mutex_t *mutex)
{
	if (platform_mutex_apis.mutex_free_fn == NULL) {
		return;
	}

	platform_mutex_apis.mutex_free_fn(&mutex->MBEDTLS_PRIVATE(mutex));
}

int mbedtls_mutex_lock(mbedtls_threading_mutex_t *mutex)
{
	if (platform_mutex_apis.mutex_lock_fn == NULL) {
		return MBEDTLS_ERR_THREADING_MUTEX_ERROR;
	}

	return platform_mutex_apis.mutex_lock_fn(&mutex->MBEDTLS_PRIVATE(mutex));
}

int mbedtls_mutex_unlock(mbedtls_threading_mutex_t *mutex)
{
	if (platform_mutex_apis.mutex_unlock_fn == NULL) {
		return MBEDTLS_ERR_THREADING_MUTEX_ERROR;
	}

	return platform_mutex_apis.mutex_unlock_fn(&mutex->MBEDTLS_PRIVATE(mutex));
}

#endif
