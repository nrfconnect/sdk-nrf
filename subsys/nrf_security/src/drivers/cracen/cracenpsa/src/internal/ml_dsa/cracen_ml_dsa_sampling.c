/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_sampling.h"

#include <cracen_psa_xof.h>
#include <cracen_psa_primitives.h>

#include <nrf_security_mem_helpers.h>

/** FIPS 204, Algorithm 14 (CoeffFromThreeBytes). Returns the coefficient, or a
 *  negative value when the three bytes must be rejected.
 */
static int32_t coeff_from_three_bytes(uint8_t b0, uint8_t b1, uint8_t b2)
{
	uint32_t z = ((uint32_t)(b2 & 0x7Fu) << 16) | ((uint32_t)b1 << 8) | (uint32_t)b0;

	if (z < ML_DSA_PRIME_NUM) {
		return (int32_t)z;
	}

	return -1;
}

psa_status_t cracen_ml_dsa_rej_ntt_poly(const uint8_t *seed, ml_dsa_poly_vector_t *out)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_xof_operation_t operation;
	uint32_t j = 0;

	status = cracen_xof_setup(&operation, PSA_ALG_SHAKE128);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_xof_update(&operation, seed, ML_DSA_REJ_NTT_SEED_BYTES);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	while (j < ML_DSA_POLY_COEFFS_COUNT) {
		uint8_t bytes[3];
		int32_t coeff;

		status = cracen_xof_output(&operation, bytes, sizeof(bytes));
		if (status != PSA_SUCCESS) {
			goto exit;
		}

		coeff = coeff_from_three_bytes(bytes[0], bytes[1], bytes[2]);
		if (coeff >= 0) {
			out->coeffs[j] = coeff;
			j++;
		}
	}

exit:
	(void)cracen_xof_abort(&operation);
	return status;
}

psa_status_t cracen_ml_dsa_sample_in_ball(const uint8_t *seed, size_t seed_len,
					  uint8_t hamming_weight,
					  ml_dsa_poly_vector_t *out_vec)
{
	cracen_xof_operation_t operation;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint32_t start_index = ML_DSA_POLY_COEFFS_COUNT - hamming_weight;
	uint64_t signs; /* used as a bit string of size 64 */

	status = cracen_xof_setup(&operation, PSA_ALG_SHAKE256);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_xof_update(&operation, seed, seed_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_output(&operation, (uint8_t *)&signs, sizeof(signs));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	safe_memzero(out_vec, sizeof(ml_dsa_poly_vector_t));
	for (uint32_t i = start_index; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		uint8_t pos;

		do {
			status = cracen_xof_output(&operation, &pos, 1);
			if (status != PSA_SUCCESS) {
				goto exit;
			}
		} while (pos > i);

		out_vec->coeffs[i] = out_vec->coeffs[pos];
		if ((signs >> (i - start_index)) & 1u) {
			out_vec->coeffs[pos] = ML_DSA_PRIME_NUM - 1; /* -1 mod q */
		} else {
			out_vec->coeffs[pos] = 1;
		}
	}

exit:
	(void)cracen_xof_abort(&operation);
	signs = 0;
	return status;
}
