/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa.h"
#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_keygen.h"
#include "cracen_ml_dsa_packing.h"
#include "cracen_ml_dsa_poly.h"
#include "cracen_ml_dsa_rounding.h"
#include "cracen_ml_dsa_sampling.h"

#include <cracen_psa_xof.h>
#include <cracen_psa_primitives.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen/common.h>
#include <nrf_security_mem_helpers.h>

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <string.h>

psa_status_t cracen_ml_dsa_keygen_internal(const ml_dsa_params_t *alg_params, const uint8_t *seed,
					   uint8_t *k_secret, ml_dsa_poly_vector_t *s1_hat,
					   ml_dsa_poly_vector_t *s2_hat,
					   ml_dsa_poly_vector_t *t0_hat, uint8_t *pk)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	/* Concatenated key-expansion output: (rho(32) || rho'(64) || K(32)). */
	uint8_t h_out[ML_DSA_KEYGEN_H_OUT_SZ_BYTES];
	/** The following array is used as temporary for different purposes:
	 *  1. as keygen_seed = (xi || IntegerToBytes(k, 1) || IntegerToBytes(l, 1));
	 *  2. as rej_ntt_poly_seed = (rho || IntegerToBytes(s, 1) || IntegerToBytes(r, 1))
	 *     for ExpandA().
	 */
	uint8_t temp_seed[ML_DSA_REJ_NTT_SEED_BYTES];
	ml_dsa_poly_vector_t acc;
	ml_dsa_poly_vector_t t1;
	/* pkEncode cursor: pk = rho || SimpleBitPack(t1) per row. */
	uint8_t *packed_t1 = pk + ML_DSA_SEED_SZ_BYTES;

	/* (rho, rho', K) = H(xi || IntegerToBytes(k, 1) || IntegerToBytes(l, 1), 128) */
	memcpy(temp_seed, seed, ML_DSA_KEY_PAIR_SEED_SZ_BYTES);
	temp_seed[ML_DSA_KEY_PAIR_SEED_SZ_BYTES] = alg_params->rows_k;
	temp_seed[ML_DSA_KEY_PAIR_SEED_SZ_BYTES + 1] = alg_params->columns_l;

	status = cracen_ml_dsa_shake256_digest(temp_seed, sizeof(temp_seed), h_out, sizeof(h_out));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* pkEncode starts with rho; K is the signing secret. */
	memcpy(pk, h_out, ML_DSA_SEED_SZ_BYTES);
	memcpy(k_secret, h_out + ML_DSA_SEED_SZ_BYTES + ML_DSA_RHO_PRIME_SZ_BYTES,
	       ML_DSA_K_SZ_BYTES);

	/* ExpandS(rho'): returns s1, s2 in the normal domain (coefficients in [-eta, eta]). */
	status = cracen_ml_dsa_expand_s(alg_params, h_out + ML_DSA_SEED_SZ_BYTES, s1_hat, s2_hat);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	for (uint32_t col = 0; col < alg_params->columns_l; col++) {
		cracen_ml_dsa_ntt(&s1_hat[col]);
	}

	memcpy(temp_seed, pk, ML_DSA_SEED_SZ_BYTES); /* rho, for ExpandA */

	/** For each row: t = NTT^-1(A_row (.) s1_hat) + s2;
	 *                (t1, t0) = Power2Round(t).
	 */
	for (uint32_t row = 0; row < alg_params->rows_k; row++) {
		safe_memzero(&acc, sizeof(acc));

		for (uint32_t col = 0; col < alg_params->columns_l; col++) {
			/* FIPS 204, algorithm 32 (ExpandA) */
			temp_seed[ML_DSA_SEED_SZ_BYTES] = (uint8_t)col;
			temp_seed[ML_DSA_SEED_SZ_BYTES + 1] = (uint8_t)row;

			status = cracen_ml_dsa_rej_ntt_poly(temp_seed, &t1);
			if (status != PSA_SUCCESS) {
				goto exit;
			}

			/* for a single row: t1 = A_row * NTT(s1_hat) */
			cracen_ml_dsa_multiply_ntt(&t1, &t1, &s1_hat[col]);
			cracen_ml_dsa_add_ntt(&acc, &acc, &t1);
		}

		cracen_ml_dsa_ntt_inversed(&acc);

		/* s2_hat still holds s2 in the normal domain here, so acc becomes t. */
		for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
			acc.coeffs[i] =
				cracen_ml_dsa_reduce_to_zq(acc.coeffs[i] + s2_hat[row].coeffs[i]);
		}

		/** "t1" keeps "t1"; "t0_hat[row]" keeps "t0"
		 *  (both values are in the normal domain for now).
		 */
		cracen_ml_dsa_power2round(&acc, &t1, &t0_hat[row]);

		/** FIPS 204, algorithm 22 (pkEncode): "t1" for a single row packed as
		 *  a byte string appended to pk.
		 */
		cracen_ml_dsa_simple_bit_pack(&t1, ML_DSA_T1_COEFF_MAX, packed_t1);
		packed_t1 += ML_DSA_T1_PACKED_POLY_BYTES;
	}

	/* Return s2 and t0 in the NTT domain, ready for the signing loop
	 * (FIPS 204, Algorithm 7, lines 3-4).
	 */
	for (uint32_t row = 0; row < alg_params->rows_k; row++) {
		cracen_ml_dsa_ntt(&s2_hat[row]);
		cracen_ml_dsa_ntt(&t0_hat[row]);
	}

exit:
	safe_memzero(h_out, sizeof(h_out));
	safe_memzero(temp_seed, sizeof(temp_seed));
	safe_memzero(&acc, sizeof(acc));
	safe_memzero(&t1, sizeof(t1));
	return status;
}
