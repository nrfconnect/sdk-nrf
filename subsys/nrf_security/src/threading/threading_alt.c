/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "threading_alt.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>

K_MUTEX_DEFINE(mbedtls_threading_key_slot_mutex);
K_MUTEX_DEFINE(mbedtls_threading_psa_globaldata_mutex);
K_MUTEX_DEFINE(mbedtls_threading_psa_rngdata_mutex);

void mbedtls_mutex_init_fn(mbedtls_threading_mutex_t *mutex)
{
	k_mutex_init(mutex);
}

void mbedtls_mutex_free_fn(mbedtls_threading_mutex_t *mutex)
{
}

int mbedtls_mutex_lock_fn(mbedtls_threading_mutex_t *mutex)
{
	return k_mutex_lock(mutex, K_FOREVER);
}

int mbedtls_mutex_unlock_fn(mbedtls_threading_mutex_t *mutex)
{
	return k_mutex_unlock(mutex);
}

void (*mbedtls_mutex_init)(mbedtls_threading_mutex_t *mutex) = mbedtls_mutex_init_fn;
void (*mbedtls_mutex_free)(mbedtls_threading_mutex_t *mutex) = mbedtls_mutex_free_fn;
int (*mbedtls_mutex_lock)(mbedtls_threading_mutex_t *mutex) = mbedtls_mutex_lock_fn;
int (*mbedtls_mutex_unlock)(mbedtls_threading_mutex_t *mutex) = mbedtls_mutex_unlock_fn;
