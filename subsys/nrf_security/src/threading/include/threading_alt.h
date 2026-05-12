/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MBEDTLS_THREADING_ALT_H
#define MBEDTLS_THREADING_ALT_H

#include <zephyr/kernel.h>

/* This needs to be a k_mutex and not a pointer because Oberon uses this
 * directly in its oberon_ctr_drbg_context_t struct.
 */
typedef struct k_mutex mbedtls_platform_mutex_t;

/* Unused, but needs to be defined. */
typedef int mbedtls_platform_condition_variable_t;

#endif /* MBEDTLS_THREADING_ALT_H */
