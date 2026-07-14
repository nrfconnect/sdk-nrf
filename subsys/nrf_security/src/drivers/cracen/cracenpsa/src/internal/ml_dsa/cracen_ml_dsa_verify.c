/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa.h"
#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_packing.h"
#include "cracen_ml_dsa_poly.h"
#include "cracen_ml_dsa_rounding.h"
#include "cracen_ml_dsa_sampling.h"
#include "cracen_ml_dsa_verify.h"

#include <cracen_psa_xof.h>
#include <cracen_psa_primitives.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen/common.h>
#include <nrf_security_mem_helpers.h>

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <string.h>

_Static_assert(ML_DSA_MATRIX_ROWS_MAX != 1 && ML_DSA_MATRIX_COLS_MAX != 1,
	       "To compile this file you need at least one ML-DSA key size (ML-DSA-44/65/87) "
	       "enabled in the driver using the PSA_WANT_* configs.");

/** FIPS 204, Algorithm 23 (pkDecode): split public key pk into seed (rho)
 *  and the pk_polynomial (t1) coefficients vector.
 */
static void pk_decode(const ml_dsa_params_t *alg_params, const uint8_t *pk, uint8_t *seed,
		      ml_dsa_poly_vector_t *pk_polynomial)
{
	const uint8_t *z;

	memcpy(seed, pk, ML_DSA_SEED_SZ_BYTES);
	z = pk + ML_DSA_SEED_SZ_BYTES;

	for (uint32_t i = 0; i < alg_params->rows_k; i++) {
		cracen_ml_dsa_simple_bit_unpack(&z[i * ML_DSA_T1_PACKED_POLY_BYTES],
						ML_DSA_T1_COEFF_MAX,
						&pk_polynomial[i]);
	}
}

/** FIPS 204, Algorithm 27 (sigDecode).
 *  Splits the signature into:
 *   - signer's commitment hash (c_tilde),
 *   - signer's response (z),
 *   - hint (h).
 *
 *  Signer's response (z) is returned in the centered representation, i.e.
 *  signed with the coefficients are in the range (-gamma1, gamma1].
 *
 *  Returns false if hint is malformed.
 */
static bool sig_decode(const ml_dsa_params_t *alg_params, const uint8_t *sig,
		       const uint8_t **signers_commitment_hash,
		       ml_dsa_poly_vector_t *signers_response, uint8_t *hint)
{
	size_t signers_response_sz;
	size_t poly_shift;
	const uint8_t *signers_response_packed;
	const uint8_t *hint_packed;
	uint32_t bitlen = cracen_ml_dsa_bit_length(2u * alg_params->gamma1 - 1u);

	/** Each z coefficient lies in range [-(gamma1-1), gamma1] and is packed with
	 *  BitPack(., gamma1-1, gamma1), needing
	 *  bitlen((gamma1-1)+gamma1) = bitlen(2*gamma1 - 1) bits per coefficient
	 *  (see FIPS 204, Alg. 17).
	 */
	signers_response_sz = cracen_ml_dsa_calc_vector_sz_bytes(bitlen);

	/* signature = signers_commitment_hash || signers_response_packed || hint_packed */
	*signers_commitment_hash = sig;
	signers_response_packed = sig + ML_DSA_SIGNERS_COMMITMENT_HASH_SZ(alg_params->lambda);
	hint_packed = signers_response_packed + (size_t)alg_params->columns_l * signers_response_sz;

	for (uint32_t j = 0; j < alg_params->columns_l; j++) {
		poly_shift = j * signers_response_sz;
		cracen_ml_dsa_bit_unpack(signers_response_packed + poly_shift,
					 alg_params->gamma1 - 1,
					 alg_params->gamma1,
					 &signers_response[j]);
	}

	return cracen_ml_dsa_hint_bit_unpack(alg_params, hint_packed, hint);
}

/** Checks if the infinity norm of the signer's response z to be in required range:
 *  ||z||_inf < (gamma1 - beta).
 *
 *  See FIPS 204, Algorithm 8.
 *
 *  Note: this function executes in constant time, which is inspired by mldsa-native.
 */
static bool check_signers_response(const ml_dsa_params_t *alg_params,
				   const ml_dsa_poly_vector_t *signers_response)
{
	int32_t bound = (int32_t)(alg_params->gamma1 - alg_params->beta);
	int32_t violation_mask = 0;

	for (uint32_t j = 0; j < alg_params->columns_l; j++) {
		for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
			int32_t coeff = signers_response[j].coeffs[i];
			int32_t magnitude = cracen_ml_dsa_abs_coeff(coeff);

			violation_mask |= cracen_ml_dsa_ge_bound_mask(magnitude, bound);
		}
	}

	return violation_mask == 0;
}

