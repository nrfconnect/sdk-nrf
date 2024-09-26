
/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR Arm’s non-OSI source license
 */

#include "threading_alt.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "nrf_security_mutexes.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

NRF_SECURITY_MUTEX_DEFINE(mbedtls_threading_key_slot_mutex);
NRF_SECURITY_MUTEX_DEFINE(mbedtls_threading_psa_globaldata_mutex);
NRF_SECURITY_MUTEX_DEFINE(mbedtls_threading_psa_rngdata_mutex);

static void mbedtls_mutex_init_fn(mbedtls_threading_mutex_t *mutex)
{
    nrf_security_mutex_init(mutex);
}

static void mbedtls_mutex_free_fn(mbedtls_threading_mutex_t *mutex)
{
    nrf_security_mutex_free(mutex);
}

static int mbedtls_mutex_lock_fn(mbedtls_threading_mutex_t *mutex)
{
    return nrf_security_mutex_lock(mutex);
}

static int mbedtls_mutex_unlock_fn(mbedtls_threading_mutex_t *mutex)
{
    return nrf_security_mutex_unlock(mutex);
}

void (*mbedtls_mutex_init)(mbedtls_threading_mutex_t *mutex) = mbedtls_mutex_init_fn;
void (*mbedtls_mutex_free)(mbedtls_threading_mutex_t *mutex) = mbedtls_mutex_free_fn;
int (*mbedtls_mutex_lock)(mbedtls_threading_mutex_t *mutex) = mbedtls_mutex_lock_fn;
int (*mbedtls_mutex_unlock)(mbedtls_threading_mutex_t *mutex) = mbedtls_mutex_unlock_fn;
