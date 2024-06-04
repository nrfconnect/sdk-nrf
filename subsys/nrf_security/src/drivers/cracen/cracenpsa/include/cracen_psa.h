/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_H
#define CRACEN_PSA_H

#include <psa/crypto.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"
#include "cracen_psa_kmu.h"
#include "cracen_psa_key_ids.h"
#include "sxsymcrypt/keyref.h"

#ifdef __NRF_TFM__
#include <tfm_builtin_key_loader.h>
#endif

/**
 * See "PSA Cryptography API" for documentation.
 */

psa_status_t cracen_sign_message(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				 size_t input_length, uint8_t *signature, size_t signature_size,
				 size_t *signature_length);

psa_status_t cracen_verify_message(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   const uint8_t *signature, size_t signature_length);

psa_status_t cracen_sign_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			      size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
			      size_t hash_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length);

psa_status_t cracen_verify_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
				size_t hash_length, const uint8_t *signature,
				size_t signature_length);

psa_status_t cracen_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				 uint8_t *hash, size_t hash_size, size_t *hash_length);

psa_status_t cracen_hash_setup(cracen_hash_operation_t *operation, psa_algorithm_t alg);

static inline psa_status_t cracen_hash_clone(const cracen_hash_operation_t *source_operation,
					     cracen_hash_operation_t *target_operation)
{
	__ASSERT_NO_MSG(source_operation != NULL);
	__ASSERT_NO_MSG(target_operation != NULL);

	memcpy(target_operation, source_operation, sizeof(cracen_hash_operation_t));

	return PSA_SUCCESS;
}

psa_status_t cracen_hash_update(cracen_hash_operation_t *operation, const uint8_t *input,
				const size_t input_length);

psa_status_t cracen_hash_finish(cracen_hash_operation_t *operation, uint8_t *hash, size_t hash_size,
				size_t *hash_length);

static inline psa_status_t cracen_hash_abort(cracen_hash_operation_t *operation)
{
	__ASSERT_NO_MSG(operation != NULL);

	safe_memzero(operation, sizeof(cracen_hash_operation_t));

	return PSA_SUCCESS;
}

psa_status_t cracen_aead_encrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *plaintext,
				 size_t plaintext_length, uint8_t *ciphertext,
				 size_t ciphertext_size, size_t *ciphertext_length);

psa_status_t cracen_aead_decrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *ciphertext,
				 size_t ciphertext_length, uint8_t *plaintext,
				 size_t plaintext_size, size_t *plaintext_length);

psa_status_t cracen_cipher_encrypt(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *iv, size_t iv_length,
				   const uint8_t *input, size_t input_length, uint8_t *output,
				   size_t output_size, size_t *output_length);

psa_status_t cracen_cipher_decrypt(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t cracen_cipher_encrypt_setup(cracen_cipher_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg);

psa_status_t cracen_cipher_decrypt_setup(cracen_cipher_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg);

psa_status_t cracen_cipher_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				  size_t iv_length);

psa_status_t cracen_cipher_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				  size_t input_length, uint8_t *output, size_t output_size,
				  size_t *output_length);

psa_status_t cracen_cipher_finish(cracen_cipher_operation_t *operation, uint8_t *output,
				  size_t output_size, size_t *output_length);

psa_status_t cracen_cipher_abort(cracen_cipher_operation_t *operation);

psa_status_t cracen_aead_encrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg);

psa_status_t cracen_aead_decrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg);

psa_status_t cracen_aead_set_nonce(cracen_aead_operation_t *operation, const uint8_t *nonce,
				   size_t nonce_length);

psa_status_t cracen_aead_set_lengths(cracen_aead_operation_t *operation, size_t ad_length,
				     size_t plaintext_length);

psa_status_t cracen_aead_update_ad(cracen_aead_operation_t *operation, const uint8_t *input,
				   size_t input_length);

psa_status_t cracen_aead_update(cracen_aead_operation_t *operation, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length);

psa_status_t cracen_aead_finish(cracen_aead_operation_t *operation, uint8_t *ciphertext,
				size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
				size_t tag_size, size_t *tag_length);

psa_status_t cracen_aead_verify(cracen_aead_operation_t *operation, uint8_t *plaintext,
				size_t plaintext_size, size_t *plaintext_length, const uint8_t *tag,
				size_t tag_length);

psa_status_t cracen_aead_abort(cracen_aead_operation_t *operation);

