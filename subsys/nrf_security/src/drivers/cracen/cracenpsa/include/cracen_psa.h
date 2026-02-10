/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_driver_api
 * @{
 * @brief PSA Crypto driver API for CRACEN hardware accelerator.
 *
 * @note This is the public driver API. Applications must use the PSA Crypto API
 *       (psa_* functions) instead of calling these driver functions directly.
 *       This header documents the driver implementation that is called by the
 *       PSA Crypto layer. See Arm's PSA Crypto API for the public interface.
 *
 * This module provides the PSA Crypto driver API implementation for the
 * CRACEN hardware accelerator. It implements cryptographic operations
 * including signing, verification, hashing, encryption, decryption, key
 * management, and more.
 */

#ifndef CRACEN_PSA_H
#define CRACEN_PSA_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"
#include "cracen_psa_kmu.h"
#include "cracen_psa_key_ids.h"
#include "cracen_psa_ctr_drbg.h"
#include "sxsymcrypt/keyref.h"

/** @brief Get the size of an opaque key.
 *
 * @param[in] attributes Key attributes.
 * @param[out] key_size  Size of the key representation in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 */
psa_status_t cracen_get_opaque_size(const psa_key_attributes_t *attributes, size_t *key_size);

/** @brief Sign a message.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Signing algorithm.
 * @param[in] input           Message to sign.
 * @param[in] input_length    Length of the message in bytes.
 * @param[out] signature      Buffer to store the signature.
 * @param[in] signature_size  Size of the signature buffer in bytes.
 * @param[out] signature_length Length of the generated signature in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 */
psa_status_t cracen_sign_message(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				 size_t input_length, uint8_t *signature, size_t signature_size,
				 size_t *signature_length);

/** @brief Verify a message signature.
 *
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               Verification algorithm.
 * @param[in] input             Message that was signed.
 * @param[in] input_length      Length of the message in bytes.
 * @param[in] signature         Signature to verify.
 * @param[in] signature_length  Length of the signature in bytes.
 *
 * @retval PSA_SUCCESS              The signature is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The signature is invalid.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 */
psa_status_t cracen_verify_message(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   const uint8_t *signature, size_t signature_length);

/** @brief Sign a hash.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Signing algorithm.
 * @param[in] hash            Hash of a message to sign.
 * @param[in] hash_length     Length of the hash in bytes.
 * @param[out] signature      Buffer to store the signature.
 * @param[in] signature_size  Size of the signature buffer in bytes.
 * @param[out] signature_length Length of the generated signature in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 */
psa_status_t cracen_sign_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			      size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
			      size_t hash_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length);

/** @brief Verify the signature of a hash.
 *
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               Verification algorithm.
 * @param[in] hash              Hash of the message that was signed.
 * @param[in] hash_length       Length of the hash in bytes.
 * @param[in] signature         Signature to verify.
 * @param[in] signature_length  Length of the signature in bytes.
 *
 * @retval PSA_SUCCESS              The signature is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The signature is invalid.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 */
psa_status_t cracen_verify_hash(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *hash,
				size_t hash_length, const uint8_t *signature,
				size_t signature_length);

/** @brief Compute the hash of a message.
 *
 * @param[in] alg          Hash algorithm.
 * @param[in] input        Message to hash.
 * @param[in] input_length Length of the message in bytes.
 * @param[out] hash        Buffer to store the hash.
 * @param[in] hash_size    Size of the hash buffer in bytes.
 * @param[out] hash_length Length of the generated hash in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The hash buffer is too small.
 */
psa_status_t cracen_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				 uint8_t *hash, size_t hash_size, size_t *hash_length);

/** @brief Set up a hash operation.
 *
 * @param[in,out] operation Hash operation context.
 * @param[in] alg           Hash algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_hash_setup(cracen_hash_operation_t *operation, psa_algorithm_t alg);

/** @brief Clone a hash operation.
 *
 * @param[in] source_operation Source hash operation context.
 * @param[out] target_operation Target hash operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
static inline psa_status_t cracen_hash_clone(const cracen_hash_operation_t *source_operation,
					     cracen_hash_operation_t *target_operation)
{
	__ASSERT_NO_MSG(source_operation != NULL);
	__ASSERT_NO_MSG(target_operation != NULL);

	memcpy(target_operation, source_operation, sizeof(cracen_hash_operation_t));

	return PSA_SUCCESS;
}

/** @brief Add a message fragment to a multi-part hash operation.
 *
 * @param[in,out] operation  Hash operation context.
 * @param[in] input          Message fragment to hash.
 * @param[in] input_length   Length of the message fragment in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 */
psa_status_t cracen_hash_update(cracen_hash_operation_t *operation, const uint8_t *input,
				const size_t input_length);

