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
#include "cracen_ml_dsa_verify.h"

#include <cracen_psa_xof.h>
#include <cracen_psa_primitives.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen/common.h>
#include <nrf_security_mem_helpers.h>

#include <string.h>
#include <zephyr/sys/util.h>

/* Public-key derivation (FIPS 204, Algorithm 6 followed by pkEncode, Algorithm 22): expand
 * the seed and keep only the encoded public key; the secret vectors are wiped on return.
 */
static psa_status_t derive_pk(const ml_dsa_params_t *alg_params, const uint8_t *seed, uint8_t *pk)
{
	psa_status_t status;
	ml_dsa_poly_vector_t s1_hat[ML_DSA_MATRIX_COLS_MAX];
	ml_dsa_poly_vector_t s2_hat[ML_DSA_MATRIX_ROWS_MAX];
	ml_dsa_poly_vector_t t0_hat[ML_DSA_MATRIX_ROWS_MAX];
	uint8_t k_secret[ML_DSA_K_SZ_BYTES];

	status = cracen_ml_dsa_keygen_internal(alg_params, seed, k_secret, s1_hat, s2_hat, t0_hat,
					       pk);

	safe_memzero(s1_hat, sizeof(s1_hat));
	safe_memzero(s2_hat, sizeof(s2_hat));
	safe_memzero(t0_hat, sizeof(t0_hat));
	safe_memzero(k_secret, sizeof(k_secret));
	return status;
}

/** Implements FIPS 204, Algorithm 3 (ML-DSA.Verify) or Algorithm 5 (HashML-DSA.Verify),
 *  depending on the is_message parameter.
 */
psa_status_t cracen_ml_dsa_verify(bool is_message,
				  const psa_key_attributes_t *attributes,
				  const uint8_t *key_buffer, size_t key_buffer_size,
				  psa_algorithm_t alg, const uint8_t *input,
				  size_t input_length, const uint8_t *context,
				  size_t context_length, const uint8_t *signature,
				  size_t signature_length)
{
	const ml_dsa_params_t *alg_params;
	psa_key_type_t key_type = psa_get_key_type(attributes);
	const uint8_t *oid = NULL;
	size_t oid_len = 0;

	if (key_type != PSA_KEY_TYPE_ML_DSA_PUBLIC_KEY ||
	    (is_message && alg != PSA_ALG_ML_DSA && alg != PSA_ALG_DETERMINISTIC_ML_DSA) ||
	    (!is_message && !PSA_ALG_IS_HASH_ML_DSA(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if ((is_message && !CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_MESSAGE, attributes)) ||
	    (!is_message && !CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_VERIFY_HASH, attributes))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!is_message) {
		if (IS_ENABLED(PSA_NEED_CRACEN_HASH_ML_DSA) ||
		    IS_ENABLED(PSA_NEED_CRACEN_DETERMINISTIC_HASH_ML_DSA)) {
			oid = cracen_ml_dsa_hash_oid(alg);
		}

		if (oid == NULL) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		oid_len = ML_DSA_HASH_OID_BYTES;
	}

	alg_params = cracen_ml_dsa_params_get(psa_get_key_bits(attributes));
	if (alg_params == NULL) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (context_length > ML_DSA_MAX_CONTEXT_LENGTH ||
	    key_buffer_size  != alg_params->pk_size    ||
	    signature_length != alg_params->sig_size) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	return cracen_ml_dsa_verify_internal(alg_params, key_buffer, signature,
					     is_message ? 0 : 1,
					     context, context_length, oid, oid_len,
					     input, input_length);
}

/** Implements FIPS 204, Algorithm 2 (ML-DSA.Sign) or Algorithm 4 (HashML-DSA.Sign),
 *  depending on the is_message parameter. The private key is provided as the 32-byte
 *  key-pair seed and expanded on demand.
 */
psa_status_t cracen_ml_dsa_sign(bool is_message,
				const psa_key_attributes_t *attributes,
				const uint8_t *key_buffer, size_t key_buffer_size,
				psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, const uint8_t *context,
				size_t context_length, uint8_t *signature,
				size_t signature_size, size_t *signature_length)
{
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	const ml_dsa_params_t *alg_params;
	psa_key_type_t key_type = psa_get_key_type(attributes);
	const uint8_t *oid = NULL;
	size_t oid_len = 0;
	uint8_t rnd[ML_DSA_RND_SZ_BYTES];
	bool deterministic;

	if (key_type != PSA_KEY_TYPE_ML_DSA_KEY_PAIR ||
	    (is_message && alg != PSA_ALG_ML_DSA && alg != PSA_ALG_DETERMINISTIC_ML_DSA) ||
	    (!is_message && !PSA_ALG_IS_HASH_ML_DSA(alg))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if ((is_message && !CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_MESSAGE, attributes)) ||
	    (!is_message && !CRACEN_PSA_IS_KEY_FLAG(PSA_KEY_USAGE_SIGN_HASH, attributes))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (context_length > ML_DSA_MAX_CONTEXT_LENGTH ||
	    key_buffer_size != ML_DSA_KEY_PAIR_SEED_SZ_BYTES) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!is_message) {
		if (IS_ENABLED(PSA_NEED_CRACEN_HASH_ML_DSA) ||
		    IS_ENABLED(PSA_NEED_CRACEN_DETERMINISTIC_HASH_ML_DSA)) {
			oid = cracen_ml_dsa_hash_oid(alg);
		}

		if (oid == NULL) {
			return PSA_ERROR_NOT_SUPPORTED;
		}
		oid_len = ML_DSA_HASH_OID_BYTES;
	}

	alg_params = cracen_ml_dsa_params_get(psa_get_key_bits(attributes));
	if (alg_params == NULL) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (signature_size < alg_params->sig_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	deterministic = (is_message && alg == PSA_ALG_DETERMINISTIC_ML_DSA) ||
			(!is_message && PSA_ALG_IS_DETERMINISTIC_HASH_ML_DSA(alg));
	if (deterministic) {
		safe_memzero(rnd, sizeof(rnd));
	} else if (IS_ENABLED(CONFIG_PSA_NEED_CRACEN_CTR_DRBG_DRIVER)) {
		status = cracen_get_random(NULL, rnd, sizeof(rnd));
		if (status != PSA_SUCCESS) {
			return status;
		}
	} else {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	status = cracen_ml_dsa_sign_internal(alg_params, key_buffer, rnd,
					     is_message ? 0 : 1,
					     context, context_length, oid, oid_len,
					     input, input_length, signature);

	safe_memzero(rnd, sizeof(rnd));
	if (status == PSA_SUCCESS) {
		*signature_length = alg_params->sig_size;
	}

	return status;
}

psa_status_t cracen_ml_dsa_export_public_key_from_seed(const psa_key_attributes_t *attributes,
						       const uint8_t *key_buffer,
						       size_t key_buffer_size, uint8_t *data,
						       size_t data_size, size_t *data_length)
{
	const ml_dsa_params_t *alg_params = cracen_ml_dsa_params_get(psa_get_key_bits(attributes));
	psa_status_t status;

	if (alg_params == NULL) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (key_buffer_size != ML_DSA_KEY_PAIR_SEED_SZ_BYTES) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (data_size < alg_params->pk_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	status = derive_pk(alg_params, key_buffer, data);
	if (status == PSA_SUCCESS) {
		*data_length = alg_params->pk_size;
	}

	return status;
}
