/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file cracen_sw_chacha20_poly1305.h
 *
 * @defgroup cracen_sw_chacha_poly Software ChaCha20-Poly1305 implementation
 * @{
 * @brief Software implementation of the ChaCha20-Poly1305 AEAD.
 *
 * This module provides a software-based implementation of ChaCha20-Poly1305 for use
 * as a workaround for hardware limitations with multipart operations.
 */

#ifndef CRACEN_SW_CHACHA_POLY_H
#define CRACEN_SW_CHACHA_POLY_H

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <stdbool.h>
#include <stdint.h>
#include "cracen_psa_primitives.h"

/**
 * @brief Set up the software ChaCha20-Poly1305 encryption operation.
 *
 * @param[in,out] operation       Pointer to the AEAD operation structure.
 * @param[in]     attributes      Key attributes.
 * @param[in]     key_buffer      Key material.
 * @param[in]     key_buffer_size Size of the key material. Max CRACEN_MAX_AEAD_KEY_SIZE bytes.
 * @param[in]     alg             Algorithm to use.
 *
 * @retval PSA_SUCCESS Operation setup successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm or key type.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid key size.
 */
psa_status_t cracen_sw_chacha20_poly1305_encrypt_setup(cracen_aead_operation_t *operation,
						       const psa_key_attributes_t *attributes,
						       const uint8_t *key_buffer,
						       size_t key_buffer_size,
						       psa_algorithm_t alg);

/**
 * @brief Set up the software ChaCha20-Poly1305 decryption operation.
 *
 * @param[in,out] operation       Pointer to the AEAD operation structure.
 * @param[in]     attributes      Key attributes.
 * @param[in]     key_buffer      Key material.
 * @param[in]     key_buffer_size Size of the key material. Max CRACEN_MAX_AEAD_KEY_SIZE bytes.
 * @param[in]     alg             Algorithm to use.
 *
 * @retval PSA_SUCCESS Operation setup successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm or key type.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid key size.
 */
psa_status_t cracen_sw_chacha20_poly1305_decrypt_setup(cracen_aead_operation_t *operation,
						       const psa_key_attributes_t *attributes,
						       const uint8_t *key_buffer,
						       size_t key_buffer_size,
						       psa_algorithm_t alg);

/**
 * @brief Set nonce for the software ChaCha20-Poly1305 operation.
 *
 * @param[in,out] operation    Pointer to the AEAD operation structure.
 * @param[in]     nonce        Nonce value.
 * @param[in]     nonce_length Length of the nonce. Must be 12 bytes for ChaCha20-Poly1305.
 *
 * @retval PSA_SUCCESS Nonce set successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid nonce length.
 */
psa_status_t cracen_sw_chacha20_poly1305_set_nonce(cracen_aead_operation_t *operation,
						   const uint8_t *nonce,
						   size_t nonce_length);

/**
 * @brief Set lengths for the software ChaCha20-Poly1305 operation.
 *
 * @param[in,out] operation        Pointer to the AEAD operation structure.
 * @param[in]     ad_length        Length of additional data.
 * @param[in]     plaintext_length Length of plaintext.
 *
 * @retval PSA_SUCCESS Lengths set successfully.
 */
psa_status_t cracen_sw_chacha20_poly1305_set_lengths(cracen_aead_operation_t *operation,
						     size_t ad_length,
						     size_t plaintext_length);

/**
 * @brief Update the software ChaCha20-Poly1305 operation with additional data.
 *
 * @param[in,out] operation    Pointer to the AEAD operation structure.
 * @param[in]     input        Additional data.
 * @param[in]     input_length Length of the additional data.
 *
 * @retval PSA_SUCCESS Additional data processed successfully.
 * @retval PSA_ERROR_BAD_STATE AD already finalized or operation not initialized.
 */
psa_status_t cracen_sw_chacha20_poly1305_update_ad(cracen_aead_operation_t *operation,
						   const uint8_t *input,
						   size_t input_length);

/**
 * @brief Update the software ChaCha20-Poly1305 operation with new data.
 *
 * @param[in,out] operation     Pointer to the AEAD operation structure.
 * @param[in]     input         Input data (plaintext for encryption, ciphertext for decryption).
 * @param[in]     input_length  Length of the input data.
 * @param[out]    output        Output buffer (ciphertext for encryption, plaintext for decryption).
 * @param[in]     output_size   Size of the output buffer.
 * @param[out]    output_length Pointer where to store the actual output length.
 *
 * @retval PSA_SUCCESS Data processed successfully.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL Output buffer too small.
 */
psa_status_t cracen_sw_chacha20_poly1305_update(cracen_aead_operation_t *operation,
						const uint8_t *input,
						size_t input_length, uint8_t *output,
						size_t output_size,
						size_t *output_length);

/**
 * @brief Finish the software ChaCha20-Poly1305 encryption operation.
 *
 * @param[in,out] operation         Pointer to the AEAD operation structure.
 * @param[out]    ciphertext        Output buffer for remaining ciphertext (typically none).
 * @param[in]     ciphertext_size   Size of the ciphertext buffer.
 * @param[out]    ciphertext_length Pointer to store the actual ciphertext length.
 * @param[out]    tag               Output buffer for the authentication tag.
 * @param[in]     tag_size          Size of the tag buffer.
 * @param[out]    tag_length        Pointer to store the actual tag length.
 *
 * @retval PSA_SUCCESS Operation finished successfully; tag generated.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL Tag buffer too small.
 * @retval PSA_ERROR_BAD_STATE Operation not initialized.
 */
psa_status_t cracen_sw_chacha20_poly1305_finish(cracen_aead_operation_t *operation,
						uint8_t *ciphertext,
						size_t ciphertext_size, size_t *ciphertext_length,
						uint8_t *tag, size_t tag_size, size_t *tag_length);

/**
 * @brief Verify the tag and finish the software ChaCha20-Poly1305 decryption operation.
 *
 * @param[in,out] operation        Pointer to the AEAD operation structure.
 * @param[out]    plaintext        Output buffer for remaining plaintext (typically none).
 * @param[in]     plaintext_size   Size of the plaintext buffer.
 * @param[out]    plaintext_length Pointer to store the actual plaintext length.
 * @param[in]     tag              Authentication tag to verify.
 * @param[in]     tag_length       Length of the authentication tag.
 *
 * @retval PSA_SUCCESS Authentication tag verified successfully.
 * @retval PSA_ERROR_INVALID_SIGNATURE Authentication tag verification failed.
 * @retval PSA_ERROR_BAD_STATE Operation not initialized.
 */
psa_status_t cracen_sw_chacha20_poly1305_verify(cracen_aead_operation_t *operation,
						uint8_t *plaintext,
						size_t plaintext_size, size_t *plaintext_length,
						const uint8_t *tag, size_t tag_length);

/**
 * @brief Abort the software ChaCha20-Poly1305 operation.
 *
 * This function clears the operation context and any sensitive data.
 *
 * @param[in,out] operation Pointer to the AEAD operation structure.
 *
 * @retval PSA_SUCCESS Operation aborted successfully.
 */
psa_status_t cracen_sw_chacha20_poly1305_abort(cracen_aead_operation_t *operation);

/**
 * @}
 */

#endif /* CRACEN_SW_CHACHA_POLY_H */