/** @brief Finish a hash operation.
 *
 * @param[in,out] operation  Hash operation context.
 * @param[out] hash          Buffer to store the hash.
 * @param[in] hash_size      Size of the hash buffer in bytes.
 * @param[out] hash_length   Length of the generated hash in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The hash buffer is too small.
 */
psa_status_t cracen_hash_finish(cracen_hash_operation_t *operation, uint8_t *hash, size_t hash_size,
				size_t *hash_length);

/** @brief Abort a hash operation.
 *
 * @param[in,out] operation Hash operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
static inline psa_status_t cracen_hash_abort(cracen_hash_operation_t *operation)
{
	__ASSERT_NO_MSG(operation != NULL);

	safe_memzero(operation, sizeof(cracen_hash_operation_t));

	return PSA_SUCCESS;
}

/** @brief Encrypt a message using a symmetric cipher.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Cipher algorithm.
 * @param[in] iv              Initialization vector.
 * @param[in] iv_length       Length of the IV in bytes.
 * @param[in] input           Plaintext to encrypt.
 * @param[in] input_length    Length of the plaintext in bytes.
 * @param[out] output         Buffer to store the ciphertext.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the generated ciphertext in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_cipher_encrypt(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *iv, size_t iv_length,
				   const uint8_t *input, size_t input_length, uint8_t *output,
				   size_t output_size, size_t *output_length);

/** @brief Decrypt a message using a symmetric cipher.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Cipher algorithm.
 * @param[in] input           Ciphertext to decrypt.
 * @param[in] input_length    Length of the ciphertext in bytes.
 * @param[out] output         Buffer to store the plaintext.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the decrypted plaintext in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_cipher_decrypt(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				   uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Set up a cipher encryption operation.
 *
 * @param[in,out] operation      Cipher operation context.
 * @param[in] attributes         Key attributes.
 * @param[in] key_buffer         Key material buffer.
 * @param[in] key_buffer_size    Size of the key buffer in bytes.
 * @param[in] alg                Cipher algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_cipher_encrypt_setup(cracen_cipher_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg);

/** @brief Set up a cipher decryption operation.
 *
 * @param[in,out] operation      Cipher operation context.
 * @param[in] attributes         Key attributes.
 * @param[in] key_buffer         Key material buffer.
 * @param[in] key_buffer_size    Size of the key buffer in bytes.
 * @param[in] alg                Cipher algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_cipher_decrypt_setup(cracen_cipher_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *key_buffer, size_t key_buffer_size,
					 psa_algorithm_t alg);

/** @brief Set the initialization vector for a cipher operation.
 *
 * @param[in,out] operation Cipher operation context.
 * @param[in] iv            Initialization vector.
 * @param[in] iv_length     Length of the IV in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_cipher_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				  size_t iv_length);

/** @brief Process input data in a cipher operation.
 *
 * @param[in,out] operation   Cipher operation context.
 * @param[in] input           Input data to process.
 * @param[in] input_length    Length of the input data in bytes.
 * @param[out] output         Buffer to store the output.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_cipher_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				  size_t input_length, uint8_t *output, size_t output_size,
				  size_t *output_length);

/** @brief Finish a cipher operation.
 *
 * @param[in,out] operation   Cipher operation context.
 * @param[out] output         Buffer to store the final output.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_cipher_finish(cracen_cipher_operation_t *operation, uint8_t *output,
				  size_t output_size, size_t *output_length);

/** @brief Abort a cipher operation.
 *
 * @param[in,out] operation Cipher operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_cipher_abort(cracen_cipher_operation_t *operation);

/** @brief Encrypt and authenticate a message using AEAD.
 *
 * @param[in] attributes             Key attributes.
 * @param[in] key_buffer             Key material buffer.
 * @param[in] key_buffer_size        Size of the key buffer in bytes.
 * @param[in] alg                    AEAD algorithm.
 * @param[in] nonce                  Nonce or IV.
 * @param[in] nonce_length           Length of the nonce in bytes.
 * @param[in] additional_data        Additional Authenticated Data (AAD).
 * @param[in] additional_data_length Length of Additional Authenticated Data (AAD) in bytes.
 * @param[in] plaintext              Plaintext to encrypt.
 * @param[in] plaintext_length       Length of the plaintext in bytes.
 * @param[out] ciphertext            Buffer to store the ciphertext and tag.
 * @param[in] ciphertext_size        Size of the ciphertext buffer in bytes.
 * @param[out] ciphertext_length     Length of the generated ciphertext in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE   The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED    The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The ciphertext buffer is too small.
 */
psa_status_t cracen_aead_encrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *plaintext,
				 size_t plaintext_length, uint8_t *ciphertext,
				 size_t ciphertext_size, size_t *ciphertext_length);

