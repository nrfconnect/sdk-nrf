/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <internal/ml_kem/cracen_ml_kem.h>
#include "cracen_ml_kem_internal.h"
#include "cracen_ml_kem_packing.h"
#include "cracen_ml_kem_poly.h"
#include "cracen_ml_kem_sampling.h"

#include <internal/pqc/cracen_pqc_xof.h>
#include <cracen/common.h>
#include <cracen/statuscodes.h>
#include <cracen_psa_ctr_drbg.h>
#include <nrf_security_mem_helpers.h>
#include <psa/crypto_extra.h>
#include <psa/crypto_values.h>
#include <string.h>
#include <zephyr/sys/util.h>

#define ML_KEM_DK_PKE_MAX_SZ_BYTES (ML_KEM_MATRIX_DIM_MAX * ML_KEM_POLY_PACKED_SZ_BYTES)

_Static_assert(ML_KEM_MATRIX_DIM_MAX != 1,
	       "To compile this file you need at least one ML-KEM key size "
	       "(ML-KEM-512/768/1024) enabled in the driver using the PSA_WANT_* configs.");

/** Returns its argument, but hides the value from the optimizer.
 *
 * Inspired by mlkem-native (mlk_value_barrier_u8).
 */
static inline uint8_t value_barrier_u8(uint8_t x)
{
	volatile uint8_t v = x;

	return v;
}

/* Returns the size in bytes of one compressed polynomial of the u vector. */
static size_t ciphertext_u_poly_size(const ml_kem_params_t *params)
{
	return (size_t)ML_KEM_POLY_COEFFS_COUNT * params->du / 8;
}

static psa_status_t hash_chunks(psa_algorithm_t alg,
				const uint8_t *chunks[], const size_t chunk_lengths[],
				size_t chunk_count, uint8_t *digest)
{
	const struct sxhashalg *sx_alg;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	status = cracen_hash_get_algo(alg, &sx_alg);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return silex_statuscodes_to_psa(
		cracen_hash_all_inputs(chunks, chunk_lengths, chunk_count, sx_alg, digest));
}

/** Combines two algorithms:
 *  1. SampleNTT(rho || j || i) to expand one cell of the A matrix at a time in NTT form (A_hat),
 *     see Algorithms 7 (SampleNTT) and lines 4...8 of Algorithm 14 (K-PKE.Encrypt) of FIPS203.
 *     Note:  A_hat is never held in memory.
 *
 *  2. Calculate output vector per cell as follows: vec_out = A_hat (.) vec_in.
 *     Note: The output vector is in NTT domain.
 */
static psa_status_t matrix_vector_multiply(const ml_kem_params_t *params, const uint8_t *rho,
					   bool transposed, const ml_kem_poly_vec_t vec_in,
					   ml_kem_poly_vec_t vec_out)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	ml_kem_poly_t tmp;

	for (uint8_t i = 0; i < params->k; i++) {
		safe_memzero(&vec_out[i], sizeof(ml_kem_poly_t));

		for (uint8_t j = 0; j < params->k; j++) {
			status = cracen_ml_kem_sample_ntt(rho,
							  transposed ? i : j,
							  transposed ? j : i,
							  &tmp);
			if (status != PSA_SUCCESS) {
				goto exit;
			}

			cracen_ml_kem_multiply_ntt(&tmp, &tmp, &vec_in[j]);
			cracen_ml_kem_add(&vec_out[i], &vec_out[i], &tmp);
			cracen_ml_kem_reduce(&vec_out[i]);
		}
	}

exit:
	safe_memzero(&tmp, sizeof(tmp));
	return status;
}

/**
 * @brief Uses randomness to generate an encryption key and a corresponding decryption key.
 *        FIPS 203, Algorithm 13 (K-PKE.KeyGen).
 *
 * @param[in] params Algorithm-specific parameter set.
 * @param[in] d Randomness, 32 bytes.
 * @param[out] ek Encapsulation key ek_pke.
 * @param[out] dk Decapsulation key dk_pke. May be NULL when only the encapsulation key is needed.
 */
