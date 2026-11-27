/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @addtogroup cracen_psa_driver_api
 * @{
 * @brief Symmetric cipher operations for the CRACEN PSA driver.
 */

#ifndef CRACEN_PSA_CIPHER_H
#define CRACEN_PSA_CIPHER_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cracen_psa_primitives.h"

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
 * @retval PSA_SUCCESS The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT The key is too large for the operation key buffer, the IV
 *     length is invalid for the algorithm, or @p input_length is not a multiple of the block size
 *     for CBC without padding.
 *
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
 * @retval PSA_SUCCESS The operation completed successfully.
 * @retval PSA_ERROR_INVALID_HANDLE The key handle is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT The key is too large for the operation key buffer,
 *     @p input_length is shorter than the IV, or the ciphertext length is not a multiple of the
 *     block size for CBC without padding.
 *
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
 * @retval PSA_SUCCESS The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 * @retval PSA_ERROR_INVALID_ARGUMENT The key is too large for the operation key buffer.
 *
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
 * @retval PSA_SUCCESS The operation completed successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED The algorithm is not supported.
 * @retval PSA_ERROR_INVALID_ARGUMENT The key is too large for the operation key buffer.
 *
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
 * @retval PSA_SUCCESS The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not initialized.
 * @retval PSA_ERROR_INVALID_ARGUMENT The IV length is invalid for the algorithm.
 * @retval PSA_ERROR_NOT_SUPPORTED The ChaCha20 nonce length is 8 or 16 bytes instead of 12.
 *
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
 * @retval PSA_ERROR_BAD_STATE     The operation is not initialized.
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
 * @retval PSA_SUCCESS The operation completed successfully.
 * @retval PSA_ERROR_BAD_STATE The operation is not initialized.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The output buffer is too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT The buffered input is not a whole block for CBC decryption
 *     with PKCS#7 padding, or the total input is not a multiple of the block size for CBC
 *     without padding.
 *
 */
psa_status_t cracen_cipher_finish(cracen_cipher_operation_t *operation, uint8_t *output,
				  size_t output_size, size_t *output_length);

/** @brief Abort a cipher operation.
 *
 * @param[in,out] operation Cipher operation context.
 *
 * @retval PSA_SUCCESS The operation has been aborted successfully.
 *
 */
psa_status_t cracen_cipher_abort(cracen_cipher_operation_t *operation);

/** @} */

#endif /* CRACEN_PSA_CIPHER_H */