/** @brief Authenticate and decrypt a message using AEAD.
 *
 * @param[in] attributes             Key attributes.
 * @param[in] key_buffer             Key material buffer.
 * @param[in] key_buffer_size        Size of the key buffer in bytes.
 * @param[in] alg                    AEAD algorithm.
 * @param[in] nonce                  Nonce or IV.
 * @param[in] nonce_length           Length of the nonce in bytes.
 * @param[in] additional_data        Additional Authenticated Data (AAD).
 * @param[in] additional_data_length Length of Additional Authenticated Data (AAD) in bytes.
 * @param[in] ciphertext             Ciphertext and tag to decrypt.
 * @param[in] ciphertext_length      Length of the ciphertext in bytes.
 * @param[out] plaintext             Buffer to store the plaintext.
 * @param[in] plaintext_size         Size of the plaintext buffer in bytes.
 * @param[out] plaintext_length      Length of the decrypted plaintext in bytes.
 *
 * @retval PSA_SUCCESS                 The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE    The key handle is invalid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The authentication tag is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED     The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL  The plaintext buffer is too small.
 */
psa_status_t cracen_aead_decrypt(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				 size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *nonce,
				 size_t nonce_length, const uint8_t *additional_data,
				 size_t additional_data_length, const uint8_t *ciphertext,
				 size_t ciphertext_length, uint8_t *plaintext,
				 size_t plaintext_size, size_t *plaintext_length);

/** @brief Set up an AEAD encryption operation.
 *
 * @param[in,out] operation     AEAD operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               AEAD algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_aead_encrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg);

/** @brief Set up an AEAD decryption operation.
 *
 * @param[in,out] operation     AEAD operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               AEAD algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_aead_decrypt_setup(cracen_aead_operation_t *operation,
				       const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg);

/** @brief Set the nonce for an AEAD operation.
 *
 * @param[in,out] operation   AEAD operation context.
 * @param[in] nonce           Nonce or IV.
 * @param[in] nonce_length    Length of the nonce in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_aead_set_nonce(cracen_aead_operation_t *operation, const uint8_t *nonce,
				   size_t nonce_length);

/** @brief Set the lengths for an AEAD operation.
 *
 * @param[in,out] operation      AEAD operation context.
 * @param[in] ad_length          Length of Additional Authenticated Data (AAD) in bytes.
 * @param[in] plaintext_length   Length of the plaintext in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_aead_set_lengths(cracen_aead_operation_t *operation, size_t ad_length,
				     size_t plaintext_length);

/** @brief Add Additional Authenticated Data (AAD) to an AEAD operation.
 *
 * @param[in,out] operation  AEAD operation context.
 * @param[in] input          Additional Authenticated Data (AAD).
 * @param[in] input_length   Length of the Additional Authenticated Data (AAD) in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_aead_update_ad(cracen_aead_operation_t *operation, const uint8_t *input,
				   size_t input_length);

/** @brief Process input data in an AEAD operation.
 *
 * @param[in,out] operation   AEAD operation context.
 * @param[in] input           Input data to process.
 * @param[in] input_length    Length of the input data in bytes.
 * @param[out] output         Buffer to store the output.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_aead_update(cracen_aead_operation_t *operation, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length);

/** @brief Finish an AEAD encryption operation.
 *
 * @param[in,out] operation       AEAD operation context.
 * @param[out] ciphertext         Buffer to store the final ciphertext.
 * @param[in] ciphertext_size     Size of the ciphertext buffer in bytes.
 * @param[out] ciphertext_length  Length of the generated ciphertext in bytes.
 * @param[out] tag                Buffer to store the authentication tag.
 * @param[in] tag_size            Size of the tag buffer in bytes.
 * @param[out] tag_length         Length of the generated tag in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL A buffer is too small.
 */
psa_status_t cracen_aead_finish(cracen_aead_operation_t *operation, uint8_t *ciphertext,
				size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
				size_t tag_size, size_t *tag_length);

/** @brief Finish an AEAD decryption operation and verify the tag.
 *
 * @param[in,out] operation      AEAD operation context.
 * @param[out] plaintext         Buffer to store the final plaintext.
 * @param[in] plaintext_size     Size of the plaintext buffer in bytes.
 * @param[out] plaintext_length  Length of the generated plaintext in bytes.
 * @param[in] tag                Authentication tag to verify.
 * @param[in] tag_length         Length of the tag in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully and the tag is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The authentication tag is invalid.
 * @retval PSA_ERROR_BAD_STATE      The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The plaintext buffer is too small.
 */
psa_status_t cracen_aead_verify(cracen_aead_operation_t *operation, uint8_t *plaintext,
				size_t plaintext_size, size_t *plaintext_length, const uint8_t *tag,
				size_t tag_length);