static psa_status_t kpke_keygen(const ml_kem_params_t *params, const uint8_t *d, uint8_t *ek,
				uint8_t *dk)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	uint8_t g_output[ML_KEM_G_OUTPUT_SZ_BYTES];
	const uint8_t *rho;
	const uint8_t *sigma;
	uint8_t nonce = 0;

	ml_kem_poly_vec_t s_hat;
	ml_kem_poly_vec_t t_hat;
	ml_kem_poly_vec_t e;

	/* (rho, sigma) = G(d || k) */
	const uint8_t *g_inputs[]      = {d, &params->k};
	const size_t g_input_lengths[] = {ML_KEM_SEED_HALF_SZ_BYTES, sizeof(params->k)};

	status = hash_chunks(PSA_ALG_SHA3_512, g_inputs, g_input_lengths, ARRAY_SIZE(g_inputs),
			     g_output);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	rho = g_output;
	sigma = g_output + ML_KEM_SEED_HALF_SZ_BYTES;

	for (uint8_t i = 0; i < params->k; i++) {
		status = cracen_ml_kem_sample_poly_cbd(sigma, nonce++, params->eta1, &s_hat[i]);
		if (status != PSA_SUCCESS) {
			goto exit;
		}

		cracen_ml_kem_ntt(&s_hat[i]);
	}

	for (uint8_t i = 0; i < params->k; i++) {
		status = cracen_ml_kem_sample_poly_cbd(sigma, nonce++, params->eta1, &e[i]);
		if (status != PSA_SUCCESS) {
			goto exit;
		}

		cracen_ml_kem_ntt(&e[i]);
	}

	/** Line 18: t_hat = A_hat (.) s_hat */
	status = matrix_vector_multiply(params, rho, false, s_hat, t_hat);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	for (uint8_t i = 0; i < params->k; i++) {
		/** t_hat is in the NTT domain, but the R^(-1) factor left by the
		 *  products has to be cancelled here by a call to cracen_ml_kem_to_montgomery().
		 */
		cracen_ml_kem_to_montgomery(&t_hat[i]);
		cracen_ml_kem_add(&t_hat[i], &t_hat[i], &e[i]);
		cracen_ml_kem_reduce(&t_hat[i]);
	}

	/* ek_pke = ByteEncode_12(t_hat) || rho */
	for (uint8_t i = 0; i < params->k; i++) {
		cracen_ml_kem_poly_pack(&t_hat[i], &ek[(size_t)i * ML_KEM_POLY_PACKED_SZ_BYTES]);
	}
	memcpy(&ek[(size_t)params->k * ML_KEM_POLY_PACKED_SZ_BYTES], rho,
	       ML_KEM_SEED_HALF_SZ_BYTES);

	/* dk_pke = ByteEncode_12(s_hat) */
	if (dk != NULL) {
		for (uint8_t i = 0; i < params->k; i++) {
			cracen_ml_kem_poly_pack(&s_hat[i],
						&dk[(size_t)i * ML_KEM_POLY_PACKED_SZ_BYTES]);
		}
	}

exit:
	safe_memzero(s_hat, sizeof(s_hat));
	safe_memzero(t_hat, sizeof(t_hat));
	safe_memzero(e, sizeof(e));
	safe_memzero(g_output, sizeof(g_output));
	return status;
}

/** @brief FIPS 203, Algorithm 14 (K-PKE.Encrypt).
 *
 * @param[in] ek           Encapsulation key ek_pke, from which t_hat and seed rho are decoded.
 *                         Its modulus check is the caller's responsibility.
 * @param[out] ciphertext  params->ciphertext_size bytes.
 */
