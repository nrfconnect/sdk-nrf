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
#include "cracen_ml_dsa_sign.h"

#include <cracen_psa_xof.h>
#include <cracen_psa_primitives.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen/common.h>
#include <nrf_security_mem_helpers.h>

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <string.h>

/* Upper bound on ML-DSA.Sign_internal rejection-loop iterations to avoid infinite loops. */
#define ML_DSA_SIGN_MAX_ATTEMPTS	1000

_Static_assert(ML_DSA_MATRIX_ROWS_MAX != 1 && ML_DSA_MATRIX_COLS_MAX != 1,
	       "To compile this file you need at least one ML-DSA key size (ML-DSA-44/65/87) "
	       "enabled in the driver using the PSA_WANT_* configs.");

/* Map a coefficient in [0, q) to its centered representative in (-q/2, q/2]. */
static int32_t to_signed(int32_t a)
{
	return a > (ML_DSA_PRIME_NUM - 1) / 2 ? a - ML_DSA_PRIME_NUM : a;
}

/* Returns true when every coefficient satisfies |coeff| < bound. Runs in constant time. */
static bool const_poly_vec_within_bound(const ml_dsa_poly_vector_t *vec, uint32_t count,
					int32_t bound)
{
	int32_t violation = 0;

	for (uint32_t j = 0; j < count; j++) {
		for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
			violation |= cracen_ml_dsa_ge_bound_mask(
					cracen_ml_dsa_abs_coeff(vec[j].coeffs[i]), bound);
		}
	}

	return violation == 0;
}

/* rho'' = H(K || rnd || mu, 64) (FIPS 204, Algorithm 7, line 7). */
static psa_status_t compute_priv_rand_seed(const uint8_t *k_secret, const uint8_t *rnd,
					   const uint8_t *mu, uint8_t *priv_rand_seed)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	cracen_xof_operation_t operation;

	status = cracen_xof_setup(&operation, PSA_ALG_SHAKE256);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = cracen_xof_update(&operation, k_secret, ML_DSA_K_SZ_BYTES);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_update(&operation, rnd, ML_DSA_RND_SZ_BYTES);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_update(&operation, mu, ML_DSA_MSG_RPZTV_SZ_BYTES);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_xof_output(&operation, priv_rand_seed, ML_DSA_PRIV_SEED_BYTES);

exit:
	(void)cracen_xof_abort(&operation);
	return status;
}

/** FIPS 204, Algorithm 6 (ML-DSA.KeyGen_internal), reduced to the signing secrets
 *  to avoid unnecessary skEncode() and skDecode() operations since the generated
 *  secret key sk is used by the signing operation only, so there is no need to keep
 *  it encoded.
 *
 *  Expands the 32-byte seed into rho (public matrix seed), K, tr and the NTT-domain
 *  secret vectors s1_hat, s2_hat, t0_hat. The shared expansion is done by
 *  cracen_ml_dsa_keygen_internal(); this wrapper additionally derives rho and the
 *  public-key hash tr = H(pk) (FIPS 204, Algorithm 6, line 8).
 */
static psa_status_t keygen_expand(const ml_dsa_params_t *alg_params, const uint8_t *seed,
				  uint8_t *rho,
				  uint8_t *k_secret,
				  uint8_t *tr,
				  ml_dsa_poly_vector_t *s1_hat,
				  ml_dsa_poly_vector_t *s2_hat,
				  ml_dsa_poly_vector_t *t0_hat)
{
	psa_status_t status;
	/* pk is public, so it does not need to be wiped. */
	uint8_t pk[ML_DSA_PK_SIZE_MAX_BYTES];

	status = cracen_ml_dsa_keygen_internal(alg_params, seed, k_secret, s1_hat, s2_hat, t0_hat,
					       pk);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* rho is the first 32 bytes of pkEncode; tr = H(pk). */
	memcpy(rho, pk, ML_DSA_SEED_SZ_BYTES);

	return cracen_ml_dsa_shake256_digest(pk, alg_params->pk_size, tr,
					     ML_DSA_PK_DIGEST_SZ_BYTES);
}

/* FIPS 204, Algorithm 26 (sigEncode): sigma = c_tilde || BitPack(z) || HintBitPack(h). */
static void sig_encode(const ml_dsa_params_t *alg_params, const uint8_t *c_tilde,
		       const ml_dsa_poly_vector_t *z, const uint8_t *hint, uint8_t *sig)
{
	size_t c_tilde_len = ML_DSA_SIGNERS_COMMITMENT_HASH_SZ(alg_params->lambda);
	uint32_t bitlen = cracen_ml_dsa_bit_length(2u * alg_params->gamma1 - 1u);
	size_t z_poly_bytes = cracen_ml_dsa_calc_vector_sz_bytes(bitlen);

	memcpy(sig, c_tilde, c_tilde_len);
	sig += c_tilde_len;

	for (uint32_t j = 0; j < alg_params->columns_l; j++) {
		cracen_ml_dsa_bit_pack(&z[j], alg_params->gamma1 - 1u, alg_params->gamma1, sig);
		sig += z_poly_bytes;
	}

	cracen_ml_dsa_hint_bit_pack(alg_params, hint, sig);
}

