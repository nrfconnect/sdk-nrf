/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PSA_CRYPTO_DRIVER_CONFIG_H
#define PSA_CRYPTO_DRIVER_CONFIG_H

#if defined(MBEDTLS_PSA_CRYPTO_CONFIG_FILE)
#include MBEDTLS_PSA_CRYPTO_CONFIG_FILE
#else
#include "psa/crypto_config.h"
#endif

#endif /* PSA_CRYPTO_DRIVER_CONFIG_H */