static psa_status_t kpke_encrypt(const ml_kem_params_t *params, const uint8_t *ek,
				 const uint8_t *msg, const uint8_t *randomness,
				 uint8_t *ciphertext)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t u_poly_size = ciphertext_u_poly_size(params);
	const uint8_t *seed_rho;
	uint8_t nonce_n;

	ml_kem_poly_vec_t t_hat = {}; /* Public key vector */
	ml_kem_poly_vec_t y_hat = {};
	ml_kem_poly_vec_t u = {};
	ml_kem_poly_t poly_v = {};
	ml_kem_poly_t tmp = {};

	/* t_hat = ByteDecode_12(ek_pke) */
	for (uint8_t i = 0; i < params->k; i++) {
		cracen_ml_kem_poly_unpack(&ek[(size_t)i * ML_KEM_POLY_PACKED_SZ_BYTES], &t_hat[i]);
	}

	seed_rho = ek + (size_t)params->k * ML_KEM_POLY_PACKED_SZ_BYTES;

	/* Calculate y and transfer it to NTT-domain. */
	nonce_n = 0;
	for (uint8_t i = 0; i < params->k; i++, nonce_n++) {
		status = cracen_ml_kem_sample_poly_cbd(randomness, nonce_n,
						       params->eta1, &y_hat[i]);
		if (status != PSA_SUCCESS) {
			goto exit;
		}

		cracen_ml_kem_ntt(&y_hat[i]);
	}

	/* Line 19: u = A_hat^T (.) y_hat */
	status = matrix_vector_multiply(params, seed_rho, true, y_hat, u);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* Line 19: u = NTT^(-1)(u) + e1 */
	for (uint8_t i = 0; i < params->k; i++, nonce_n++) {
		cracen_ml_kem_ntt_inversed(&u[i]);

		/* tmp contains a single poly of e1 sampled from CBD */
		status = cracen_ml_kem_sample_poly_cbd(randomness, nonce_n, params->eta2, &tmp);
		if (status != PSA_SUCCESS) {
			goto exit;
		}

		cracen_ml_kem_add(&u[i], &u[i], &tmp);
		/** The vector u will later be compressed (Compress_du(u)).
		 *  For that it must be reduced to have coefficients in range (-q, q).
		 */
		cracen_ml_kem_reduce(&u[i]);
	}

	/* Line 21: v = t_hat^T (.) y_hat */
	for (uint8_t i = 0; i < params->k; i++) {
		/** Note: t_hat is a vector and transposing it as t_hat^T (as psecified in FIPS203)
		 *  just changes its representation which do not require implementation changes
		 *  (as done above for transposed matrix A).
		 */
		cracen_ml_kem_multiply_ntt(&tmp, &t_hat[i], &y_hat[i]);
		cracen_ml_kem_add(&poly_v, &poly_v, &tmp);
		cracen_ml_kem_reduce(&poly_v);
	}
	/* Line 21: v = NTT^(-1)(v) */
	cracen_ml_kem_ntt_inversed(&poly_v);

	/** Lines 17 and 21: Computing e2 here and saving as tmp in order to decrease
	 *  the number of temporary variables.
	 *
	 *  v = v + e2
	 */
	status = cracen_ml_kem_sample_poly_cbd(randomness, nonce_n, params->eta2, &tmp);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	cracen_ml_kem_add(&poly_v, &poly_v, &tmp);

	/** Lines 20 and 21: v = v + mu.
	 *  mu = Decompress_1(ByteDecode_1(msg)).
	 */
	cracen_ml_kem_poly_decode_decompress(msg, 1, &tmp);
	cracen_ml_kem_add(&poly_v, &poly_v, &tmp);
	cracen_ml_kem_reduce(&poly_v);

	/* c = ByteEncode_du(Compress_du(u)) || ByteEncode_dv(Compress_dv(v)) */
	for (uint8_t i = 0; i < params->k; i++) {
		cracen_ml_kem_poly_compress_encode(&u[i], params->du, &ciphertext[i * u_poly_size]);
	}
	cracen_ml_kem_poly_compress_encode(&poly_v, params->dv,
					   &ciphertext[params->k * u_poly_size]);

