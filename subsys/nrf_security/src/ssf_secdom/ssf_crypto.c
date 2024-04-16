/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <psa/crypto.h>
#include <sdfw/sdfw_services/crypto_service.h>

psa_status_t psa_crypto_init(void)
{
	return ssf_psa_crypto_init();
}

psa_status_t psa_get_key_attributes(mbedtls_svc_key_id_t key, psa_key_attributes_t *attributes)
{
	return ssf_psa_get_key_attributes(key, attributes);
}

void psa_reset_key_attributes(psa_key_attributes_t *attributes)
{
	ssf_psa_reset_key_attributes(attributes);
}

psa_status_t psa_purge_key(mbedtls_svc_key_id_t key)
{
	return ssf_psa_purge_key(key);
}

psa_status_t psa_copy_key(mbedtls_svc_key_id_t source_key, const psa_key_attributes_t *attributes,
			  mbedtls_svc_key_id_t *target_key)
{
	return ssf_psa_copy_key(source_key, attributes, target_key);
}

psa_status_t psa_destroy_key(mbedtls_svc_key_id_t key)
{
	return ssf_psa_destroy_key(key);
}

psa_status_t psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, mbedtls_svc_key_id_t *key)
{
	return ssf_psa_import_key(attributes, data, data_length, key);
}

psa_status_t psa_export_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
			    size_t *data_length)
{
	return ssf_psa_export_key(key, data, data_size, data_length);
}

psa_status_t psa_export_public_key(mbedtls_svc_key_id_t key, uint8_t *data, size_t data_size,
				   size_t *data_length)
{
	return ssf_psa_export_public_key(key, data, data_size, data_length);
}

psa_status_t psa_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
			      uint8_t *hash, size_t hash_size, size_t *hash_length)
{
	return ssf_psa_hash_compute(alg, input, input_length, hash, hash_size, hash_length);
}

psa_status_t psa_hash_compare(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
			      const uint8_t *hash, size_t hash_length)
{
	return ssf_psa_hash_compare(alg, input, input_length, hash, hash_length);
}

psa_status_t psa_hash_setup(psa_hash_operation_t *operation, psa_algorithm_t alg)
{
	return ssf_psa_hash_setup(&operation->handle, alg);
}

psa_status_t psa_hash_update(psa_hash_operation_t *operation, const uint8_t *input,
			     size_t input_length)
{
	return ssf_psa_hash_update(&operation->handle, input, input_length);
}

psa_status_t psa_hash_finish(psa_hash_operation_t *operation, uint8_t *hash, size_t hash_size,
			     size_t *hash_length)
{
	return ssf_psa_hash_finish(&operation->handle, hash, hash_size, hash_length);
}

psa_status_t psa_hash_verify(psa_hash_operation_t *operation, const uint8_t *hash,
			     size_t hash_length)
{
	return ssf_psa_hash_verify(&operation->handle, hash, hash_length);
}

psa_status_t psa_hash_abort(psa_hash_operation_t *operation)
{
	return ssf_psa_hash_abort(&operation->handle);
}

psa_status_t psa_hash_clone(const psa_hash_operation_t *source_operation,
			    psa_hash_operation_t *target_operation)
{
	return ssf_psa_hash_clone(source_operation->handle, &target_operation->handle);
}

psa_status_t psa_mac_compute(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
			     size_t input_length, uint8_t *mac, size_t mac_size, size_t *mac_length)
{
	return ssf_psa_mac_compute(key, alg, input, input_length, mac, mac_size, mac_length);
}

psa_status_t psa_mac_verify(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
			    size_t input_length, const uint8_t *mac, size_t mac_length)
{
	return ssf_psa_mac_verify(key, alg, input, input_length, mac, mac_length);
}

psa_status_t psa_mac_sign_setup(psa_mac_operation_t *operation, mbedtls_svc_key_id_t key,
				psa_algorithm_t alg)
{
	return ssf_psa_mac_sign_setup(&operation->handle, key, alg);
}

psa_status_t psa_mac_verify_setup(psa_mac_operation_t *operation, mbedtls_svc_key_id_t key,
				  psa_algorithm_t alg)
{
	return ssf_psa_mac_verify_setup(&operation->handle, key, alg);
}

psa_status_t psa_mac_update(psa_mac_operation_t *operation, const uint8_t *input,
			    size_t input_length)
{
	return ssf_psa_mac_update(&operation->handle, input, input_length);
}

psa_status_t psa_mac_sign_finish(psa_mac_operation_t *operation, uint8_t *mac, size_t mac_size,
				 size_t *mac_length)
{
	return ssf_psa_mac_sign_finish(&operation->handle, mac, mac_size, mac_length);
}

psa_status_t psa_mac_verify_finish(psa_mac_operation_t *operation, const uint8_t *mac,
				   size_t mac_length)
{
	return ssf_psa_mac_verify_finish(&operation->handle, mac, mac_length);
}

psa_status_t psa_mac_abort(psa_mac_operation_t *operation)
{
	return ssf_psa_mac_abort(&operation->handle);
}

psa_status_t psa_cipher_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length)
{
	return ssf_psa_cipher_encrypt(key, alg, input, input_length, output, output_size,
				      output_length);
}

psa_status_t psa_cipher_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length)
{
	return ssf_psa_cipher_decrypt(key, alg, input, input_length, output, output_size,
				      output_length);
}

psa_status_t psa_cipher_encrypt_setup(psa_cipher_operation_t *operation, mbedtls_svc_key_id_t key,
				      psa_algorithm_t alg)
{
	return ssf_psa_cipher_encrypt_setup(&operation->handle, key, alg);
}

psa_status_t psa_cipher_decrypt_setup(psa_cipher_operation_t *operation, mbedtls_svc_key_id_t key,
				      psa_algorithm_t alg)
{
	return ssf_psa_cipher_decrypt_setup(&operation->handle, key, alg);
}

psa_status_t psa_cipher_generate_iv(psa_cipher_operation_t *operation, uint8_t *iv, size_t iv_size,
				    size_t *iv_length)
{
	return ssf_psa_cipher_generate_iv(&operation->handle, iv, iv_size, iv_length);
}

psa_status_t psa_cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv,
			       size_t iv_length)
{
	return ssf_psa_cipher_set_iv(&operation->handle, iv, iv_length);
}

psa_status_t psa_cipher_update(psa_cipher_operation_t *operation, const uint8_t *input,
			       size_t input_length, uint8_t *output, size_t output_size,
			       size_t *output_length)
{
	return ssf_psa_cipher_update(&operation->handle, input, input_length, output, output_size,
				     output_length);
}

psa_status_t psa_cipher_finish(psa_cipher_operation_t *operation, uint8_t *output,
			       size_t output_size, size_t *output_length)
{
	return ssf_psa_cipher_finish(&operation->handle, output, output_size, output_length);
}

psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation)
{
	return ssf_psa_cipher_abort(&operation->handle);
}

psa_status_t psa_aead_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
			      size_t nonce_length, const uint8_t *additional_data,
			      size_t additional_data_length, const uint8_t *plaintext,
			      size_t plaintext_length, uint8_t *ciphertext, size_t ciphertext_size,
			      size_t *ciphertext_length)
{
	return ssf_psa_aead_encrypt(key, alg, nonce, nonce_length, additional_data,
				    additional_data_length, plaintext, plaintext_length, ciphertext,
				    ciphertext_size, ciphertext_length);
}

psa_status_t psa_aead_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
			      size_t nonce_length, const uint8_t *additional_data,
			      size_t additional_data_length, const uint8_t *ciphertext,
			      size_t ciphertext_length, uint8_t *plaintext, size_t plaintext_size,
			      size_t *plaintext_length)
{
	return ssf_psa_aead_decrypt(key, alg, nonce, nonce_length, additional_data,
				    additional_data_length, ciphertext, ciphertext_length,
				    plaintext, plaintext_size, plaintext_length);
}

psa_status_t psa_aead_encrypt_setup(psa_aead_operation_t *operation, mbedtls_svc_key_id_t key,
				    psa_algorithm_t alg)
{
	return ssf_psa_aead_encrypt_setup(&operation->handle, key, alg);
}

psa_status_t psa_aead_decrypt_setup(psa_aead_operation_t *operation, mbedtls_svc_key_id_t key,
				    psa_algorithm_t alg)
{
	return ssf_psa_aead_decrypt_setup(&operation->handle, key, alg);
}

psa_status_t psa_aead_generate_nonce(psa_aead_operation_t *operation, uint8_t *nonce,
				     size_t nonce_size, size_t *nonce_length)
{
	return ssf_psa_aead_generate_nonce(&operation->handle, nonce, nonce_size, nonce_length);
}

psa_status_t psa_aead_set_nonce(psa_aead_operation_t *operation, const uint8_t *nonce,
				size_t nonce_length)
{
	return ssf_psa_aead_set_nonce(&operation->handle, nonce, nonce_length);
}

psa_status_t psa_aead_set_lengths(psa_aead_operation_t *operation, size_t ad_length,
				  size_t plaintext_length)
{
	return ssf_psa_aead_set_lengths(&operation->handle, ad_length, plaintext_length);
}

psa_status_t psa_aead_update_ad(psa_aead_operation_t *operation, const uint8_t *input,
				size_t input_length)
{
	return ssf_psa_aead_update_ad(&operation->handle, input, input_length);
}

psa_status_t psa_aead_update(psa_aead_operation_t *operation, const uint8_t *input,
			     size_t input_length, uint8_t *output, size_t output_size,
			     size_t *output_length)
{
	return ssf_psa_aead_update(&operation->handle, input, input_length, output, output_size,
				   output_length);
}

psa_status_t psa_aead_finish(psa_aead_operation_t *operation, uint8_t *ciphertext,
			     size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
			     size_t tag_size, size_t *tag_length)
{
	return ssf_psa_aead_finish(&operation->handle, ciphertext, ciphertext_size,
				   ciphertext_length, tag, tag_size, tag_length);
}

psa_status_t psa_aead_verify(psa_aead_operation_t *operation, uint8_t *plaintext,
			     size_t plaintext_size, size_t *plaintext_length, const uint8_t *tag,
			     size_t tag_length)
{
	return ssf_psa_aead_verify(&operation->handle, plaintext, plaintext_size, plaintext_length,
				   tag, tag_length);
}

psa_status_t psa_aead_abort(psa_aead_operation_t *operation)
{
	return ssf_psa_aead_abort(&operation->handle);
}

psa_status_t psa_sign_message(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
			      size_t input_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length)
{
	return ssf_psa_sign_message(key, alg, input, input_length, signature, signature_size,
				    signature_length);
}

psa_status_t psa_verify_message(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, const uint8_t *signature,
				size_t signature_length)
{
	return ssf_psa_verify_message(key, alg, input, input_length, signature, signature_length);
}

psa_status_t psa_sign_hash(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
			   size_t hash_length, uint8_t *signature, size_t signature_size,
			   size_t *signature_length)
{
	return ssf_psa_sign_hash(key, alg, hash, hash_length, signature, signature_size,
				 signature_length);
}

psa_status_t psa_verify_hash(mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
			     size_t hash_length, const uint8_t *signature, size_t signature_length)
{
	return ssf_psa_verify_hash(key, alg, hash, hash_length, signature, signature_length);
}

psa_status_t psa_asymmetric_encrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				    const uint8_t *input, size_t input_length, const uint8_t *salt,
				    size_t salt_length, uint8_t *output, size_t output_size,
				    size_t *output_length)
{
	return ssf_psa_asymmetric_encrypt(key, alg, input, input_length, salt, salt_length, output,
					  output_size, output_length);
}

psa_status_t psa_asymmetric_decrypt(mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				    const uint8_t *input, size_t input_length, const uint8_t *salt,
				    size_t salt_length, uint8_t *output, size_t output_size,
				    size_t *output_length)
{
	return ssf_psa_asymmetric_decrypt(key, alg, input, input_length, salt, salt_length, output,
					  output_size, output_length);
}

psa_status_t psa_key_derivation_setup(psa_key_derivation_operation_t *operation,
				      psa_algorithm_t alg)
{
	return ssf_psa_key_derivation_setup(&operation->handle, alg);
}

psa_status_t psa_key_derivation_get_capacity(const psa_key_derivation_operation_t *operation,
					     size_t *capacity)
{
	return ssf_psa_key_derivation_get_capacity(operation->handle, capacity);
}

psa_status_t psa_key_derivation_set_capacity(psa_key_derivation_operation_t *operation,
					     size_t capacity)
{
	return ssf_psa_key_derivation_set_capacity(&operation->handle, capacity);
}

psa_status_t psa_key_derivation_input_bytes(psa_key_derivation_operation_t *operation,
					    psa_key_derivation_step_t step, const uint8_t *data,
					    size_t data_length)
{
	return ssf_psa_key_derivation_input_bytes(&operation->handle, step, data, data_length);
}

psa_status_t psa_key_derivation_input_integer(psa_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step, uint64_t value)
{
	return ssf_psa_key_derivation_input_integer(&operation->handle, step, value);
}

psa_status_t psa_key_derivation_input_key(psa_key_derivation_operation_t *operation,
					  psa_key_derivation_step_t step, mbedtls_svc_key_id_t key)
{
	return ssf_psa_key_derivation_input_key(&operation->handle, step, key);
}

