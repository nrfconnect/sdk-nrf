/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"
#include "cracen_psa_primitives.h"

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <sicrypto/rsa_keys.h>
#include <sicrypto/rsaes_oaep.h>
#include <sicrypto/sicrypto.h>
#include <sicrypto/rsaes_pkcs1v15.h>
#include <silexpk/blinding.h>

#define SI_DMAMEM_MAX_SZ (1024)

static bool is_alg_supported(psa_algorithm_t alg)
{
	if (IS_ENABLED(PSA_NEED_CRACEN_RSA_OAEP)) {
		if (PSA_ALG_IS_RSA_OAEP(alg)) {
			return true;
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_RSA_PKCS1V15_CRYPT)) {
		if (alg == PSA_ALG_RSA_PKCS1V15_CRYPT) {
			return true;
		}
	}

	return false;
}

/* Use a define to fix a line wrap formatting compliance issue */
#define WORKMEM_SIZE (PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + PSA_HASH_MAX_SIZE + 4)

static psa_status_t
cracen_asymmetric_crypt_internal(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				 size_t input_length, const uint8_t *salt, size_t salt_length,
				 uint8_t *output, size_t output_size, size_t *output_length,
				 enum cipher_operation dir)
{
#if PSA_MAX_RSA_KEY_BITS > 0
	psa_status_t status = PSA_SUCCESS;
	struct sitask t;
	const struct sxhashalg *hashalg;
	struct si_rsa_key pubkey;
	struct sx_buf modulus;
	struct sx_buf exponent;

	if (!is_alg_supported(alg)) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	int si_status = cracen_signature_get_rsa_key(
		&pubkey, dir == CRACEN_ENCRYPT,
		psa_get_key_type(attributes) == PSA_KEY_TYPE_RSA_KEY_PAIR, key_buffer,
		key_buffer_size, &modulus, &exponent);

	if (si_status != SX_OK) {
		return silex_statuscodes_to_psa(si_status);
	}

	struct si_ase_text text = {(char *)input, input_length};

	char workmem[WORKMEM_SIZE] = {};

	si_task_init(&t, workmem, sizeof(workmem));

	if (IS_ENABLED(PSA_NEED_CRACEN_RSA_OAEP)) {
		if (PSA_ALG_IS_RSA_OAEP(alg)) {
			status = hash_get_algo(PSA_ALG_RSA_OAEP_GET_HASH(alg), &hashalg);
			if (status != PSA_SUCCESS) {
				return status;
			}

			struct sx_buf label = {salt_length, (char *)salt};

			if (dir == CRACEN_ENCRYPT) {
				si_ase_create_rsa_oaep_encrypt(&t, hashalg, &pubkey, &text, &label);
			} else {
				si_ase_create_rsa_oaep_decrypt(&t, hashalg, &pubkey, &text, &label);
			}
		}
	}

	if (IS_ENABLED(PSA_NEED_CRACEN_RSA_PKCS1V15_CRYPT)) {
		if (alg == PSA_ALG_RSA_PKCS1V15_CRYPT) {
			if (dir == CRACEN_ENCRYPT) {
				si_ase_create_rsa_pkcs1v15_encrypt(&t, &pubkey, &text);
			} else {
				si_ase_create_rsa_pkcs1v15_decrypt(&t, &pubkey, &text);
			}
		}
	}

	si_task_run(&t);

	si_status = si_task_wait(&t);

	if (si_status != SX_OK) {
		status = silex_statuscodes_to_psa(si_status);
		goto exit;
	}

	if (text.sz > output_size) {
		status = PSA_ERROR_BUFFER_TOO_SMALL;
		goto exit;
	}

	memcpy(output, text.addr, text.sz);
	*output_length = text.sz;

exit:
	safe_memzero(workmem, sizeof(workmem));
	return status;
#else
	return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_MAX_RSA_KEY_BITS > 0 */
}

psa_status_t cracen_asymmetric_encrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length)
{
	return cracen_asymmetric_crypt_internal(attributes, key_buffer, key_buffer_size, alg, input,
						input_length, salt, salt_length, output,
						output_size, output_length, CRACEN_ENCRYPT);
}

psa_status_t cracen_asymmetric_decrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length)
{
	return cracen_asymmetric_crypt_internal(attributes, key_buffer, key_buffer_size, alg, input,
						input_length, salt, salt_length, output,
						output_size, output_length, CRACEN_DECRYPT);
}