exit:
	safe_memzero(t_hat, sizeof(t_hat));
	safe_memzero(y_hat, sizeof(y_hat));
	safe_memzero(u, sizeof(u));
	safe_memzero(&poly_v, sizeof(poly_v));
	safe_memzero(&tmp, sizeof(tmp));
	return status;
}

/**
 * @brief Uses the decryption key to decrypt a ciphertext.
 *        FIPS 203, Algorithm 15 (K-PKE.Decrypt).
 *
 * @param[in] params Algorithm-specific parameter set.
 * @param[in] dk Decapsulation key dk_pke, from which s_hat is decoded.
 * @param[in] ciphertext Ciphertext
 * @param[out] msg Decrypted message, 32 bytes.
 */
static void kpke_decrypt(const ml_kem_params_t *params, const uint8_t *dk,
			 const uint8_t *ciphertext, uint8_t *msg)
{
	size_t u_poly_size = ciphertext_u_poly_size(params);

	ml_kem_poly_vec_t s_hat = {};
	ml_kem_poly_vec_t u = {};
	ml_kem_poly_t poly_w = {};
	ml_kem_poly_t tmp = {};

	/* u' = Decompress_du(ByteDecode_du(c1)) */
	for (uint8_t i = 0; i < params->k; i++) {
		cracen_ml_kem_poly_decode_decompress(&ciphertext[i * u_poly_size],
						     params->du,
						     &u[i]);
		cracen_ml_kem_ntt(&u[i]);
	}

	/* Line 5: s_hat = ByteDecode_12(dk_pke) */
	for (uint8_t i = 0; i < params->k; i++) {
		cracen_ml_kem_poly_unpack(&dk[(size_t)i * ML_KEM_POLY_PACKED_SZ_BYTES], &s_hat[i]);
	}

	/*Line 4: w = NTT^(-1)(s_hat^T (.) NTT(u')) */
	for (uint8_t j = 0; j < params->k; j++) {
		cracen_ml_kem_multiply_ntt(&tmp, &s_hat[j], &u[j]);
		cracen_ml_kem_add(&poly_w, &poly_w, &tmp);
		cracen_ml_kem_reduce(&poly_w);
	}
	cracen_ml_kem_ntt_inversed(&poly_w);

	/* Line 4: v' = Decompress_du(ByteDecode_du(c2)) */
	cracen_ml_kem_poly_decode_decompress(&ciphertext[params->k * u_poly_size],
					     params->dv,
					     &tmp);
	/* Line 6: w = v' - w */
	cracen_ml_kem_subtract(&poly_w, &tmp, &poly_w);
	cracen_ml_kem_reduce(&poly_w);

	/* msg = ByteEncode_1(Compress_1(w)) */
	cracen_ml_kem_poly_compress_encode(&poly_w, 1, msg);

	safe_memzero(s_hat, sizeof(s_hat));
	safe_memzero(u, sizeof(u));
	safe_memzero(&poly_w, sizeof(poly_w));
	safe_memzero(&tmp, sizeof(tmp));
}

/** Encapsulation key modulus check required by FIPS 203, Section 7.2:
 *  ek is rejected unless ek == ByteEncode_12(ByteDecode_12(ek)).
 *
 *  This check ensures that the integers encoded in the public key
 *  are in the valid range [0, q−1].
 */