psa_status_t cracen_mac_compute(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *mac, size_t mac_size,
				size_t *mac_length);

psa_status_t cracen_mac_sign_setup(cracen_mac_operation_t *operation,
				   const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg);

psa_status_t cracen_mac_verify_setup(cracen_mac_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg);

psa_status_t cracen_mac_update(cracen_mac_operation_t *operation, const uint8_t *input,
			       size_t input_length);

psa_status_t cracen_mac_sign_finish(cracen_mac_operation_t *operation, uint8_t *mac,
				    size_t mac_size, size_t *mac_length);

psa_status_t cracen_mac_verify_finish(cracen_mac_operation_t *operation, const uint8_t *mac,
				      size_t mac_length);

psa_status_t cracen_mac_abort(cracen_mac_operation_t *operation);

psa_status_t cracen_key_agreement(const psa_key_attributes_t *attributes, const uint8_t *priv_key,
				  size_t priv_key_size, const uint8_t *publ_key,
				  size_t publ_key_size, uint8_t *output, size_t output_size,
				  size_t *output_length, psa_algorithm_t alg);

psa_status_t cracen_key_derivation_setup(cracen_key_derivation_operation_t *operation,
					 psa_algorithm_t alg);

psa_status_t cracen_key_derivation_set_capacity(cracen_key_derivation_operation_t *operation,
						size_t capacity);

psa_status_t cracen_key_derivation_input_key(cracen_key_derivation_operation_t *operation,
					     psa_key_derivation_step_t step,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size);

psa_status_t cracen_key_derivation_input_bytes(cracen_key_derivation_operation_t *operation,
					       psa_key_derivation_step_t step, const uint8_t *data,
					       size_t data_length);

psa_status_t cracen_key_derivation_input_integer(cracen_key_derivation_operation_t *operation,
						 psa_key_derivation_step_t step, uint64_t value);

psa_status_t cracen_key_derivation_output_bytes(cracen_key_derivation_operation_t *operation,
						uint8_t *output, size_t output_length);

psa_status_t cracen_key_derivation_abort(cracen_key_derivation_operation_t *operation);

psa_status_t cracen_export_public_key(const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, size_t key_buffer_size,
				      uint8_t *data, size_t data_size, size_t *data_length);

psa_status_t cracen_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			       size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
			       size_t *key_buffer_length, size_t *key_bits);

psa_status_t cracen_generate_key(const psa_key_attributes_t *attributes, uint8_t *key_buffer,
				 size_t key_buffer_size, size_t *key_buffer_length);

psa_status_t cracen_asymmetric_encrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t cracen_asymmetric_decrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t cracen_get_builtin_key(psa_drv_slot_number_t slot_number,
				    psa_key_attributes_t *attributes, uint8_t *key_buffer,
				    size_t key_buffer_size, size_t *key_buffer_length);

psa_status_t cracen_export_key(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			       size_t key_buffer_size, uint8_t *data, size_t data_size,
			       size_t *data_length);

psa_status_t cracen_destroy_key(const psa_key_attributes_t *attributes);

size_t cracen_get_opaque_size(const psa_key_attributes_t *attributes);

psa_status_t cracen_jpake_setup(cracen_jpake_operation_t *operation,
				const psa_key_attributes_t *attributes, const uint8_t *password,
				size_t password_length,
				const psa_pake_cipher_suite_t *cipher_suite);

psa_status_t cracen_jpake_set_password_key(cracen_jpake_operation_t *operation,
					   const psa_key_attributes_t *attributes,
					   const uint8_t *password, size_t password_length);

psa_status_t cracen_jpake_set_user(cracen_jpake_operation_t *operation, const uint8_t *user_id,
				   size_t user_id_len);

psa_status_t cracen_jpake_set_peer(cracen_jpake_operation_t *operation, const uint8_t *peer_id,
				   size_t peer_id_len);

psa_status_t cracen_jpake_set_role(cracen_jpake_operation_t *operation, psa_pake_role_t role);

