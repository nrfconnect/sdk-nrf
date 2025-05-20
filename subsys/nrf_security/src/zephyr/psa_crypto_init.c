/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#if defined(CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT) && !defined(CONFIG_MBEDTLS_BUILTIN)

#include "psa/crypto.h"

/* Ensuring psa_crypto_init is run before possible PSA crypto API calls */
static int _psa_crypto_init(void)
{
	psa_status_t err = psa_crypto_init();

	if (err != PSA_SUCCESS) {
		k_panic();
		return -EIO;
	}

	return 0;
}

/* Ensure PSA crypto is initialized after nrf_cc3xx mutex initialization */
SYS_INIT(_psa_crypto_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT */
