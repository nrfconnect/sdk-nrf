/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRYPTO_SERVICE_H__
#define CRYPTO_SERVICE_H__

#include <psa/crypto.h>

/** @brief Execute psa_crypto_init over SSF.
 *
 * See psa_crypto_init for details.
 */
psa_status_t ssf_psa_crypto_init(void);

/** @brief Execute psa_get_key_attributes over SSF.
 *
 * See psa_get_key_attributes for details.
 */
psa_status_t ssf_psa_get_key_attributes(mbedtls_svc_key_id_t key, psa_key_attributes_t *attributes);

/** @brief Execute psa_reset_key_attributes over SSF.
 *
 * See psa_reset_key_attributes for details.
 */
psa_status_t ssf_psa_reset_key_attributes(psa_key_attributes_t *attributes);

/** @brief Execute psa_purge_key over SSF.
 *
 * See psa_purge_key for details.
 */
psa_status_t ssf_psa_purge_key(mbedtls_svc_key_id_t key);

/** @brief Execute psa_copy_key over SSF.
 *
 * See psa_copy_key for details.
 */
psa_status_t ssf_psa_copy_key(mbedtls_svc_key_id_t source_key,
			      const psa_key_attributes_t *attributes,
			      mbedtls_svc_key_id_t *target_key);

/** @brief Execute psa_destroy_key over SSF.
 *
 * See psa_destroy_key for details.
 */
psa_status_t ssf_psa_destroy_key(mbedtls_svc_key_id_t key);

/** @brief Execute psa_import_key over SSF.
 *
 * See psa_import_key for details.
 */
psa_status_t ssf_psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
				size_t data_length, mbedtls_svc_key_id_t *key);

/** @brief Execute psa_export_key over SSF.
 *
 * See psa_export_key for details.
 */
psa_status_t ssf_psa_export_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
				size_t *data_length);

/** @brief Execute psa_export_public_key over SSF.
 *
 * See psa_export_public_key for details.
 */
psa_status_t ssf_psa_export_public_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
				       size_t *data_length);

/** @brief Execute psa_hash_compute over SSF.
 *
 * See psa_hash_compute for details.
 */
psa_status_t ssf_psa_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				  uint8_t *hash, size_t hash_size, size_t *hash_length);

/** @brief Execute psa_hash_compare over SSF.
 *
 * See psa_hash_compare for details.
 */
psa_status_t ssf_psa_hash_compare(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				  const uint8_t *hash, size_t hash_length);

/** @brief Execute psa_hash_setup over SSF.
 *
 * See psa_hash_setup for details.
 */
psa_status_t ssf_psa_hash_setup(mbedtls_psa_client_handle_t *p_handle, psa_algorithm_t alg);

/** @brief Execute psa_hash_update over SSF.
 *
 * See psa_hash_update for details.
 */
psa_status_t ssf_psa_hash_update(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				 size_t input_length);

/** @brief Execute psa_hash_finish over SSF.
 *
 * See psa_hash_finish for details.
 */
psa_status_t ssf_psa_hash_finish(mbedtls_psa_client_handle_t *p_handle, uint8_t *hash,
				 size_t hash_size, size_t *hash_length);

/** @brief Execute psa_hash_verify over SSF.
 *
 * See psa_hash_verify for details.
 */
psa_status_t ssf_psa_hash_verify(mbedtls_psa_client_handle_t *p_handle, const uint8_t *hash,
				 size_t hash_length);

/** @brief Execute psa_hash_abort over SSF.
 *
 * See psa_hash_abort for details.
 */
psa_status_t ssf_psa_hash_abort(mbedtls_psa_client_handle_t *p_handle);

/** @brief Execute psa_hash_clone over SSF.
 *
 * See psa_hash_clone for details.
 */
psa_status_t ssf_psa_hash_clone(mbedtls_psa_client_handle_t handle,
				mbedtls_psa_client_handle_t *p_handle);

/** @brief Execute psa_mac_compute over SSF.
 *
 * See psa_mac_compute for details.
 */
psa_status_t ssf_psa_mac_compute(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				 const uint8_t *input, size_t input_length, uint8_t *mac,
				 size_t mac_size, size_t *mac_length);

/** @brief Execute psa_mac_verify over SSF.
 *
 * See psa_mac_verify for details.
 */
psa_status_t ssf_psa_mac_verify(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, const uint8_t *mac, size_t mac_length);

/** @brief Execute psa_mac_sign_setup over SSF.
 *
 * See psa_mac_sign_setup for details.
 */
psa_status_t ssf_psa_mac_sign_setup(mbedtls_psa_client_handle_t *p_handle, mbedtls_svc_key_id_t key,
				    psa_algorithm_t alg);

/** @brief Execute psa_mac_verify_setup over SSF.
 *
 * See psa_mac_verify_setup for details.
 */
psa_status_t ssf_psa_mac_verify_setup(mbedtls_psa_client_handle_t *p_handle,
				      mbedtls_svc_key_id_t key, psa_algorithm_t alg);

/** @brief Execute psa_mac_update over SSF.
 *
 * See psa_mac_update for details.
 */
psa_status_t ssf_psa_mac_update(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				size_t input_length);

/** @brief Execute psa_mac_sign_finish over SSF.
 *
 * See psa_mac_sign_finish for details.
 */
psa_status_t ssf_psa_mac_sign_finish(mbedtls_psa_client_handle_t *p_handle, uint8_t *mac,
				     size_t mac_size, size_t *mac_length);

/** @brief Execute psa_mac_verify_finish over SSF.
 *
 * See psa_mac_verify_finish for details.
 */
psa_status_t ssf_psa_mac_verify_finish(mbedtls_psa_client_handle_t *p_handle, const uint8_t *mac,
				       size_t mac_length);

/** @brief Execute psa_mac_abort over SSF.
 *
 * See psa_mac_abort for details.
 */
psa_status_t ssf_psa_mac_abort(mbedtls_psa_client_handle_t *p_handle);

/** @brief Execute psa_cipher_encrypt over SSF.
 *
 * See psa_cipher_encrypt for details.
 */
psa_status_t ssf_psa_cipher_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				    const uint8_t *input, size_t input_length, uint8_t *output,
				    size_t output_size, size_t *output_length);

/** @brief Execute psa_cipher_decrypt over SSF.
 *
 * See psa_cipher_decrypt for details.
 */
psa_status_t ssf_psa_cipher_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				    const uint8_t *input, size_t input_length, uint8_t *output,
				    size_t output_size, size_t *output_length);

/** @brief Execute psa_cipher_encrypt_setup over SSF.
 *
 * See psa_cipher_encrypt_setup for details.
 */
psa_status_t ssf_psa_cipher_encrypt_setup(mbedtls_psa_client_handle_t *p_handle,
					  mbedtls_svc_key_id_t key, psa_algorithm_t alg);

/** @brief Execute psa_cipher_decrypt_setup over SSF.
 *
 * See psa_cipher_decrypt_setup for details.
 */
psa_status_t ssf_psa_cipher_decrypt_setup(mbedtls_psa_client_handle_t *p_handle,
					  mbedtls_svc_key_id_t key, psa_algorithm_t alg);

/** @brief Execute psa_cipher_generate_iv over SSF.
 *
 * See psa_cipher_generate_iv for details.
 */
psa_status_t ssf_psa_cipher_generate_iv(mbedtls_psa_client_handle_t *p_handle, uint8_t *iv,
					size_t iv_size, size_t *iv_length);

/** @brief Execute psa_cipher_set_iv over SSF.
 *
 * See psa_cipher_set_iv for details.
 */
psa_status_t ssf_psa_cipher_set_iv(mbedtls_psa_client_handle_t *p_handle, const uint8_t *iv,
				   size_t iv_length);

/** @brief Execute psa_cipher_update over SSF.
 *
 * See psa_cipher_update for details.
 */
psa_status_t ssf_psa_cipher_update(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				   size_t input_length, uint8_t *output, size_t output_size,
				   size_t *output_length);

/** @brief Execute psa_cipher_finish over SSF.
 *
 * See psa_cipher_finish for details.
 */
psa_status_t ssf_psa_cipher_finish(mbedtls_psa_client_handle_t *p_handle, uint8_t *output,
				   size_t output_size, size_t *output_length);

/** @brief Execute psa_cipher_abort over SSF.
 *
 * See psa_cipher_abort for details.
 */
psa_status_t ssf_psa_cipher_abort(mbedtls_psa_client_handle_t *p_handle);

/** @brief Execute psa_aead_encrypt over SSF.
 *
 * See psa_aead_encrypt for details.
 */
psa_status_t ssf_psa_aead_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				  const uint8_t *nonce, size_t nonce_length,
				  const uint8_t *additional_data, size_t additional_data_length,
				  const uint8_t *plaintext, size_t plaintext_length,
				  uint8_t *ciphertext, size_t ciphertext_size,
				  size_t *ciphertext_length);

/** @brief Execute psa_aead_decrypt over SSF.
 *
 * See psa_aead_decrypt for details.
 */
psa_status_t ssf_psa_aead_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				  const uint8_t *nonce, size_t nonce_length,
				  const uint8_t *additional_data, size_t additional_data_length,
				  const uint8_t *ciphertext, size_t ciphertext_length,
				  uint8_t *plaintext, size_t plaintext_size,
				  size_t *plaintext_length);

/** @brief Execute psa_aead_encrypt_setup over SSF.
 *
 * See psa_aead_encrypt_setup for details.
 */
psa_status_t ssf_psa_aead_encrypt_setup(mbedtls_psa_client_handle_t *p_handle,
					mbedtls_svc_key_id_t key, psa_algorithm_t alg);

/** @brief Execute psa_aead_decrypt_setup over SSF.
 *
 * See psa_aead_decrypt_setup for details.
 */
psa_status_t ssf_psa_aead_decrypt_setup(mbedtls_psa_client_handle_t *p_handle,
					mbedtls_svc_key_id_t key, psa_algorithm_t alg);

/** @brief Execute psa_aead_generate_nonce over SSF.
 *
 * See psa_aead_generate_nonce for details.
 */
psa_status_t ssf_psa_aead_generate_nonce(mbedtls_psa_client_handle_t *p_handle, uint8_t *nonce,
					 size_t nonce_size, size_t *nonce_length);

/** @brief Execute psa_aead_set_nonce over SSF.
 *
 * See psa_aead_set_nonce for details.
 */
psa_status_t ssf_psa_aead_set_nonce(mbedtls_psa_client_handle_t *p_handle, const uint8_t *nonce,
				    size_t nonce_length);

/** @brief Execute psa_aead_set_lengths over SSF.
 *
 * See psa_aead_set_lengths for details.
 */
psa_status_t ssf_psa_aead_set_lengths(mbedtls_psa_client_handle_t *p_handle, size_t ad_length,
				      size_t plaintext_length);

/** @brief Execute psa_aead_update_ad over SSF.
 *
 * See psa_aead_update_ad for details.
 */
psa_status_t ssf_psa_aead_update_ad(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				    size_t input_length);

/** @brief Execute psa_aead_update over SSF.
 *
 * See psa_aead_update for details.
 */
psa_status_t ssf_psa_aead_update(mbedtls_psa_client_handle_t *p_handle, const uint8_t *input,
				 size_t input_length, uint8_t *output, size_t output_size,
				 size_t *output_length);

/** @brief Execute psa_aead_finish over SSF.
 *
 * See psa_aead_finish for details.
 */
psa_status_t ssf_psa_aead_finish(mbedtls_psa_client_handle_t *p_handle, uint8_t *ciphertext,
				 size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
				 size_t tag_size, size_t *tag_length);

/** @brief Execute psa_aead_verify over SSF.
 *
 * See psa_aead_verify for details.
 */
psa_status_t ssf_psa_aead_verify(mbedtls_psa_client_handle_t *p_handle, uint8_t *plaintext,
				 size_t plaintext_size, size_t *plaintext_length,
				 const uint8_t *tag, size_t tag_length);

/** @brief Execute psa_aead_abort over SSF.
 *
 * See psa_aead_abort for details.
 */
psa_status_t ssf_psa_aead_abort(mbedtls_psa_client_handle_t *p_handle);

/** @brief Execute psa_sign_message over SSF.
 *
 * See psa_sign_message for details.
 */
psa_status_t ssf_psa_sign_message(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				  const uint8_t *input, size_t input_length, uint8_t *signature,
				  size_t signature_size, size_t *signature_length);

/** @brief Execute psa_verify_message over SSF.
 *
 * See psa_verify_message for details.
 */
psa_status_t ssf_psa_verify_message(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				    const uint8_t *input, size_t input_length,
				    const uint8_t *signature, size_t signature_length);

/** @brief Execute psa_sign_hash over SSF.
 *
 * See psa_sign_hash for details.
 */
psa_status_t ssf_psa_sign_hash(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
			       size_t hash_length, uint8_t *signature, size_t signature_size,
			       size_t *signature_length);

/** @brief Execute psa_verify_hash over SSF.
 *
 * See psa_verify_hash for details.
 */
psa_status_t ssf_psa_verify_hash(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
				 size_t hash_length, const uint8_t *signature,
				 size_t signature_length);

/** @brief Execute psa_asymmetric_encrypt over SSF.
 *
 * See psa_asymmetric_encrypt for details.
 */
psa_status_t ssf_psa_asymmetric_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
					const uint8_t *input, size_t input_length,
					const uint8_t *salt, size_t salt_length, uint8_t *output,
					size_t output_size, size_t *output_length);

/** @brief Execute psa_asymmetric_decrypt over SSF.
 *
 * See psa_asymmetric_decrypt for details.
 */
psa_status_t ssf_psa_asymmetric_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
					const uint8_t *input, size_t input_length,
					const uint8_t *salt, size_t salt_length, uint8_t *output,
					size_t output_size, size_t *output_length);

/** @brief Execute psa_key_derivation_setup over SSF.
 *
 * See psa_key_derivation_setup for details.
 */
psa_status_t ssf_psa_key_derivation_setup(mbedtls_psa_client_handle_t *p_handle,
					  psa_algorithm_t alg);

/** @brief Execute psa_key_derivation_get_capacity over SSF.
 *
 * See psa_key_derivation_get_capacity for details.
 */
psa_status_t ssf_psa_key_derivation_get_capacity(mbedtls_psa_client_handle_t handle,
						 size_t *capacity);

/** @brief Execute psa_key_derivation_set_capacity over SSF.
 *
 * See psa_key_derivation_set_capacity for details.
 */
psa_status_t ssf_psa_key_derivation_set_capacity(mbedtls_psa_client_handle_t *p_handle,
						 size_t capacity);

/** @brief Execute psa_key_derivation_input_bytes over SSF.
 *
 * See psa_key_derivation_input_bytes for details.
 */
psa_status_t ssf_psa_key_derivation_input_bytes(mbedtls_psa_client_handle_t *p_handle,
						psa_key_derivation_step_t step, const uint8_t *data,
						size_t data_length);

/** @brief Execute psa_key_derivation_input_integer over SSF.
 *
 * See psa_key_derivation_input_integer for details.
 */
psa_status_t ssf_psa_key_derivation_input_integer(mbedtls_psa_client_handle_t *p_handle,
						  psa_key_derivation_step_t step, uint64_t value);

/** @brief Execute psa_key_derivation_input_key over SSF.
 *
 * See psa_key_derivation_input_key for details.
 */
psa_status_t ssf_psa_key_derivation_input_key(mbedtls_psa_client_handle_t *p_handle,
					      psa_key_derivation_step_t step,
					      mbedtls_svc_key_id_t key);

/** @brief Execute psa_key_derivation_key_agreement over SSF.
 *
 * See psa_key_derivation_key_agreement for details.
 */
psa_status_t ssf_psa_key_derivation_key_agreement(mbedtls_psa_client_handle_t *p_handle,
						  psa_key_derivation_step_t step,
						  mbedtls_svc_key_id_t private_key,
						  const uint8_t *peer_key, size_t peer_key_length);

/** @brief Execute psa_key_derivation_output_bytes over SSF.
 *
 * See psa_key_derivation_output_bytes for details.
 */
psa_status_t ssf_psa_key_derivation_output_bytes(mbedtls_psa_client_handle_t *p_handle,
						 uint8_t *output, size_t output_length);

/** @brief Execute psa_key_derivation_output_key over SSF.
 *
 * See psa_key_derivation_output_key for details.
 */
psa_status_t ssf_psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
					       mbedtls_psa_client_handle_t *p_handle,
					       mbedtls_svc_key_id_t *key);

/** @brief Execute psa_key_derivation_abort over SSF.
 *
 * See psa_key_derivation_abort for details.
 */
psa_status_t ssf_psa_key_derivation_abort(mbedtls_psa_client_handle_t *p_handle);

/** @brief Execute psa_raw_key_agreement over SSF.
 *
 * See psa_raw_key_agreement for details.
 */
psa_status_t ssf_psa_raw_key_agreement(psa_algorithm_t alg, mbedtls_svc_key_id_t private_key,
				       const uint8_t *peer_key, size_t peer_key_length,
				       uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Execute psa_generate_random over SSF.
 *
 * See psa_generate_random for details.
 */
psa_status_t ssf_psa_generate_random(uint8_t *output, size_t output_size);

/** @brief Execute psa_generate_key over SSF.
 *
 * See psa_generate_key for details.
 */
psa_status_t ssf_psa_generate_key(const psa_key_attributes_t *attributes,
				  mbedtls_svc_key_id_t *key);

/** @brief Execute psa_sign_hash_start over SSF.
 *
 * See psa_sign_hash_start for details.
 */
psa_status_t ssf_psa_sign_hash_start(psa_sign_hash_interruptible_operation_t *operation,
				     mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				     const uint8_t *hash, size_t hash_length);

/** @brief Execute psa_sign_hash_abort over SSF.
 *
 * See psa_sign_hash_abort for details.
 */
psa_status_t ssf_psa_sign_hash_abort(psa_sign_hash_interruptible_operation_t *operation);

/** @brief Execute psa_verify_hash_start over SSF.
 *
 * See psa_verify_hash_start for details.
 */
psa_status_t ssf_psa_verify_hash_start(psa_verify_hash_interruptible_operation_t *operation,
				       mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				       const uint8_t *hash, size_t hash_length,
				       const uint8_t *signature, size_t signature_length);

/** @brief Execute psa_verify_hash_abort over SSF.
 *
 * See psa_verify_hash_abort for details.
 */
psa_status_t ssf_psa_verify_hash_abort(psa_verify_hash_interruptible_operation_t *operation);

/** @brief Execute psa_pake_setup over SSF.
 *
 * See psa_pake_setup for details.
 */
psa_status_t ssf_psa_pake_setup(mbedtls_psa_client_handle_t *p_handle,
				mbedtls_svc_key_id_t password_key,
				const psa_pake_cipher_suite_t *cipher_suite);

/** @brief Execute psa_pake_set_role over SSF.
 *
 * See psa_pake_set_role for details.
 */
psa_status_t ssf_psa_pake_set_role(mbedtls_psa_client_handle_t *p_handle, psa_pake_role_t role);

/** @brief Execute psa_pake_set_user over SSF.
 *
 * See psa_pake_set_user for details.
 */
psa_status_t ssf_psa_pake_set_user(mbedtls_psa_client_handle_t *p_handle, const uint8_t *user_id,
				   size_t user_id_len);

/** @brief Execute psa_pake_set_peer over SSF.
 *
 * See psa_pake_set_peer for details.
 */
psa_status_t ssf_psa_pake_set_peer(mbedtls_psa_client_handle_t *p_handle, const uint8_t *peer_id,
				   size_t peer_id_len);

/** @brief Execute psa_pake_set_context over SSF.
 *
 * See psa_pake_set_context for details.
 */
psa_status_t ssf_psa_pake_set_context(mbedtls_psa_client_handle_t *p_handle, const uint8_t *context,
				      size_t context_len);

/** @brief Execute psa_pake_output over SSF.
 *
 * See psa_pake_output for details.
 */
psa_status_t ssf_psa_pake_output(mbedtls_psa_client_handle_t *p_handle, psa_pake_step_t step,
				 uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Execute psa_pake_input over SSF.
 *
 * See psa_pake_input for details.
 */
psa_status_t ssf_psa_pake_input(mbedtls_psa_client_handle_t *p_handle, psa_pake_step_t step,
				const uint8_t *input, size_t input_length);

/** @brief Execute psa_pake_get_shared_key over SSF.
 *
 * See psa_pake_get_shared_key for details.
 */
psa_status_t ssf_psa_pake_get_shared_key(mbedtls_psa_client_handle_t *p_handle,
					 const psa_key_attributes_t *attributes,
					 mbedtls_svc_key_id_t *key);

/** @brief Execute psa_pake_abort over SSF.
 *
 * See psa_pake_abort for details.
 */
psa_status_t ssf_psa_pake_abort(mbedtls_psa_client_handle_t *p_handle);

#endif /* CRYPTO_SERVICE_H__ */