/** @brief Abort an AEAD operation.
 *
 * @param[in,out] operation AEAD operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_aead_abort(cracen_aead_operation_t *operation);

/** @brief Compute a MAC in a single operation.
 *
 * @param[in] attributes     Key attributes.
 * @param[in] key_buffer     Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg            MAC algorithm.
 * @param[in] input          Input data to authenticate.
 * @param[in] input_length   Length of the input data in bytes.
 * @param[out] mac           Buffer to store the MAC.
 * @param[in] mac_size       Size of the MAC buffer in bytes.
 * @param[out] mac_length    Length of the generated MAC in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The MAC buffer is too small.
 */
psa_status_t cracen_mac_compute(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
				size_t key_buffer_size, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *mac, size_t mac_size,
				size_t *mac_length);

/** @brief Set up a MAC signing operation.
 *
 * @param[in,out] operation     MAC operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               MAC algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_mac_sign_setup(cracen_mac_operation_t *operation,
				   const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg);

/** @brief Set up a MAC verification operation.
 *
 * @param[in,out] operation     MAC operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 * @param[in] alg               MAC algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_mac_verify_setup(cracen_mac_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg);

/** @brief Add input data to a MAC operation.
 *
 * @param[in,out] operation  MAC operation context.
 * @param[in] input          Input data to add.
 * @param[in] input_length   Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_mac_update(cracen_mac_operation_t *operation, const uint8_t *input,
			       size_t input_length);

/** @brief Finish a MAC signing operation.
 *
 * @param[in,out] operation MAC operation context.
 * @param[out] mac          Buffer to store the MAC.
 * @param[in] mac_size      Size of the MAC buffer in bytes.
 * @param[out] mac_length   Length of the generated MAC in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The MAC buffer is too small.
 */
psa_status_t cracen_mac_sign_finish(cracen_mac_operation_t *operation, uint8_t *mac,
				    size_t mac_size, size_t *mac_length);

/** @brief Finish a MAC verification operation.
 *
 * @param[in,out] operation MAC operation context.
 * @param[in] mac           MAC to verify.
 * @param[in] mac_length    Length of the MAC in bytes.
 *
 * @retval PSA_SUCCESS              The MAC is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The MAC is invalid.
 * @retval PSA_ERROR_BAD_STATE      The operation is not in a valid state.
 */
psa_status_t cracen_mac_verify_finish(cracen_mac_operation_t *operation, const uint8_t *mac,
				      size_t mac_length);

/** @brief Abort a MAC operation.
 *
 * @param[in,out] operation MAC operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_mac_abort(cracen_mac_operation_t *operation);

/** @brief Perform a key agreement operation.
 *
 * @param[in] attributes     Key attributes for the private key.
 * @param[in] priv_key       Private key material.
 * @param[in] priv_key_size  Size of the private key in bytes.
 * @param[in] publ_key       Public key material.
 * @param[in] publ_key_size  Size of the public key in bytes.
 * @param[out] output        Buffer to store the shared secret.
 * @param[in] output_size    Size of the output buffer in bytes.
 * @param[out] output_length Length of the generated shared secret in bytes.
 * @param[in] alg            Key agreement algorithm.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_key_agreement(const psa_key_attributes_t *attributes, const uint8_t *priv_key,
				  size_t priv_key_size, const uint8_t *publ_key,
				  size_t publ_key_size, uint8_t *output, size_t output_size,
				  size_t *output_length, psa_algorithm_t alg);

/** @brief Set up a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 * @param[in] alg           Key derivation algorithm.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_key_derivation_setup(cracen_key_derivation_operation_t *operation,
					 psa_algorithm_t alg);

/** @brief Set the capacity for a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 * @param[in] capacity      Maximum capacity in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_set_capacity(cracen_key_derivation_operation_t *operation,
						size_t capacity);

/** @brief Provide a key as input to a key derivation operation.
 *
 * @param[in,out] operation     Key derivation operation context.
 * @param[in] step              Derivation step.
 * @param[in] attributes        Key attributes.
 * @param[in] key_buffer        Key material buffer.
 * @param[in] key_buffer_size   Size of the key buffer in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_input_key(cracen_key_derivation_operation_t *operation,
					     psa_key_derivation_step_t step,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size);

/** @brief Provide bytes as input to a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 * @param[in] step          Derivation step.
 * @param[in] data          Input data.
 * @param[in] data_length   Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_input_bytes(cracen_key_derivation_operation_t *operation,
					       psa_key_derivation_step_t step, const uint8_t *data,
					       size_t data_length);

/** @brief Provide an integer as input to a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 * @param[in] step          Derivation step.
 * @param[in] value         Integer value.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_input_integer(cracen_key_derivation_operation_t *operation,
						 psa_key_derivation_step_t step, uint64_t value);

/** @brief Read output from a key derivation operation.
 *
 * @param[in,out] operation    Key derivation operation context.
 * @param[out] output          Buffer to store the output.
 * @param[in] output_length    Length of the output to read in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_key_derivation_output_bytes(cracen_key_derivation_operation_t *operation,
						uint8_t *output, size_t output_length);

/** @brief Abort a key derivation operation.
 *
 * @param[in,out] operation Key derivation operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_key_derivation_abort(cracen_key_derivation_operation_t *operation);

/** @brief Encrypt a message using an asymmetric key.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Encryption algorithm.
 * @param[in] input           Plaintext to encrypt.
 * @param[in] input_length    Length of the plaintext in bytes.
 * @param[in] salt            Optional salt.
 * @param[in] salt_length     Length of the salt in bytes.
 * @param[out] output         Buffer to store the ciphertext.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the generated ciphertext in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_asymmetric_encrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Decrypt a message using an asymmetric key.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[in] alg             Decryption algorithm.
 * @param[in] input           Ciphertext to decrypt.
 * @param[in] input_length    Length of the ciphertext in bytes.
 * @param[in] salt            Optional salt.
 * @param[in] salt_length     Length of the salt in bytes.
 * @param[out] output         Buffer to store the plaintext.
 * @param[in] output_size     Size of the output buffer in bytes.
 * @param[out] output_length  Length of the decrypted plaintext in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED  The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_asymmetric_decrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *input,
				       size_t input_length, const uint8_t *salt, size_t salt_length,
				       uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Export a public key.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[out] data           Buffer to store the public key.
 * @param[in] data_size       Size of the data buffer in bytes.
 * @param[out] data_length    Length of the exported public key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The data buffer is too small.
 */
