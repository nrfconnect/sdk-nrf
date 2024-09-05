
/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR Arm’s non-OSI source license
 */

#include "threading_alt.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_MULTITHREADING),
	"This file is intended for multi-threading, but single-threading is enabled. "
	"Please check your config build configuration!");

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

K_MUTEX_DEFINE(mbedtls_threading_key_slot_mutex);
K_MUTEX_DEFINE(mbedtls_threading_psa_globaldata_mutex);
K_MUTEX_DEFINE(mbedtls_threading_psa_rngdata_mutex);

#if defined(MBEDTLS_THREADING_ALT) && !(defined(CONFIG_CC3XX_BACKEND) || defined(CONFIG_PSA_CRYPTO_DRIVER_CC3XX))

void mbedtls_threading_set_alt( void (*mutex_init)( mbedtls_threading_mutex_t * ),
                       void (*mutex_free)( mbedtls_threading_mutex_t * ),
                       int (*mutex_lock)( mbedtls_threading_mutex_t * ),
                       int (*mutex_unlock)( mbedtls_threading_mutex_t * ) )
{
    // Do nothing
}

void mbedtls_threading_free_alt( void )
{
    // Do nothing
}

#endif /* MBEDTLS_THREADING_ALT */

#if defined(MBEDTLS_THREADING_C)

void mutex_init(mbedtls_threading_mutex_t *mutex)
{
    (void)k_mutex_init((struct k_mutex*)mutex);
}

void mutex_free(mbedtls_threading_mutex_t *mutex)
{
    // Do nothing
}

int mutex_lock(mbedtls_threading_mutex_t *mutex)
{
    return k_mutex_lock((struct k_mutex*)mutex, K_FOREVER);
}

int mutex_unlock(mbedtls_threading_mutex_t *mutex)
{
    return k_mutex_unlock((struct k_mutex*)mutex);
}

void (*mbedtls_mutex_init)(mbedtls_threading_mutex_t *mutex) = mutex_init;
void (*mbedtls_mutex_free)(mbedtls_threading_mutex_t *mutex) = mutex_free;
int (*mbedtls_mutex_lock)(mbedtls_threading_mutex_t *mutex) = mutex_lock;
int (*mbedtls_mutex_unlock)(mbedtls_threading_mutex_t *mutex) = mutex_unlock;

#endif /* MBEDTLS_THREADING_C */
