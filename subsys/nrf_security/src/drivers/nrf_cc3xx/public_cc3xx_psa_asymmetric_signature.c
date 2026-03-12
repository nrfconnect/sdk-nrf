/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** \file cc3xx_psa_asymmetric_signature.c
 *
 * This file contains the implementation of the entry points associated to the
 * asymmetric signature capability as described by the PSA Cryptoprocessor
 * Driver interface specification
 *
 */

#include <psa/crypto.h>
#include <stdint.h>
#include <cc3xx_psa_asymmetric_signature.h>
#include <cc3xx_psa_hash.h>

#define HASH_SHA512_BLOCK_SIZE_IN_BYTES 128

/** \defgroup psa_asym_sign PSA driver entry points for asymmetric sign/verify
 *
 *  Entry points for asymmetric message signing and signature verification as
 *  described by the PSA Cryptoprocessor Driver interface specification
 *
 *  @{
 */
psa_status_t cc3xx_sign_hash(const psa_key_attributes_t *attributes, const uint8_t *key,
			     size_t key_length, psa_algorithm_t alg, const uint8_t *input,
			     size_t input_length, uint8_t *signature, size_t signature_size,
			     size_t *signature_length)
{
	if (alg != PSA_ALG_RSA_PKCS1V15_SIGN_RAW &&
	    input_length != PSA_HASH_LENGTH(PSA_ALG_SIGN_GET_HASH(alg))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!PSA_KEY_TYPE_IS_ASYMMETRIC(psa_get_key_type(attributes))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (PSA_ALG_IS_ECDSA(alg)) {
		return cc3xx_internal_ecdsa_sign(attributes, key, key_length, alg, input,
						 input_length, signature, signature_size,
						 signature_length, false);
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) || PSA_ALG_IS_RSA_PSS(alg)) {
		return cc3xx_internal_rsa_sign(attributes, key, key_length, alg, input,
					       input_length, signature, signature_size,
					       signature_length, false);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cc3xx_verify_hash(const psa_key_attributes_t *attributes, const uint8_t *key,
			       size_t key_length, psa_algorithm_t alg, const uint8_t *hash,
			       size_t hash_length, const uint8_t *signature,
			       size_t signature_length)
{
	if (alg != PSA_ALG_RSA_PKCS1V15_SIGN_RAW &&
	    hash_length != PSA_HASH_LENGTH(PSA_ALG_SIGN_GET_HASH(alg))) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!PSA_KEY_TYPE_IS_ASYMMETRIC(psa_get_key_type(attributes))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (PSA_ALG_IS_ECDSA(alg)) {
		return cc3xx_internal_ecdsa_verify(attributes, key, key_length, alg, hash,
						   hash_length, signature, signature_length, false);
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) || PSA_ALG_IS_RSA_PSS(alg)) {
		return cc3xx_internal_rsa_verify(attributes, key, key_length, alg, hash,
						 hash_length, signature, signature_length, false);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cc3xx_sign_message(const psa_key_attributes_t *attributes, const uint8_t *key,
				size_t key_length, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *signature, size_t signature_size,
				size_t *signature_length)
{
	psa_status_t ret;
	uint8_t hash[HASH_SHA512_BLOCK_SIZE_IN_BYTES];
	size_t hash_len;

	if (!PSA_KEY_TYPE_IS_ASYMMETRIC(psa_get_key_type(attributes))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (PSA_ALG_IS_DETERMINISTIC_ECDSA(alg)) {
		ret = cc3xx_hash_compute(PSA_ALG_SIGN_GET_HASH(alg), input, input_length, hash,
					 sizeof(hash), &hash_len);
		if (ret != PSA_SUCCESS) {
			return ret;
		}

		return cc3xx_internal_ecdsa_sign(attributes, key, key_length, alg, hash, hash_len,
						 signature, signature_size, signature_length,
						 false);
	} else if (PSA_ALG_IS_ECDSA(alg)) {
		return cc3xx_internal_ecdsa_sign(attributes, key, key_length, alg, input,
						 input_length, signature, signature_size,
						 signature_length, true);
	} else if (alg == PSA_ALG_PURE_EDDSA) {
		return cc3xx_internal_eddsa_sign(attributes, key, key_length, alg, input,
						 input_length, signature, signature_size,
						 signature_length);
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) || PSA_ALG_IS_RSA_PSS(alg)) {
		return cc3xx_internal_rsa_sign(attributes, key, key_length, alg, input,
					       input_length, signature, signature_size,
					       signature_length, true);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cc3xx_verify_message(const psa_key_attributes_t *attributes, const uint8_t *key,
				  size_t key_length, psa_algorithm_t alg, const uint8_t *input,
				  size_t input_length, const uint8_t *signature,
				  size_t signature_length)
{
	if (!PSA_KEY_TYPE_IS_ASYMMETRIC(psa_get_key_type(attributes))) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	if (PSA_ALG_IS_ECDSA(alg)) {
		return cc3xx_internal_ecdsa_verify(attributes, key, key_length, alg, input,
						   input_length, signature, signature_length, true);
	} else if (alg == PSA_ALG_PURE_EDDSA) {
		return cc3xx_internal_eddsa_verify(attributes, key, key_length, alg, input,
						   input_length, signature, signature_length);
	} else if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) || PSA_ALG_IS_RSA_PSS(alg)) {
		return cc3xx_internal_rsa_verify(attributes, key, key_length, alg, input,
						 input_length, signature, signature_length, true);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}