psa_status_t cracen_export_public_key(const psa_key_attributes_t *attributes,
				      const uint8_t *key_buffer, size_t key_buffer_size,
				      uint8_t *data, size_t data_size, size_t *data_length);

/** @brief Import a key.
 *
 * @param[in] attributes          Key attributes.
 * @param[in] data                Key data to import.
 * @param[in] data_length         Length of the key data in bytes.
 * @param[out] key_buffer         Buffer to represent the stored key.
 * @param[in] key_buffer_size     Size of the key buffer in bytes.
 * @param[out] key_buffer_length  Length of key buffer for the imported key in bytes.
 * @param[out] key_bits           Size of the key in bits.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT The key data is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t cracen_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			       size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
			       size_t *key_buffer_length, size_t *key_bits);

/** @brief Generate a key.
 *
 * @param[in] attributes          Key attributes.
 * @param[out] key_buffer         Buffer to represent the generated key.
 * @param[in] key_buffer_size     Size of the key buffer in bytes.
 * @param[out] key_buffer_length  Length of key buffer for the generated key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED  The key type or size is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t cracen_generate_key(const psa_key_attributes_t *attributes, uint8_t *key_buffer,
				 size_t key_buffer_size, size_t *key_buffer_length);

/** @brief Get a built-in key.
 *
 * @param[in] slot_number         Key slot number.
 * @param[out] attributes         Key attributes.
 * @param[out] key_buffer         Buffer representing the built-in key.
 * @param[in] key_buffer_size     Size of the key buffer in bytes.
 * @param[out] key_buffer_length  Length of key buffer for the built-in key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_DOES_NOT_EXIST The key does not exist.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t cracen_get_builtin_key(psa_drv_slot_number_t slot_number,
				    psa_key_attributes_t *attributes, uint8_t *key_buffer,
				    size_t key_buffer_size, size_t *key_buffer_length);

/** @brief Export a key.
 *
 * @param[in] attributes      Key attributes.
 * @param[in] key_buffer      Key material buffer.
 * @param[in] key_buffer_size Size of the key buffer in bytes.
 * @param[out] data           Buffer to store the exported key.
 * @param[in] data_size       Size of the data buffer in bytes.
 * @param[out] data_length    Length of the exported key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The data buffer is too small.
 */
psa_status_t cracen_export_key(const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			       size_t key_buffer_size, uint8_t *data, size_t data_size,
			       size_t *data_length);

/** @brief Copy a key.
 *
 * @param[in,out] attributes             Key attributes for the target key.
 * @param[in] source_key                 Source key material.
 * @param[in] source_key_length          Length of the source key in bytes.
 * @param[out] target_key_buffer         Buffer to store the copied key.
 * @param[in] target_key_buffer_size     Size of the target key buffer in bytes.
 * @param[out] target_key_buffer_length  Length of the target key buffer for the copied key
 *                                       in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The target key buffer is too small.
 */
