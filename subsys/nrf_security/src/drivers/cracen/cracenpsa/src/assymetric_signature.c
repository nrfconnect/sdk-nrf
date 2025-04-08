/**
 * @file
 *
 * @copyright Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <sicrypto/drbghash.h>
#include <sicrypto/ik.h>
#include <sicrypto/internal.h>
#include <sicrypto/rsapss.h>
#include <sicrypto/rsassa_pkcs1v15.h>
#include <sicrypto/sicrypto.h>
#include <silexpk/blinding.h>
#include <silexpk/ec_curves.h>
#include <silexpk/ed25519.h>
#include <silexpk/ik.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <string.h>
#include <sxsymcrypt/hashdefs.h>
#include <sxsymcrypt/trng.h>
#include "hashdefs.h"

#include "common.h"
#include "cracen_psa.h"
#include "cracen_psa_ecdsa.h"
#include "cracen_psa_eddsa.h"
#include "ecc.h"
#include "cracen_signature_ecc.h"
#include "cracen_signature_rsa.h"

#define SI_IS_MESSAGE	   (1)
#define SI_IS_HASH	   (0)
#define SI_EXTRACT_PUBKEY  (1)
#define SI_EXTRACT_PRIVKEY (0)

psa_status_t cracen_sign_message(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				 size_t input_length, uint8_t *signature, size_t signature_size,
				 size_t *signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC)) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			return cracen_signature_ecc_sign(
				SI_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg, input,
				input_length, signature, signature_size, signature_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)) {
		if (PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
			return cracen_signature_rsa_sign(
				SI_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg, input,
				input_length, signature, signature_size, signature_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_sign_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			      size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
			      size_t hash_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC)) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			return cracen_signature_ecc_sign(
				SI_IS_HASH, attributes, key_buffer, key_buffer_size, alg, hash,
				hash_length, signature, signature_size, signature_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)) {
		if (PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
			return cracen_signature_rsa_sign(
				SI_IS_HASH, attributes, key_buffer, key_buffer_size, alg, hash,
				hash_length, signature, signature_size, signature_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_verify_message(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   const uint8_t *signature, size_t signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC)) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			return cracen_signature_ecc_verify(
				SI_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg, input,
				input_length, signature, signature_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)) {
		if (PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
			return cracen_signature_rsa_verify(
				SI_IS_MESSAGE, attributes, key_buffer, key_buffer_size, alg, input,
				input_length, signature, signature_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_verify_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
				size_t hash_length, const uint8_t *signature,
				size_t signature_length)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_ECC)) {
		if (PSA_KEY_TYPE_IS_ECC(psa_get_key_type(attributes))) {
			return cracen_signature_ecc_verify(SI_IS_HASH, attributes, key_buffer,
							   key_buffer_size, alg, hash, hash_length,
							   signature, signature_length);
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)) {
		if (PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
			return cracen_signature_rsa_verify(SI_IS_HASH, attributes, key_buffer,
							   key_buffer_size, alg, hash, hash_length,
							   signature, signature_length);
		}
	}

	return PSA_ERROR_NOT_SUPPORTED;
}