/** A single iteration of the ML-DSA.Sign_internal rejection-sampling loop
 *  (FIPS 204, Algorithm 7, lines 11-26).
 *
 *  Draws the masking vector y from the nonce @p counter, builds the commitment, derives
 *  the challenge, computes the signer's response z and the hint, and applies the validity
 *  checks. On acceptance the encoded signature is written to @p sig and @p signature_produced
 *  is set to true. On rejection @p signature_produced stays false and PSA_SUCCESS is returned,
 *  so the caller retries with the next nonce. All working buffers are local and wiped on return.
 *
 */
static psa_status_t sign_attempt(const ml_dsa_params_t *alg_params, const uint8_t *rho,
				 const ml_dsa_poly_vector_t *s1_hat,
				 const ml_dsa_poly_vector_t *s2_hat,
				 const ml_dsa_poly_vector_t *t0_hat,
				 const uint8_t *msg_representative, const uint8_t *priv_rand_seed,
				 uint16_t counter, uint8_t *sig, bool *signature_produced)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	/** mask_or_signers_resp serves two different purposes:
	 *  1. holds the masking vector;
	 *  2. the signer's response z once c*s1 is added.
	 */
	ml_dsa_poly_vector_t mask_or_signers_resp[ML_DSA_MATRIX_COLS_MAX];
	/* w */
	ml_dsa_poly_vector_t commitment[ML_DSA_MATRIX_ROWS_MAX];
	uint32_t bitlen = cracen_ml_dsa_bit_length(alg_params->w1_max);
	/* w1_bytes */
	size_t commitment_sz_bytes = cracen_ml_dsa_calc_vector_sz_bytes(bitlen);
	/* w1_encoded */
	uint8_t enc_commitment[ML_DSA_COMMITMENT_SIZE_MAX_BYTES];
	/* c_tilde */
	uint8_t commitment_hash[ML_DSA_C_TILDE_MAX_BYTES];
	size_t commitment_hash_len = ML_DSA_SIGNERS_COMMITMENT_HASH_SZ(alg_params->lambda);
	uint8_t hint[ML_DSA_MATRIX_ROWS_MAX * ML_DSA_POLY_COEFFS_COUNT];
	uint8_t rej_ntt_poly_seed[ML_DSA_REJ_NTT_SEED_BYTES];
	/* c */
	ml_dsa_poly_vector_t verifiers_challenge;
	ml_dsa_poly_vector_t tmp;
	ml_dsa_poly_vector_t tmp2;
	int32_t gamma2 = (int32_t)alg_params->gamma2;
	/* z_bound */
	int32_t signers_response_bound = (int32_t)(alg_params->gamma1 - alg_params->beta);
	int32_t r0_bound = (int32_t)(alg_params->gamma2 - alg_params->beta);
	int32_t reject_mask = 0;
	uint32_t hint_weight = 0;

	*signature_produced = false;

	memcpy(rej_ntt_poly_seed, rho, ML_DSA_SEED_SZ_BYTES);

	status = cracen_ml_dsa_expand_mask(alg_params, priv_rand_seed, counter,
					   mask_or_signers_resp);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/** w = NTT^-1(A (.) NTT(y));
	 *  note: calculating per column.
	 */
	safe_memzero(commitment, sizeof(commitment));
	for (uint32_t col = 0; col < alg_params->columns_l; col++) {
		tmp2 = mask_or_signers_resp[col];
		cracen_ml_dsa_ntt(&tmp2);

		for (uint32_t row = 0; row < alg_params->rows_k; row++) {
			/* FIPS 204, algorithm 32 (ExpandA) */
			rej_ntt_poly_seed[ML_DSA_SEED_SZ_BYTES] = (uint8_t)col;
			rej_ntt_poly_seed[ML_DSA_SEED_SZ_BYTES + 1] = (uint8_t)row;

			status = cracen_ml_dsa_rej_ntt_poly(rej_ntt_poly_seed, &tmp);
			if (status != PSA_SUCCESS) {
				goto exit;
			}

			/** for a single row:
			 *  tmp = A_row * NTT(mask_or_signers_resp)
			 */
			cracen_ml_dsa_multiply_ntt(&tmp, &tmp, &tmp2);
			cracen_ml_dsa_add_ntt(&commitment[row], &commitment[row], &tmp);
		}
	}

	for (uint32_t row = 0; row < alg_params->rows_k; row++) {
		cracen_ml_dsa_ntt_inversed(&commitment[row]);
	}

	/* Computing encoded signer's commitment (w1_encoded) */
	for (uint32_t row = 0; row < alg_params->rows_k; row++) {
		/* 1. w1 = HighBits(w), which is applied componentwise */
		for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
			tmp.coeffs[i] = cracen_ml_dsa_high_bits(commitment[row].coeffs[i], gamma2);
		}

		/** 2. FIPS 204, algorithm 28 (w1Encode(w1))
		 *     for a single row of a matrix A
		 */
		cracen_ml_dsa_simple_bit_pack(&tmp, alg_params->w1_max,
					      &enc_commitment[row * commitment_sz_bytes]);
	}

	/* Computing signer's commitment hash: c_tilde = H(mu || w1_encoded) */
	status = cracen_ml_dsa_compute_commitment_hash(msg_representative, enc_commitment,
						alg_params->rows_k * commitment_sz_bytes,
						commitment_hash, commitment_hash_len);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/** Computing verifier's challenge and transforming it to the NTT domain:
	 *  1. c = SampleInBall(c_tilde);
	 *  2. c_hat = NTT(c)
	 */
	status = cracen_ml_dsa_sample_in_ball(commitment_hash, commitment_hash_len,
					      alg_params->tau, &verifiers_challenge);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	cracen_ml_dsa_ntt(&verifiers_challenge);

	/** Computing signer's response:
	 *    Algorithm notation: z = y + c*s1
	 *    Variables: tmp = mask_or_signers_resp + verifiers_challenge * s1_hat
	 */
	for (uint32_t col = 0; col < alg_params->columns_l; col++) {
		cracen_ml_dsa_multiply_ntt(&tmp, &verifiers_challenge, &s1_hat[col]);
		cracen_ml_dsa_ntt_inversed(&tmp);
		/* Signer's response is kept in the centered (signed) representation */
		for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
			mask_or_signers_resp[col].coeffs[i] += to_signed(tmp.coeffs[i]);
		}
	}

	/** Note: Inspired by the implementation of mldsa-native, which also does not
	 *        perform validity checks for z (signer's response) and r0
	 *        at the same place (as FIPS 204, Algorithm 7, line 23 specifies).
	 */
	if (!const_poly_vec_within_bound(mask_or_signers_resp, alg_params->columns_l,
					 signers_response_bound)) {
		/* Rejected: retry required. */
		goto exit;
	}

	/** r0 = LowBits(w - c*s2);
	 *  h = MakeHint(-c*t0, w - c*s2 + c*t0).
	 */
	for (uint32_t row = 0; row < alg_params->rows_k; row++) {
		cracen_ml_dsa_multiply_ntt(&tmp, &verifiers_challenge, &s2_hat[row]);
		cracen_ml_dsa_ntt_inversed(&tmp);   /* tmp  = c*s2 */
		cracen_ml_dsa_multiply_ntt(&tmp2, &verifiers_challenge, &t0_hat[row]);
		cracen_ml_dsa_ntt_inversed(&tmp2);  /* tmp2 = c*t0 */

		for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
			int32_t w_minus_cs2 = commitment[row].coeffs[i] - tmp.coeffs[i];
			int32_t ct0 = tmp2.coeffs[i];
			int32_t r0 = cracen_ml_dsa_low_bits(w_minus_cs2, gamma2);
			int32_t hint_bit;

			reject_mask |= cracen_ml_dsa_ge_bound_mask(
					cracen_ml_dsa_abs_coeff(r0), r0_bound);
			reject_mask |= cracen_ml_dsa_ge_bound_mask(
					cracen_ml_dsa_abs_coeff(to_signed(ct0)), gamma2);

			hint_bit = cracen_ml_dsa_make_hint(-ct0, w_minus_cs2 + ct0, gamma2);
			hint[row * ML_DSA_POLY_COEFFS_COUNT + i] = (uint8_t)hint_bit;
			hint_weight += (uint32_t)hint_bit;
		}
	}

	/* Note: constant time check */
	if (reject_mask != 0 || hint_weight > alg_params->omega) {
		/* Rejected: retry required. */
		goto exit;
	}

	sig_encode(alg_params, commitment_hash, mask_or_signers_resp, hint, sig);
	*signature_produced = true;
	status = PSA_SUCCESS;