psa_status_t cracen_copy_key(psa_key_attributes_t *attributes, const uint8_t *source_key,
			     size_t source_key_length, uint8_t *target_key_buffer,
			     size_t target_key_buffer_size, size_t *target_key_buffer_length);

/** @brief Destroy a key.
 *
 * @param[in] attributes Key attributes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 */
psa_status_t cracen_destroy_key(const psa_key_attributes_t *attributes);

/** @brief Derive a key from input material.
 *
 * @param[in] attributes      Key attributes for the derived key.
 * @param[in] input           Input material.
 * @param[in] input_length    Length of the input material in bytes.
 * @param[out] key            Buffer to store the derived key.
 * @param[in] key_size        Size of the key buffer in bytes.
 * @param[out] key_length     Length of the derived key in bytes.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED  The key derivation is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t cracen_derive_key(const psa_key_attributes_t *attributes, const uint8_t *input,
			       size_t input_length, uint8_t *key, size_t key_size,
			       size_t *key_length);

/** @brief Get the key slot for a given key ID.
 *
 * @param[in] key_id       Key ID.
 * @param[out] lifetime    Key lifetime.
 * @param[out] slot_number Key slot number.
 *
 * @retval PSA_SUCCESS              The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_DOES_NOT_EXIST The key does not exist.
 */
psa_status_t cracen_get_key_slot(mbedtls_svc_key_id_t key_id, psa_key_lifetime_t *lifetime,
				 psa_drv_slot_number_t *slot_number);