psa_status_t psa_key_derivation_key_agreement(psa_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step,
					      mbedtls_svc_key_id_t private_key,
					      const uint8_t *peer_key, size_t peer_key_length)
{
	return ssf_psa_key_derivation_key_agreement(&operation->handle, step, private_key, peer_key,
						    peer_key_length);
}

psa_status_t psa_key_derivation_output_bytes(psa_key_derivation_operation_t *operation,
					     uint8_t *output, size_t output_length)
{
	return ssf_psa_key_derivation_output_bytes(&operation->handle, output, output_length);
}

psa_status_t psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
					   psa_key_derivation_operation_t *operation,
					   mbedtls_svc_key_id_t *key)
{
	return ssf_psa_key_derivation_output_key(attributes, &operation->handle, key);
}

psa_status_t psa_key_derivation_abort(psa_key_derivation_operation_t *operation)
{
	return ssf_psa_key_derivation_abort(&operation->handle);
}

psa_status_t psa_raw_key_agreement(psa_algorithm_t alg, mbedtls_svc_key_id_t private_key,
				   const uint8_t *peer_key, size_t peer_key_length, uint8_t *output,
				   size_t output_size, size_t *output_length)
{
	return ssf_psa_raw_key_agreement(alg, private_key, peer_key, peer_key_length, output,
					 output_size, output_length);
}

psa_status_t psa_generate_random(uint8_t *output, size_t output_size)
{
	return ssf_psa_generate_random(output, output_size);
}

psa_status_t psa_generate_key(const psa_key_attributes_t *attributes, mbedtls_svc_key_id_t *key)
{
	return ssf_psa_generate_key(attributes, key);
}

psa_status_t psa_sign_hash_start(psa_sign_hash_interruptible_operation_t *operation,
				 mbedtls_svc_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
				 size_t hash_length)
{
	return ssf_psa_sign_hash_start(operation, key, alg, hash, hash_length);
}

psa_status_t psa_sign_hash_abort(psa_sign_hash_interruptible_operation_t *operation)
{
	return ssf_psa_sign_hash_abort(operation);
}

psa_status_t psa_verify_hash_start(psa_verify_hash_interruptible_operation_t *operation,
				   mbedtls_svc_key_id_t key, psa_algorithm_t alg,
				   const uint8_t *hash, size_t hash_length,
				   const uint8_t *signature, size_t signature_length)
{
	return ssf_psa_verify_hash_start(operation, key, alg, hash, hash_length, signature,
					 signature_length);
}

psa_status_t psa_verify_hash_abort(psa_verify_hash_interruptible_operation_t *operation)
{
	return ssf_psa_verify_hash_abort(operation);
}

psa_status_t psa_pake_setup(psa_pake_operation_t *operation, mbedtls_svc_key_id_t password_key,
			    const psa_pake_cipher_suite_t *cipher_suite)
{
	return ssf_psa_pake_setup(&operation->handle, password_key, cipher_suite);
}

psa_status_t psa_pake_set_role(psa_pake_operation_t *operation, psa_pake_role_t role)
{
	return ssf_psa_pake_set_role(&operation->handle, role);
}

psa_status_t psa_pake_set_user(psa_pake_operation_t *operation, const uint8_t *user_id,
			       size_t user_id_len)
{
	return ssf_psa_pake_set_user(&operation->handle, user_id, user_id_len);
}

psa_status_t psa_pake_set_peer(psa_pake_operation_t *operation, const uint8_t *peer_id,
			       size_t peer_id_len)
{
	return ssf_psa_pake_set_peer(&operation->handle, peer_id, peer_id_len);
}

psa_status_t psa_pake_set_context(psa_pake_operation_t *operation, const uint8_t *context,
				  size_t context_len)
{
	return ssf_psa_pake_set_context(&operation->handle, context, context_len);
}

psa_status_t psa_pake_output(psa_pake_operation_t *operation, psa_pake_step_t step, uint8_t *output,
			     size_t output_size, size_t *output_length)
{
	return ssf_psa_pake_output(&operation->handle, step, output, output_size, output_length);
}

psa_status_t psa_pake_input(psa_pake_operation_t *operation, psa_pake_step_t step,
			    const uint8_t *input, size_t input_length)
{
	return ssf_psa_pake_input(&operation->handle, step, input, input_length);
}

psa_status_t psa_pake_get_shared_key(psa_pake_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     mbedtls_svc_key_id_t *key)
{
	return ssf_psa_pake_get_shared_key(&operation->handle, attributes, key);
}

psa_status_t psa_pake_abort(psa_pake_operation_t *operation)
{
	return ssf_psa_pake_abort(&operation->handle);
}
