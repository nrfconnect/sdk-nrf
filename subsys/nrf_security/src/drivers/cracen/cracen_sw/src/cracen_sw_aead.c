/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cracen/mem_helpers.h>
#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#include <cracen_psa_primitives.h>
#include "../../../cracenpsa/src/common.h"
#include <cracen_sw_common.h>
#include <cracen_sw_aead.h>

#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
#include <cracen_sw_aes_ccm.h>
#endif

psa_status_t cracen_aead_encrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg)
{
	if (key_buffer_size > sizeof(operation->key_buffer)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_encrypt_setup(operation, attributes, key_buffer,
						       key_buffer_size, alg);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_aead_decrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg)
{
	if (key_buffer_size > sizeof(operation->key_buffer)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_decrypt_setup(operation, attributes, key_buffer,
						       key_buffer_size, alg);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_aead_set_nonce(cracen_aead_operation_t *operation, const uint8_t *nonce,
				   size_t nonce_length)
{
#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (operation->alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_set_nonce(operation, nonce, nonce_length);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_aead_set_lengths(cracen_aead_operation_t *operation, size_t ad_length,
				     size_t plaintext_length)
{
#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (operation->alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_set_lengths(operation, ad_length, plaintext_length);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_aead_update_ad(cracen_aead_operation_t *operation, const uint8_t *input,
				   size_t input_length)
{
	if (operation->ad_finished) {
		return PSA_ERROR_BAD_STATE;
	}
	if (input_length == 0) {
		return PSA_SUCCESS;
	}

#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (operation->alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_update_ad(operation, input, input_length);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_aead_update(cracen_aead_operation_t *operation, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length)
{
	*output_length = 0;
	if (input_length == 0) {
		return PSA_SUCCESS;
	}
	if (output == NULL || output_size < input_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (operation->alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_update(operation, input, input_length, output, output_size,
						output_length);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_aead_finish(cracen_aead_operation_t *operation, uint8_t *ciphertext,
				size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
				size_t tag_size, size_t *tag_length)
{
	*ciphertext_length = 0;
	*tag_length = 0;
	if (tag_size < operation->tag_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (operation->alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_finish(operation, ciphertext, ciphertext_size,
						ciphertext_length, tag, tag_size, tag_length);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_aead_verify(cracen_aead_operation_t *operation, uint8_t *plaintext,
				size_t plaintext_size, size_t *plaintext_length, const uint8_t *tag,
				size_t tag_length)
{
	*plaintext_length = 0;
	if (tag_length != operation->tag_size) {
		return PSA_ERROR_INVALID_SIGNATURE;
	}

#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (operation->alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_verify(operation, plaintext, plaintext_size,
						plaintext_length, tag, tag_length);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_aead_abort(cracen_aead_operation_t *operation)
{
	switch (operation->alg) {
#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	case PSA_ALG_CCM:
		return cracen_sw_aes_ccm_abort(operation);
#endif
	default:
		safe_memzero(operation, sizeof(*operation));
		return PSA_SUCCESS;
	}
}

psa_status_t cracen_aead_encrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *plaintext,
				 size_t plaintext_length, uint8_t *ciphertext,
				 size_t ciphertext_size, size_t *ciphertext_length)
{
#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_encrypt(
			attributes, key_buffer, key_buffer_size, alg, nonce, nonce_length,
			additional_data, additional_data_length, plaintext, plaintext_length,
			ciphertext, ciphertext_size, ciphertext_length);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t cracen_aead_decrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *ciphertext,
				 size_t ciphertext_length, uint8_t *plaintext,
				 size_t plaintext_size, size_t *plaintext_length)
{
#if defined(CONFIG_PSA_NEED_CRACEN_CCM_AES)
	if (alg == PSA_ALG_CCM) {
		return cracen_sw_aes_ccm_decrypt(
			attributes, key_buffer, key_buffer_size, alg, nonce, nonce_length,
			additional_data, additional_data_length, ciphertext, ciphertext_length,
			plaintext, plaintext_size, plaintext_length);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}
