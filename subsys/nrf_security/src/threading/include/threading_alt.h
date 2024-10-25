/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MBEDTLS_THREADING_ALT_H
#define MBEDTLS_THREADING_ALT_H

#include "mbedtls/build_info.h"
#include "nrf_security_mutexes.h"

/* Give access to the threading function-pointer prototypes (always used) */
extern void (*mbedtls_mutex_init)(mbedtls_threading_mutex_t *mutex);
extern void (*mbedtls_mutex_free)(mbedtls_threading_mutex_t *mutex);
extern int (*mbedtls_mutex_lock)(mbedtls_threading_mutex_t *mutex);
extern int (*mbedtls_mutex_unlock)(mbedtls_threading_mutex_t *mutex);

#endif /* MBEDTLS_THREADING_ALT_H */