/** @brief Set up a JPAKE operation.
 *
 * @param[in,out] operation     JPAKE operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 * @param[in] cipher_suite      PAKE cipher suite.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_jpake_setup(cracen_jpake_operation_t *operation,
				const psa_key_attributes_t *attributes, const uint8_t *password,
				size_t password_length,
				const psa_pake_cipher_suite_t *cipher_suite);

/** @brief Set the password key for a JPAKE operation.
 *
 * @param[in,out] operation     JPAKE operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_set_password_key(cracen_jpake_operation_t *operation,
					   const psa_key_attributes_t *attributes,
					   const uint8_t *password, size_t password_length);

/** @brief Set the user identifier for a JPAKE operation.
 *
 * @param[in,out] operation JPAKE operation context.
 * @param[in] user_id       User identifier.
 * @param[in] user_id_len   Length of the user identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_set_user(cracen_jpake_operation_t *operation, const uint8_t *user_id,
				   size_t user_id_len);

/** @brief Set the peer identifier for a JPAKE operation.
 *
 * @param[in,out] operation JPAKE operation context.
 * @param[in] peer_id       Peer identifier.
 * @param[in] peer_id_len   Length of the peer identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_set_peer(cracen_jpake_operation_t *operation, const uint8_t *peer_id,
				   size_t peer_id_len);

/** @brief Set the role for a JPAKE operation.
 *
 * @param[in,out] operation JPAKE operation context.
 * @param[in] role          PAKE role.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_set_role(cracen_jpake_operation_t *operation, psa_pake_role_t role);

/** @brief Get output from a JPAKE operation step.
 *
 * @param[in,out] operation      JPAKE operation context.
 * @param[in] step               PAKE step.
 * @param[out] output            Buffer to store the output.
 * @param[in] output_size        Size of the output buffer in bytes.
 * @param[out] output_length     Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_jpake_output(cracen_jpake_operation_t *operation, psa_pake_step_t step,
				 uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Provide input to a JPAKE operation step.
 *
 * @param[in,out] operation   JPAKE operation context.
 * @param[in] step            PAKE step.
 * @param[in] input           Input data.
 * @param[in] input_length    Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_jpake_input(cracen_jpake_operation_t *operation, psa_pake_step_t step,
				const uint8_t *input, size_t input_length);

/** @brief Get the shared key from a completed JPAKE operation.
 *
 * @param[in,out] operation     JPAKE operation context.
 * @param[in] attributes        Key attributes for the shared key.
 * @param[out] output           Buffer to store the shared key.
 * @param[in] output_size       Size of the output buffer in bytes.
 * @param[out] output_length    Length of the generated shared key in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_jpake_get_shared_key(cracen_jpake_operation_t *operation,
					 const psa_key_attributes_t *attributes, uint8_t *output,
					 size_t output_size, size_t *output_length);

/** @brief Abort a JPAKE operation.
 *
 * @param[in,out] operation JPAKE operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_jpake_abort(cracen_jpake_operation_t *operation);

/** @brief Set up an SRP operation.
 *
 * @param[in,out] operation     SRP operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 * @param[in] cipher_suite      PAKE cipher suite.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_srp_setup(cracen_srp_operation_t *operation,
			      const psa_key_attributes_t *attributes, const uint8_t *password,
			      size_t password_length, const psa_pake_cipher_suite_t *cipher_suite);

/** @brief Set the user identifier for an SRP operation.
 *
 * @param[in,out] operation SRP operation context.
 * @param[in] user_id       User identifier.
 * @param[in] user_id_len   Length of the user identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_srp_set_user(cracen_srp_operation_t *operation, const uint8_t *user_id,
				 size_t user_id_len);

/** @brief Set the role for an SRP operation.
 *
 * @param[in,out] operation SRP operation context.
 * @param[in] role          PAKE role.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_srp_set_role(cracen_srp_operation_t *operation, psa_pake_role_t role);

/** @brief Set the password key for an SRP operation.
 *
 * @param[in,out] operation     SRP operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_srp_set_password_key(cracen_srp_operation_t *operation,
					 const psa_key_attributes_t *attributes,
					 const uint8_t *password, size_t password_length);

/** @brief Get output from an SRP operation step.
 *
 * @param[in,out] operation      SRP operation context.
 * @param[in] step               PAKE step.
 * @param[out] output            Buffer to store the output.
 * @param[in] output_size        Size of the output buffer in bytes.
 * @param[out] output_length     Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_srp_output(cracen_srp_operation_t *operation, psa_pake_step_t step,
			       uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Provide input to an SRP operation step.
 *
 * @param[in,out] operation   SRP operation context.
 * @param[in] step            PAKE step.
 * @param[in] input           Input data.
 * @param[in] input_length    Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_srp_input(cracen_srp_operation_t *operation, psa_pake_step_t step,
			      const uint8_t *input, size_t input_length);

/** @brief Get the shared key from a completed SRP operation.
 *
 * @param[in,out] operation     SRP operation context.
 * @param[in] attributes        Key attributes for the shared key.
 * @param[out] output           Buffer to store the shared key.
 * @param[in] output_size       Size of the output buffer in bytes.
 * @param[out] output_length    Length of the generated shared key in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_srp_get_shared_key(cracen_srp_operation_t *operation,
				       const psa_key_attributes_t *attributes, uint8_t *output,
				       size_t output_size, size_t *output_length);

/** @brief Abort an SRP operation.
 *
 * @param[in,out] operation SRP operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_srp_abort(cracen_srp_operation_t *operation);

/** @brief Set up a PAKE operation.
 *
 * @param[in,out] operation     PAKE operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 * @param[in] cipher_suite      PAKE cipher suite.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_pake_setup(cracen_pake_operation_t *operation,
			       const psa_key_attributes_t *attributes, const uint8_t *password,
			       size_t password_length, const psa_pake_cipher_suite_t *cipher_suite);

/** @brief Set the user identifier for a PAKE operation.
 *
 * @param[in,out] operation PAKE operation context.
 * @param[in] user_id       User identifier.
 * @param[in] user_id_len   Length of the user identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_pake_set_user(cracen_pake_operation_t *operation, const uint8_t *user_id,
				  size_t user_id_len);

/** @brief Set the peer identifier for a PAKE operation.
 *
 * @param[in,out] operation PAKE operation context.
 * @param[in] peer_id       Peer identifier.
 * @param[in] peer_id_len   Length of the peer identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_pake_set_peer(cracen_pake_operation_t *operation, const uint8_t *peer_id,
				  size_t peer_id_len);

/** @brief Set the context for a PAKE operation.
 *
 * @param[in,out] operation    PAKE operation context.
 * @param[in] context          Context data.
 * @param[in] context_length   Length of the context data in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_pake_set_context(cracen_pake_operation_t *operation, const uint8_t *context,
				     size_t context_length);

/** @brief Set the role for a PAKE operation.
 *
 * @param[in,out] operation PAKE operation context.
 * @param[in] role          PAKE role.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_pake_set_role(cracen_pake_operation_t *operation, psa_pake_role_t role);

/** @brief Get output from a PAKE operation step.
 *
 * @param[in,out] operation      PAKE operation context.
 * @param[in] step               PAKE step.
 * @param[out] output            Buffer to store the output.
 * @param[in] output_size        Size of the output buffer in bytes.
 * @param[out] output_length     Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_pake_output(cracen_pake_operation_t *operation, psa_pake_step_t step,
				uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Provide input to a PAKE operation step.
 *
 * @param[in,out] operation   PAKE operation context.
 * @param[in] step            PAKE step.
 * @param[in] input           Input data.
 * @param[in] input_length    Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_pake_input(cracen_pake_operation_t *operation, psa_pake_step_t step,
			       const uint8_t *input, size_t input_length);

/** @brief Get the shared key from a completed PAKE operation.
 *
 * @param[in,out] operation      PAKE operation context.
 * @param[in] attributes         Key attributes for the shared key.
 * @param[out] key_buffer        Buffer to store the shared key.
 * @param[in] key_buffer_size    Size of the key buffer in bytes.
 * @param[out] key_buffer_length Length of the generated shared key in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The key buffer is too small.
 */
