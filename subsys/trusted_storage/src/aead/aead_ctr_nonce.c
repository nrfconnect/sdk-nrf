/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "aead_nonce.h"

/*
 * Nonce is a 96 bits counter.
 */
static struct aead_ctr_nonce {
	uint64_t low;
	uint32_t high;
} __packed g_nonce;

static bool is_ctr_initialized;

static psa_status_t trusted_storage_nonce_init(void)
{
	psa_status_t status;

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* Initialize nonce to a random value. */
	status = psa_generate_random((void *)&g_nonce, sizeof(g_nonce));
	if (status != PSA_SUCCESS) {
		return status;
	}

	is_ctr_initialized = true;

	return status;
}

/* Return an incrementing nonce */
psa_status_t trusted_storage_get_nonce(uint8_t *nonce, size_t nonce_len)
{
	psa_status_t status;

	if (nonce == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (nonce_len != sizeof(g_nonce)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (nonce_len == 0) {
		return PSA_SUCCESS;
	}

	if (!is_ctr_initialized) {
		status = trusted_storage_nonce_init();

		if (status != PSA_SUCCESS) {
			return status;
		}
	}

	/* Incrementing is implemented by using a 64-bit and a 32-bit number */
	g_nonce.low++;

	/* On overflow */
	if (g_nonce.low == 0) {
		++g_nonce.high;
	}

	memcpy(nonce, &g_nonce, nonce_len);

	return PSA_SUCCESS;
}
