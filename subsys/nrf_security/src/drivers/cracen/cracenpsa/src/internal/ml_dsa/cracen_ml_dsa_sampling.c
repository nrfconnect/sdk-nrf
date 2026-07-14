/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_packing.h"
#include "cracen_ml_dsa_sampling.h"

#include <cracen_psa_xof.h>
#include <cracen_psa_primitives.h>

#include <nrf_security_mem_helpers.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

/** Upper bound on the number of 3-byte candidates RejNTTPoly needs to fill all
 *  256 coefficients: with rejection probability below 2^-128
 *  (FIPS 204, Appendix C loop bound).
 */
#define ML_DSA_MAX_CANDIDATES_COUNT	298u
#define ML_DSA_CANDIDATE_SIZE_BYTES	3u

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

	uint8_t bytes[ML_DSA_CANDIDATE_SIZE_BYTES * ML_DSA_MAX_CANDIDATES_COUNT];
	size_t bytes_to_squeeze = sizeof(bytes);
	size_t pos = 0;
	uint32_t j = 0;

	status = cracen_xof_setup(&operation, PSA_ALG_SHAKE128);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_xof_update(&operation, seed, ML_DSA_REJ_NTT_SEED_BYTES);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_output(&operation, bytes, bytes_to_squeeze);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	while (j < ML_DSA_POLY_COEFFS_COUNT) {
		int32_t coeff;

		if (pos == bytes_to_squeeze) {
			/* Rejections exhausted the buffer: fetch just what is missing. */
			bytes_to_squeeze =
				ML_DSA_CANDIDATE_SIZE_BYTES * (ML_DSA_MAX_CANDIDATES_COUNT - j);

			status = cracen_xof_output(&operation, bytes, bytes_to_squeeze);
			if (status != PSA_SUCCESS) {
				goto exit;
			}
			pos = 0;
		}

		coeff = coeff_from_three_bytes(bytes[pos], bytes[pos + 1], bytes[pos + 2]);
		pos += 3;
		if (coeff >= 0) {
			out->coeffs[j] = coeff;
			j++;
		}
	}

exit:
	safe_memzero(bytes, sizeof(bytes));
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

/** FIPS 204, Algorithm 15 (CoeffFromHalfByte).
 *  Maps a half-byte b in [0, 15] to a coefficient in [-eta, eta],
 *  or returns the INT32_MIN value on rejection.
 */
static int32_t coeff_from_half_byte(uint8_t b, uint8_t eta)
{
	if (eta == 2 && b < 15) {
		return 2 - (int32_t)(b % 5);
	}

	if (eta == 4 && b < 9) {
		return 4 - (int32_t)b;
	}

	return INT32_MIN;
}

/** FIPS 204, Algorithm 31 (RejBoundedPoly).
 *  Samples one polynomial with coefficients in [-eta, eta]
 *  from a 66-byte seed (rho' || 2-byte nonce).
 */
static psa_status_t rej_bounded_poly(const uint8_t *seed, uint8_t eta, ml_dsa_poly_vector_t *out)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_xof_operation_t operation;
	uint32_t j = 0;

	status = cracen_xof_setup(&operation, PSA_ALG_SHAKE256);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_xof_update(&operation, seed, ML_DSA_REJ_BOUNDED_SEED_BYTES);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	while (j < ML_DSA_POLY_COEFFS_COUNT) {
		uint8_t z;
		int32_t z0;
		int32_t z1;

		status = cracen_xof_output(&operation, &z, 1);
		if (status != PSA_SUCCESS) {
			goto exit;
		}

		z0 = coeff_from_half_byte(z & 0x0Fu, eta);
		z1 = coeff_from_half_byte(z >> 4, eta);

		if (z0 != INT32_MIN) {
			out->coeffs[j++] = z0;
		}

		if (z1 != INT32_MIN && j < ML_DSA_POLY_COEFFS_COUNT) {
			out->coeffs[j++] = z1;
		}
	}

exit:
	cracen_xof_abort(&operation);
	return status;
}

psa_status_t cracen_ml_dsa_expand_s(const ml_dsa_params_t *alg_params, const uint8_t *rho_prime,
				    ml_dsa_poly_vector_t *s1, ml_dsa_poly_vector_t *s2)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	/* rho' || IntegerToBytes(nonce, 2) */
	uint8_t seed[ML_DSA_REJ_BOUNDED_SEED_BYTES];
	uint16_t nonce = 0;

	memcpy(seed, rho_prime, ML_DSA_RHO_PRIME_SZ_BYTES);

	for (uint32_t r = 0; r < alg_params->columns_l; r++) {
		sys_put_le16(nonce, &seed[ML_DSA_RHO_PRIME_SZ_BYTES]);
		status = rej_bounded_poly(seed, alg_params->eta, &s1[r]);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		nonce++;
	}

	for (uint32_t r = 0; r < alg_params->rows_k; r++) {
		sys_put_le16(nonce, &seed[ML_DSA_RHO_PRIME_SZ_BYTES]);
		status = rej_bounded_poly(seed, alg_params->eta, &s2[r]);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		nonce++;
	}

exit:
	safe_memzero(seed, sizeof(seed));
	return status;
}

psa_status_t cracen_ml_dsa_expand_mask(const ml_dsa_params_t *alg_params,
				       const uint8_t *rho, uint16_t mu,
				       ml_dsa_poly_vector_t *y)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t seed[ML_DSA_EXPAND_MASK_SEED_BYTES];
	uint8_t v[ML_DSA_EXPAND_MASK_MAX_BYTES];

	/** c = 1 + bitlen(gamma1 - 1);
	 *  each polynomial is packed with 32 * c bytes.
	 */
	size_t v_len = 32u * (1u + cracen_ml_dsa_bit_length(alg_params->gamma1 - 1u));

	/* rho || IntegerToBytes(mu + r, 2) */
	memcpy(seed, rho, ML_DSA_RHO_PRIME_SZ_BYTES);

	for (uint32_t r = 0; r < alg_params->columns_l; r++) {
		sys_put_le16((uint16_t)(mu + r), &seed[ML_DSA_RHO_PRIME_SZ_BYTES]);

		status = cracen_ml_dsa_shake256_digest(seed, sizeof(seed), v, v_len);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		cracen_ml_dsa_bit_unpack(v, alg_params->gamma1 - 1u, alg_params->gamma1, &y[r]);
	}

exit:
	safe_memzero(seed, sizeof(seed));
	safe_memzero(v, sizeof(v));
	return status;
}
