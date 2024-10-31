/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/tls_credentials.h>
#include "psa_tls_functions.h"
#include "psa_tls_credentials.h"
#include "certificate.h"
#include "dummy_psk.h"
#include "psa/crypto.h"

LOG_MODULE_REGISTER(psa_tls_sample);


int main(void)
{
	int err;

	LOG_INF("PSA TLS app started");

#if defined(MBEDTLS_USE_PSA_CRYPTO)
	err = psa_crypto_init();
	if (err < 0) {
		return APP_ERROR;
	}
#endif

	err = tls_set_credentials();
	if (err < 0) {
		return APP_ERROR;
	}

	process_psa_tls();

	return APP_SUCCESS;
}