/* FIPS 204, Algorithm 8, line 8 combined with w1Encode (Algorithm 28):
 * for each row compute w'Approx = NTT^{-1}(A_i (.) z_hat - c_hat (.) NTT(t1_i * 2^d)),
 * apply UseHint, and SimpleBitPack the resulting w1 into w1_encoded.
 *
 * Signer's response (z) holds z_hat (already in the NTT domain);
 * PK polynomial (t1) is consumed (scaled and transformed in place).
 */
static psa_status_t compute_w1_encoded(const ml_dsa_params_t *alg_params, const uint8_t *seed,
				       const ml_dsa_poly_vector_t *signers_response,
				       ml_dsa_poly_vector_t *verifiers_challenge,
				       ml_dsa_poly_vector_t *pk_polynomial, const uint8_t *hint,
				       size_t w1_bytes, uint8_t *w1_encoded)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	ml_dsa_poly_vector_t acc;
	ml_dsa_poly_vector_t tmp;
	/* RejNTTPoly seed: (seed || s || r) */
	uint8_t rej_ntt_poly_seed[ML_DSA_REJ_NTT_SEED_BYTES];
	uint32_t hint_shift_per_row;

	memcpy(rej_ntt_poly_seed, seed, ML_DSA_SEED_SZ_BYTES);

	/* Note: NTT(c) is computed here, before the loop */
	cracen_ml_dsa_ntt(verifiers_challenge);

	/** Note: the following loop executes per row of the matrix A,
	 *  implementing multiple algorithms from FIPS 204.
	 */
	for (uint32_t row_r = 0; row_r < alg_params->rows_k; row_r++) {
		safe_memzero(&acc, sizeof(ml_dsa_poly_vector_t));

		/** Multiplying a single row of the matrix A by signer's response (z) vector:
		 *  A_i (.) z_hat, expanding A one cell at a time (FIPS 204, Alg 30/32).
		 */
		for (uint32_t column_s = 0; column_s < alg_params->columns_l; column_s++) {
			/* FIPS 204, algorithm 32 (ExpandA) */
			rej_ntt_poly_seed[ML_DSA_SEED_SZ_BYTES] = (uint8_t)column_s;
			rej_ntt_poly_seed[ML_DSA_SEED_SZ_BYTES + 1] = (uint8_t)row_r;

			status = cracen_ml_dsa_rej_ntt_poly(rej_ntt_poly_seed, &tmp);
			if (status != PSA_SUCCESS) {
				goto exit;
			}

			/** for a single row:
			 *  tmp = A * NTT(signers_response)
			 */
			cracen_ml_dsa_multiply_ntt(&tmp, &tmp, &signers_response[column_s]);
			cracen_ml_dsa_add_ntt(&acc, &acc, &tmp);
		}

		/** The public key polynomial received from signer (t1) contains
		 *  most significant bits, so moving them leaving dropped bits empty:
		 *  t1[row_r] = t1[row_r] * 2^d
		 */
		for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
			pk_polynomial[row_r].coeffs[i] <<= ML_DSA_DROPPED_BITS_COUNT;
		}

		/* NTT(c) * NTT(t1[row_r])*/
		cracen_ml_dsa_ntt(&pk_polynomial[row_r]);
		cracen_ml_dsa_multiply_ntt(&tmp, verifiers_challenge, &pk_polynomial[row_r]);

		/* Calculating acc = w'Approx: */
		cracen_ml_dsa_subtract_ntt(&acc, &acc, &tmp);
		cracen_ml_dsa_ntt_inversed(&acc);

		/* FIPS 204, algorithm 40 (UseHint) for a single row of a matrix A */
		/* w1 = UseHint(h_i, w'Approx_i); SimpleBitPack into w1_encoded. */
		hint_shift_per_row = row_r * ML_DSA_POLY_COEFFS_COUNT;
		for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
			tmp.coeffs[i] = cracen_ml_dsa_use_hint(hint[hint_shift_per_row + i],
							       acc.coeffs[i],
							       alg_params->gamma2);
		}

		/* FIPS 204, algorithm 28 (w1Encode) for a single row of a matrix A */
		cracen_ml_dsa_simple_bit_pack(&tmp, alg_params->w1_max,
					      &w1_encoded[(size_t)row_r * w1_bytes]);
	}

exit:
	safe_memzero(&acc, sizeof(ml_dsa_poly_vector_t));
	safe_memzero(&tmp, sizeof(ml_dsa_poly_vector_t));
	return status;
}

/* FIPS 204, Algorithm 8 (ML-DSA.Verify_internal). The message representative is
 * supplied through the M' components (domain, ctx, oid, msg) so the same routine
 * serves both pure ML-DSA and HashML-DSA.
 */