exit:
	safe_memzero(mask_or_signers_resp, sizeof(mask_or_signers_resp));
	safe_memzero(commitment, sizeof(commitment));
	safe_memzero(&verifiers_challenge, sizeof(verifiers_challenge));
	safe_memzero(&tmp, sizeof(tmp));
	safe_memzero(&tmp2, sizeof(tmp2));
	safe_memzero(hint, sizeof(hint));
	safe_memzero(enc_commitment, sizeof(enc_commitment));
	safe_memzero(commitment_hash, sizeof(commitment_hash));
	safe_memzero(rej_ntt_poly_seed, sizeof(rej_ntt_poly_seed));
	return status;
}

/* FIPS 204, Algorithm 7 (ML-DSA.Sign_internal) rejection sampling loop. */
static psa_status_t sign_loop(const ml_dsa_params_t *alg_params, const uint8_t *rho,
			      const ml_dsa_poly_vector_t *s1_hat,
			      const ml_dsa_poly_vector_t *s2_hat,
			      const ml_dsa_poly_vector_t *t0_hat, const uint8_t *msg_representative,
			      const uint8_t *priv_rand_seed, uint8_t *sig)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	bool signature_produced = false;
	uint16_t counter = 0;

	for (uint32_t attempt = 0; attempt < ML_DSA_SIGN_MAX_ATTEMPTS; attempt++) {
		status = sign_attempt(alg_params, rho, s1_hat, s2_hat, t0_hat, msg_representative,
				      priv_rand_seed, counter, sig, &signature_produced);
		if (status != PSA_SUCCESS) {
			return status;
		}

		if (signature_produced) {
			return PSA_SUCCESS;
		}
		counter += alg_params->columns_l;
	}

	/* Attempts of signature rejection loop exhausted. */
	safe_memzero(sig, alg_params->sig_size);
	return PSA_ERROR_GENERIC_ERROR;
}

