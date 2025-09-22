/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrfx_cracen.h>

#include "psa/crypto.h"

/* This file exists to provide TRNG to seed the PRNG in OBERON on devices that only support TRNG */

static psa_status_t cracen_trng_init(void)
{
	nrfx_err_t nrfx_error;

	/* This is TRNG even though the naming states otherwise.
	 * On devices that don't support hardware crypto it will default to trng
	 */
	nrfx_error = nrfx_cracen_ctr_drbg_init();
	if (nrfx_error != NRFX_SUCCESS) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_trng_get_entropy(uint32_t flags, size_t *estimate_bits,
				      uint8_t *output, size_t output_size)
{
	uint16_t request_len = MIN(UINT16_MAX, output_size);
	psa_status_t status;
	nrfx_err_t nrfx_error;

	/* Ignore flags as CRACEN TRNG doesn't support entropy generation flags */
	(void)flags;

	if (output == NULL || estimate_bits == NULL || output_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = cracen_trng_init();
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* This is TRNG even though the naming states otherwise.
	 * On devices that don't support hardware crypto it will default to trng
	 */
	nrfx_error = nrfx_cracen_ctr_drbg_random_get(output, request_len);
	if (nrfx_error != NRFX_SUCCESS) {
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	*estimate_bits = PSA_BYTES_TO_BITS(request_len);

	return PSA_SUCCESS;
}