psa_status_t cracen_pake_get_shared_key(cracen_pake_operation_t *operation,
					const psa_key_attributes_t *attributes, uint8_t *key_buffer,
					size_t key_buffer_size, size_t *key_buffer_length);

/** @brief Abort a PAKE operation.
 *
 * @param[in,out] operation PAKE operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_pake_abort(cracen_pake_operation_t *operation);

/** @brief Set up a SPAKE2+ operation.
 *
 * @param[in,out] operation     SPAKE2+ operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 * @param[in] cipher_suite      PAKE cipher suite.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 */
psa_status_t cracen_spake2p_setup(cracen_spake2p_operation_t *operation,
				  const psa_key_attributes_t *attributes, const uint8_t *password,
				  size_t password_length,
				  const psa_pake_cipher_suite_t *cipher_suite);

/** @brief Set the password key for a SPAKE2+ operation.
 *
 * @param[in,out] operation     SPAKE2+ operation context.
 * @param[in] attributes        Key attributes.
 * @param[in] password          Password.
 * @param[in] password_length   Length of the password in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_password_key(cracen_spake2p_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *password, size_t password_length);

/** @brief Set the user identifier for a SPAKE2+ operation.
 *
 * @param[in,out] operation SPAKE2+ operation context.
 * @param[in] user_id       User identifier.
 * @param[in] user_id_len   Length of the user identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_user(cracen_spake2p_operation_t *operation, const uint8_t *user_id,
				     size_t user_id_len);

/** @brief Set the peer identifier for a SPAKE2+ operation.
 *
 * @param[in,out] operation SPAKE2+ operation context.
 * @param[in] peer_id       Peer identifier.
 * @param[in] peer_id_len   Length of the peer identifier in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_peer(cracen_spake2p_operation_t *operation, const uint8_t *peer_id,
				     size_t peer_id_len);

/** @brief Set the context for a SPAKE2+ operation.
 *
 * @param[in,out] operation    SPAKE2+ operation context.
 * @param[in] context          Context data.
 * @param[in] context_length   Length of the context data in bytes.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_context(cracen_spake2p_operation_t *operation,
					const uint8_t *context, size_t context_length);

/** @brief Set the role for a SPAKE2+ operation.
 *
 * @param[in,out] operation SPAKE2+ operation context.
 * @param[in] role         PAKE role.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_set_role(cracen_spake2p_operation_t *operation, psa_pake_role_t role);

/** @brief Get output from a SPAKE2+ operation step.
 *
 * @param[in,out] operation      SPAKE2+ operation context.
 * @param[in] step               PAKE step.
 * @param[out] output            Buffer to store the output.
 * @param[in] output_size        Size of the output buffer in bytes.
 * @param[out] output_length     Length of the generated output in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_spake2p_output(cracen_spake2p_operation_t *operation, psa_pake_step_t step,
				   uint8_t *output, size_t output_size, size_t *output_length);

/** @brief Provide input to a SPAKE2+ operation step.
 *
 * @param[in,out] operation   SPAKE2+ operation context.
 * @param[in] step            PAKE step.
 * @param[in] input           Input data.
 * @param[in] input_length    Length of the input data in bytes.
 *
 * @retval PSA_SUCCESS         The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not in a valid state.
 */
psa_status_t cracen_spake2p_input(cracen_spake2p_operation_t *operation, psa_pake_step_t step,
				  const uint8_t *input, size_t input_length);

/** @brief Get the shared key from a completed SPAKE2+ operation.
 *
 * @param[in,out] operation     SPAKE2+ operation context.
 * @param[in] attributes        Key attributes for the shared key.
 * @param[out] output           Buffer to store the shared key.
 * @param[in] output_size       Size of the output buffer in bytes.
 * @param[out] output_length    Length of the generated shared key in bytes.
 *
 * @retval PSA_SUCCESS             The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE     The operation is not in a valid state.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 */
psa_status_t cracen_spake2p_get_shared_key(cracen_spake2p_operation_t *operation,
					   const psa_key_attributes_t *attributes, uint8_t *output,
					   size_t output_size, size_t *output_length);

/** @brief Abort a SPAKE2+ operation.
 *
 * @param[in,out] operation SPAKE2+ operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_spake2p_abort(cracen_spake2p_operation_t *operation);

/** @} */

#endif /* CRACEN_PSA_H */