/* FIPS 204, Algorithm 7 (ML-DSA.Sign_internal). The message representative is supplied
 * through the M' components (domain, ctx, oid, msg) so the same routine serves both pure
 * ML-DSA and HashML-DSA.
 */
psa_status_t cracen_ml_dsa_sign_internal(const ml_dsa_params_t *alg_params, const uint8_t *seed,
					 const uint8_t *rnd, uint8_t domain, const uint8_t *ctx,
					 size_t ctx_len, const uint8_t *oid, size_t oid_len,
					 const uint8_t *msg, size_t msg_len, uint8_t *sig)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	/** Note: "_hat" suffix for the variable name mean that the vector is in NTT domain,
	 *        i.e. s1_hat = NTT(s1).
	 *  Names of variables below correspond to FIPS 204 spec.
	 */
	ml_dsa_poly_vector_t s1_hat[ML_DSA_MATRIX_COLS_MAX];
	ml_dsa_poly_vector_t s2_hat[ML_DSA_MATRIX_ROWS_MAX];
	ml_dsa_poly_vector_t t0_hat[ML_DSA_MATRIX_ROWS_MAX];
	/* rho: public matrix seed */
	uint8_t public_matrix_seed[ML_DSA_SEED_SZ_BYTES];
	/* K: signing secret */
	uint8_t k_secret[ML_DSA_K_SZ_BYTES];
	/* tr: hash of the public key */
	uint8_t pk_digest[ML_DSA_PK_DIGEST_SZ_BYTES];
	/* mu: message representative */
	uint8_t msg_representative[ML_DSA_MSG_RPZTV_SZ_BYTES];
	/* rho'': private random seed */
	uint8_t private_random_seed[ML_DSA_PRIV_SEED_BYTES];

	status = keygen_expand(alg_params, seed, public_matrix_seed, k_secret, pk_digest,
			       s1_hat, s2_hat, t0_hat);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = cracen_ml_dsa_compute_msg_representative(pk_digest, domain,
							  ctx, ctx_len,
							  oid, oid_len,
							  msg, msg_len,
							  msg_representative);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = compute_priv_rand_seed(k_secret, rnd, msg_representative, private_random_seed);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = sign_loop(alg_params, public_matrix_seed,
			   s1_hat, s2_hat, t0_hat,
			   msg_representative, private_random_seed,
			   sig);

exit:
	safe_memzero(s1_hat, sizeof(s1_hat));
	safe_memzero(s2_hat, sizeof(s2_hat));
	safe_memzero(t0_hat, sizeof(t0_hat));
	safe_memzero(k_secret, sizeof(k_secret));
	safe_memzero(pk_digest, sizeof(pk_digest));
	safe_memzero(msg_representative, sizeof(msg_representative));
	safe_memzero(private_random_seed, sizeof(private_random_seed));

	return status;
}