psa_status_t cracen_ml_dsa_verify_internal(const ml_dsa_params_t *alg_params, const uint8_t *pk,
					   const uint8_t *sig, uint8_t domain, const uint8_t *ctx,
					   size_t ctx_len, const uint8_t *oid, size_t oid_len,
					   const uint8_t *msg, size_t msg_len)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	/* t1 */
	ml_dsa_poly_vector_t pk_polynomial[ML_DSA_MATRIX_ROWS_MAX];
	/* z */
	ml_dsa_poly_vector_t signers_response[ML_DSA_MATRIX_COLS_MAX];
	/* c */
	ml_dsa_poly_vector_t verifiers_challenge;
	/* rho */
	uint8_t seed[ML_DSA_SEED_SZ_BYTES];
	/* h */
	uint8_t hint[ML_DSA_MATRIX_ROWS_MAX * ML_DSA_POLY_COEFFS_COUNT];
	/* tr: hash of the public key */
	uint8_t pk_digest[ML_DSA_PK_DIGEST_SZ_BYTES];
	/* mu: message representative */
	uint8_t msg_representative[ML_DSA_MSG_RPZTV_SZ_BYTES];
	/* c_tilde */
	const uint8_t *signers_commitment_hash;
	/* w1_encoded */
	uint8_t enc_commitment[ML_DSA_COMMITMENT_SIZE_MAX_BYTES];
	/* c_tilde_prime */
	uint8_t commitment_hash[ML_DSA_C_TILDE_MAX_BYTES];
	uint32_t bitlen = cracen_ml_dsa_bit_length(alg_params->w1_max);
	size_t commitment_sz_bytes = cracen_ml_dsa_calc_vector_sz_bytes(bitlen);
	int hash_cmp_res;

	pk_decode(alg_params, pk, seed, pk_polynomial);

	if (!sig_decode(alg_params, sig, &signers_commitment_hash, signers_response, hint)) {
		/* Note: early return according to spec. */
		status = PSA_ERROR_INVALID_SIGNATURE;
		goto exit;
	}

	if (!check_signers_response(alg_params, signers_response)) {
		/** Early return here to avoid using signer's response value which is out of bound.
		 *  Inspired by mldsa-native.
		 */
		status = PSA_ERROR_INVALID_SIGNATURE;
		goto exit;
	}

	status = cracen_ml_dsa_shake256_digest(pk, alg_params->pk_size,
					       pk_digest, ML_DSA_PK_DIGEST_SZ_BYTES);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_ml_dsa_compute_msg_representative(pk_digest, domain, ctx, ctx_len,
							  oid, oid_len, msg, msg_len,
							  msg_representative);
	safe_memzero(pk_digest, sizeof(pk_digest));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_ml_dsa_sample_in_ball(signers_commitment_hash,
					      ML_DSA_SIGNERS_COMMITMENT_HASH_SZ(alg_params->lambda),
					      alg_params->tau, &verifiers_challenge);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/** sig_decode() returns signer's response (z) in centered representation
	 *  (coefficients in (-gamma1, gamma1]). cracen_ml_dsa_ntt() accepts any signed
	 *  representation with |coefficient| < q, so z is transformed directly.
	 */
	for (uint32_t j = 0; j < alg_params->columns_l; j++) {
		cracen_ml_dsa_ntt(&signers_response[j]);
	}

	/** Computing encoded commitment (w1).
	 *  This combines steps 5, 9, 10 and (partially) 12 of the algorithm 8.
	 */
	status = compute_w1_encoded(alg_params, seed, signers_response, &verifiers_challenge,
				    pk_polynomial, hint, commitment_sz_bytes, enc_commitment);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_ml_dsa_compute_commitment_hash(msg_representative, enc_commitment,
					(size_t)alg_params->rows_k * commitment_sz_bytes,
					commitment_hash,
					ML_DSA_SIGNERS_COMMITMENT_HASH_SZ(alg_params->lambda));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* The ||z||_inf bound has already been verified above (check_signers_response()). */
	hash_cmp_res = constant_memcmp(signers_commitment_hash,
				       commitment_hash,
				       ML_DSA_SIGNERS_COMMITMENT_HASH_SZ(alg_params->lambda));

	status = hash_cmp_res == 0 ? PSA_SUCCESS : PSA_ERROR_INVALID_SIGNATURE;

exit:
	safe_memzero(&pk_polynomial, sizeof(pk_polynomial));
	safe_memzero(&signers_response, sizeof(signers_response));
	safe_memzero(&verifiers_challenge, sizeof(verifiers_challenge));
	safe_memzero(hint, sizeof(hint));
	safe_memzero(msg_representative, sizeof(msg_representative));
	safe_memzero(enc_commitment, sizeof(enc_commitment));
	safe_memzero(commitment_hash, sizeof(commitment_hash));

	return status;
}