psa_status_t cracen_jpake_output(cracen_jpake_operation_t *operation, psa_pake_step_t step,
				 uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t cracen_jpake_input(cracen_jpake_operation_t *operation, psa_pake_step_t step,
				const uint8_t *input, size_t input_length);

psa_status_t cracen_jpake_get_shared_key(cracen_jpake_operation_t *operation,
					 const psa_key_attributes_t *attributes, uint8_t *output,
					 size_t output_size, size_t *output_length);

psa_status_t cracen_jpake_abort(cracen_jpake_operation_t *operation);

psa_status_t cracen_init_random(cracen_prng_context_t *context);
psa_status_t cracen_get_random(cracen_prng_context_t *context, uint8_t *output, size_t output_size);
psa_status_t cracen_free_random(cracen_prng_context_t *context);

psa_status_t cracen_srp_setup(cracen_srp_operation_t *operation,
			      const psa_key_attributes_t *attributes, const uint8_t *password,
			      size_t password_length, const psa_pake_cipher_suite_t *cipher_suite);

psa_status_t cracen_srp_set_user(cracen_srp_operation_t *operation, const uint8_t *user_id,
				 size_t user_id_len);
psa_status_t cracen_srp_set_role(cracen_srp_operation_t *operation, psa_pake_role_t role);
psa_status_t cracen_srp_set_password_key(cracen_srp_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *password, size_t password_length);
psa_status_t cracen_srp_output(cracen_srp_operation_t *operation, psa_pake_step_t step,
			       uint8_t *output, size_t output_size, size_t *output_length);
psa_status_t cracen_srp_input(cracen_srp_operation_t *operation, psa_pake_step_t step,
			      const uint8_t *input, size_t input_length);
psa_status_t cracen_srp_get_shared_key(cracen_srp_operation_t *operation,
				       const psa_key_attributes_t *attributes, uint8_t *output,
				       size_t output_size, size_t *output_length);

psa_status_t cracen_srp_abort(cracen_srp_operation_t *operation);

psa_status_t cracen_pake_setup(cracen_pake_operation_t *operation,
			       const psa_key_attributes_t *attributes, const uint8_t *password,
			       size_t password_length, const psa_pake_cipher_suite_t *cipher_suite);

psa_status_t cracen_pake_set_user(cracen_pake_operation_t *operation, const uint8_t *user_id,
				  size_t user_id_len);

psa_status_t cracen_pake_set_peer(cracen_pake_operation_t *operation, const uint8_t *peer_id,
				  size_t peer_id_len);

psa_status_t cracen_pake_set_context(cracen_pake_operation_t *operation, const uint8_t *context,
				     size_t context_length);

psa_status_t cracen_pake_set_role(cracen_pake_operation_t *operation, psa_pake_role_t role);

psa_status_t cracen_pake_output(cracen_pake_operation_t *operation, psa_pake_step_t step,
				uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t cracen_pake_input(cracen_pake_operation_t *operation, psa_pake_step_t step,
			       const uint8_t *input, size_t input_length);

psa_status_t cracen_pake_get_shared_key(cracen_pake_operation_t *operation,
					const psa_key_attributes_t *attributes, uint8_t *key_buffer,
					size_t key_buffer_size, size_t *key_buffer_length);

psa_status_t cracen_pake_abort(cracen_pake_operation_t *operation);

psa_status_t cracen_spake2p_setup(cracen_spake2p_operation_t *operation,
				  const psa_key_attributes_t *attributes, const uint8_t *password,
				  size_t password_length,
				  const psa_pake_cipher_suite_t *cipher_suite);

psa_status_t cracen_spake2p_set_password_key(cracen_spake2p_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *password, size_t password_length);

psa_status_t cracen_spake2p_set_user(cracen_spake2p_operation_t *operation, const uint8_t *user_id,
				     size_t user_id_len);

psa_status_t cracen_spake2p_set_peer(cracen_spake2p_operation_t *operation, const uint8_t *peer_id,
				     size_t peer_id_len);

psa_status_t cracen_spake2p_set_context(cracen_spake2p_operation_t *operation,
					const uint8_t *context, size_t context_length);

psa_status_t cracen_spake2p_set_role(cracen_spake2p_operation_t *operation, psa_pake_role_t role);

psa_status_t cracen_spake2p_output(cracen_spake2p_operation_t *operation, psa_pake_step_t step,
				   uint8_t *output, size_t output_size, size_t *output_length);

psa_status_t cracen_spake2p_input(cracen_spake2p_operation_t *operation, psa_pake_step_t step,
				  const uint8_t *input, size_t input_length);

psa_status_t cracen_spake2p_get_shared_key(cracen_spake2p_operation_t *operation,
					   const psa_key_attributes_t *attributes, uint8_t *output,
					   size_t output_size, size_t *output_length);

psa_status_t cracen_spake2p_abort(cracen_spake2p_operation_t *operation);

#endif /* CRACEN_PSA_H */