static bool check_encapsulation_key(const ml_kem_params_t *params, const uint8_t *ek)
{
	ml_kem_poly_t poly;
	uint8_t reencoded_poly_buf[ML_KEM_POLY_PACKED_SZ_BYTES];
	bool valid = true;

	for (uint8_t i = 0; i < params->k; i++) {
		size_t poly_offset = (size_t)i * ML_KEM_POLY_PACKED_SZ_BYTES;

		cracen_ml_kem_poly_unpack(&ek[poly_offset], &poly);
		cracen_ml_kem_reduce(&poly);
		cracen_ml_kem_poly_pack(&poly, reencoded_poly_buf);

		valid &= (constant_memcmp(&ek[poly_offset], reencoded_poly_buf,
					  sizeof(reencoded_poly_buf)) == 0);
	}

	return valid;
}

static psa_status_t validate_parameter_set(const psa_key_attributes_t *attributes,
					   psa_algorithm_t alg,
					   size_t key_length, size_t output_key_size,
					   const ml_kem_params_t *params)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);

	if (alg != PSA_ALG_ML_KEM || params == NULL || !PSA_KEY_TYPE_IS_ML_KEM(key_type)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if ((key_type == PSA_KEY_TYPE_ML_KEM_KEY_PAIR && key_length != ML_KEM_SEED_SZ_BYTES) ||
	    (key_type == PSA_KEY_TYPE_ML_KEM_PUBLIC_KEY && key_length != params->pk_size)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (output_key_size < ML_KEM_SHARED_SECRET_SZ_BYTES) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	return PSA_SUCCESS;
}

psa_status_t cracen_ml_kem_public_key_from_seed(size_t key_bits, const uint8_t *seed, uint8_t *ek,
					       size_t ek_size, size_t *ek_length)
{
	const ml_kem_params_t *params = cracen_ml_kem_params_get(key_bits);
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	if (params == NULL) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (ek_size < params->pk_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = kpke_keygen(params, seed, ek, NULL);
	if (status == PSA_SUCCESS) {
		*ek_length = params->pk_size;
	}

	return status;
}

psa_status_t cracen_ml_kem_encapsulate(const psa_key_attributes_t *attributes, const uint8_t *key,
				       size_t key_length, psa_algorithm_t alg,
				       const psa_key_attributes_t *output_attributes,
				       uint8_t *output_key, size_t output_key_size,
				       size_t *output_key_length, uint8_t *ciphertext,
				       size_t ciphertext_size, size_t *ciphertext_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	psa_key_type_t key_type = psa_get_key_type(attributes);
	const ml_kem_params_t *alg_params;
	/* ek */
	const uint8_t *encapsulation_key;
	uint8_t encapsulation_key_buf[ML_KEM_PK_MAX_SZ_BYTES];
	/* m */
	uint8_t randomness_m[ML_KEM_MSG_SZ_BYTES];
	uint8_t pk_digest[ML_KEM_PK_DIGEST_SZ_BYTES];
	/* K */
	const uint8_t *shared_secret_K;
	/* r */
	const uint8_t *randomness_r;
	uint8_t g_output[ML_KEM_G_OUTPUT_SZ_BYTES];

	(void)output_attributes;

	alg_params = cracen_ml_kem_params_get(psa_get_key_bits(attributes));
	status = validate_parameter_set(attributes, alg, key_length, output_key_size, alg_params);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (ciphertext_size < alg_params->ciphertext_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (key_type == PSA_KEY_TYPE_ML_KEM_KEY_PAIR) {
		/** Key pair is stored as the seed value (d || z),
		 * so the encapsulation key must be regenerated.
		 */
		status = kpke_keygen(alg_params, key, encapsulation_key_buf, NULL);
		if (status != PSA_SUCCESS) {
			goto exit;
		}
		encapsulation_key = encapsulation_key_buf;
	} else if (key_type == PSA_KEY_TYPE_ML_KEM_PUBLIC_KEY) {
		encapsulation_key = key;
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
		goto exit;
	}

	if (!check_encapsulation_key(alg_params, encapsulation_key)) {
		status = PSA_ERROR_INVALID_ARGUMENT;
		goto exit;
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_CTR_DRBG_DRIVER)) {
		/* FIPS 203, Algorithm 20 (ML-KEM.Encaps), step 1. */
		status = cracen_get_random(NULL, randomness_m, sizeof(randomness_m));
		if (status != PSA_SUCCESS) {
			goto exit;
		}
	} else {
		status = PSA_ERROR_NOT_SUPPORTED;
		goto exit;
	}

	/* FIPS 203, Algorithm 17 (ML-KEM.Encaps_internal) */
	/* Hashing encapulation key: pk_digest = H(ek) */
	const uint8_t *h_inputs[]      = {encapsulation_key};
	const size_t h_input_lengths[] = {alg_params->pk_size};

	status = hash_chunks(PSA_ALG_SHA3_256, h_inputs, h_input_lengths, ARRAY_SIZE(h_inputs),
			     pk_digest);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/** Derive shared secret key and randomness:
	 * (K, r) = G(m || pk_digest)
	 */
	const uint8_t *g_inputs[] =      {randomness_m,         pk_digest};
	const size_t g_input_lengths[] = {sizeof(randomness_m), sizeof(pk_digest)};

	status = hash_chunks(PSA_ALG_SHA3_512, g_inputs, g_input_lengths, ARRAY_SIZE(g_inputs),
			     g_output);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	shared_secret_K = g_output;
	randomness_r = g_output + ML_KEM_SEED_HALF_SZ_BYTES;

	/* c = K-PKE.Encrypt(ek, m, r) */
	status = kpke_encrypt(alg_params, encapsulation_key, randomness_m, randomness_r,
			      ciphertext);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	memcpy(output_key, shared_secret_K, ML_KEM_SHARED_SECRET_SZ_BYTES);
	*output_key_length = ML_KEM_SHARED_SECRET_SZ_BYTES;
	*ciphertext_length = alg_params->ciphertext_size;

exit:
	safe_memzero(randomness_m, sizeof(randomness_m));
	safe_memzero(g_output, sizeof(g_output));
	safe_memzero(encapsulation_key_buf, sizeof(encapsulation_key_buf));
	return status;
}

psa_status_t cracen_ml_kem_decapsulate(const psa_key_attributes_t *attributes, const uint8_t *key,
				       size_t key_length, psa_algorithm_t alg,
				       const uint8_t *ciphertext, size_t ciphertext_length,
				       const psa_key_attributes_t *output_attributes,
				       uint8_t *output_key, size_t output_key_size,
				       size_t *output_key_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	const ml_kem_params_t *alg_params;
	/* ek and dk_pke, both regenerated from the seed */
	uint8_t encapsulation_key_buf[ML_KEM_PK_MAX_SZ_BYTES];
	uint8_t decapsulation_key_buf[ML_KEM_DK_PKE_MAX_SZ_BYTES];
	/* c', the re-encryption of m' */
	uint8_t ciphertext_buf[ML_KEM_CIPHERTEXT_MAX_SZ_BYTES];
	/* m' */
	uint8_t message_m[ML_KEM_MSG_SZ_BYTES];
	uint8_t pk_digest[ML_KEM_PK_DIGEST_SZ_BYTES];
	/* K' and r' */
	const uint8_t *shared_secret_K;
	const uint8_t *randomness_r;
	uint8_t g_output[ML_KEM_G_OUTPUT_SZ_BYTES];
	/* K_bar, returned instead of K' when the ciphertext is rejected */
	uint8_t rejection_secret[ML_KEM_SHARED_SECRET_SZ_BYTES];
	/* z, the implicit rejection seed, is the second half of the key pair seed */
	const uint8_t *rejection_seed_z;
	uint8_t secret_cmp_res;
	uint8_t secret_sel_mask;

	(void)output_attributes;

	alg_params = cracen_ml_kem_params_get(psa_get_key_bits(attributes));
	status = validate_parameter_set(attributes, alg, key_length, output_key_size, alg_params);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (psa_get_key_type(attributes) != PSA_KEY_TYPE_ML_KEM_KEY_PAIR) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (ciphertext_length != alg_params->ciphertext_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	/** Key pair is stored as the seed value (d || z),
	 *  so the encapsulation key must be regenerated.
	 *
	 *  Since they key is generated and not provided as input and taking into account that
	 *  this function is used by the decapulating party, checking of the decapsulation key
	 *  need not be performed (FIPS 203, section 7.3).
	 */
	status = kpke_keygen(alg_params, key, encapsulation_key_buf, decapsulation_key_buf);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* FIPS 203, Algorithm 18 (ML-KEM.Decaps_internal). */
	/* m' = K-PKE.Decrypt(dk_pke, c) */
	kpke_decrypt(alg_params, decapsulation_key_buf, ciphertext, message_m);

	/* Line 3: extracting hash of PKE encryption key */
	const uint8_t *h_inputs[] = {encapsulation_key_buf};
	size_t h_input_lengths[]  = {alg_params->pk_size};

	status = hash_chunks(PSA_ALG_SHA3_256, h_inputs, h_input_lengths, ARRAY_SIZE(h_inputs),
			     pk_digest);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* Line 6: (K', r') = G(m' || h) */
	const uint8_t *g_inputs[] =	 {message_m,	     pk_digest};
	const size_t g_input_lengths[] = {sizeof(message_m), sizeof(pk_digest)};

	status = hash_chunks(PSA_ALG_SHA3_512, g_inputs, g_input_lengths, ARRAY_SIZE(g_inputs),
			     g_output);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	shared_secret_K = g_output;
	randomness_r = g_output + ML_KEM_SEED_HALF_SZ_BYTES;

	/* K_bar = J(z || c) */
	rejection_seed_z = key + ML_KEM_SEED_HALF_SZ_BYTES;

	const uint8_t *j_inputs[] = {rejection_seed_z,          ciphertext};
	size_t j_input_lengths[]  = {ML_KEM_SEED_HALF_SZ_BYTES, ciphertext_length};

	status = cracen_pqc_xof_compute(PSA_ALG_SHAKE256, j_inputs, j_input_lengths,
					ARRAY_SIZE(j_inputs), rejection_secret,
					sizeof(rejection_secret));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* c' = K-PKE.Encrypt(ek_pke, m', r') */
	status = kpke_encrypt(alg_params, encapsulation_key_buf, message_m, randomness_r,
			      ciphertext_buf);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/** Implicit rejection: a ciphertext that does not re-encrypt to itself yields
	 *  K_bar instead of K', without revealing which of the two was returned.
	 */
	secret_cmp_res = (uint8_t)constant_memcmp(ciphertext,
						  ciphertext_buf,
						  ciphertext_length);

	/* Spread any set bit of diff over the whole mask: for a non-zero diff either
	 * diff or its two's complement has the most significant bit set.
	 */
	secret_sel_mask = 0u - (uint32_t)((secret_cmp_res | (uint8_t)(0u - secret_cmp_res)) >> 7);
	secret_sel_mask = value_barrier_u8(secret_sel_mask);
	constant_mask_select_bin(secret_sel_mask, rejection_secret, shared_secret_K, output_key,
				 ML_KEM_SHARED_SECRET_SZ_BYTES);

	*output_key_length = ML_KEM_SHARED_SECRET_SZ_BYTES;

exit:
	safe_memzero(encapsulation_key_buf, sizeof(encapsulation_key_buf));
	safe_memzero(decapsulation_key_buf, sizeof(decapsulation_key_buf));
	safe_memzero(ciphertext_buf, sizeof(ciphertext_buf));
	safe_memzero(message_m, sizeof(message_m));
	safe_memzero(pk_digest, sizeof(pk_digest));
	safe_memzero(g_output, sizeof(g_output));
	safe_memzero(rejection_secret, sizeof(rejection_secret));
	return status;
}
