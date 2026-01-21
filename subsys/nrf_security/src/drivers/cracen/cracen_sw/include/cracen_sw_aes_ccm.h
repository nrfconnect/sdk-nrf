/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file cracen_sw_aes_ccm.h
 *
 * @defgroup cracen_sw_aes_ccm Software AES-CCM implementation
 * @{
 * @brief Software implementation of the AES-CCM (Counter with CBC-MAC) mode.
 *
 * This module provides a software-based implementation of AES-CCM for use
 * as a workaround for hardware limitations with multipart operations.
 */

#ifndef CRACEN_SW_AES_CCM_H
#define CRACEN_SW_AES_CCM_H

#include <psa/crypto.h>
#include <psa/crypto_values.h>
#include <stdbool.h>
#include <stdint.h>
#include "cracen_psa_primitives.h"

/**
 * @brief Set up the software AES-CCM encryption operation.
 *
 * @param[in,out] operation       Pointer to the AEAD operation structure.
 * @param[in]     attributes      Key attributes.
 * @param[in]     key_buffer      Key material.
 * @param[in]     key_buffer_size Size of the key material.
 * @param[in]     alg             Algorithm to use.
 *
 * @retval PSA_SUCCESS Operation setup successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm or key type.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid key size.
 */
psa_status_t cracen_sw_aes_ccm_encrypt_setup(cracen_aead_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg);

/**
 * @brief Set up the software AES-CCM decryption operation.
 *
 * @param[in,out] operation       Pointer to the AEAD operation structure.
 * @param[in]     attributes      Key attributes.
 * @param[in]     key_buffer      Key material.
 * @param[in]     key_buffer_size Size of the key material.
 * @param[in]     alg             Algorithm to use.
 *
 * @retval PSA_SUCCESS Operation setup successfully.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm or key type.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid key size.
 */
psa_status_t cracen_sw_aes_ccm_decrypt_setup(cracen_aead_operation_t *operation,
					     const psa_key_attributes_t *attributes,
					     const uint8_t *key_buffer, size_t key_buffer_size,
					     psa_algorithm_t alg);

/**
 * @brief Set nonce for the software AES-CCM operation.
 *
 * @param[in,out] operation    Pointer to the AEAD operation structure.
 * @param[in]     nonce        Nonce value.
 * @param[in]     nonce_length Length of the nonce. Must be 7-13 bytes for CCM.
 *
 * @retval PSA_SUCCESS Nonce set successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid nonce length.
 */
psa_status_t cracen_sw_aes_ccm_set_nonce(cracen_aead_operation_t *operation, const uint8_t *nonce,
					 size_t nonce_length);

/**
 * @brief Set lengths for the software AES-CCM operation.
 *
 * CCM requires the lengths of additional data and plaintext to be known up-front.
 *
 * @param[in,out] operation        Pointer to the AEAD operation structure.
 * @param[in]     ad_length        Length of additional data.
 * @param[in]     plaintext_length Length of plaintext.
 *
 * @retval PSA_SUCCESS Lengths set successfully.
 */
psa_status_t cracen_sw_aes_ccm_set_lengths(cracen_aead_operation_t *operation, size_t ad_length,
					    size_t plaintext_length);

/**
 * @brief Update the software AES-CCM operation with additional data.
 *
 * @param[in,out] operation    Pointer to the AEAD operation structure.
 * @param[in]     input        Additional data.
 * @param[in]     input_length Length of the additional data.
 *
 * @retval PSA_SUCCESS Additional data processed successfully.
 * @retval PSA_ERROR_BAD_STATE AD already finalized or operation not initialized.
 */
psa_status_t cracen_sw_aes_ccm_update_ad(cracen_aead_operation_t *operation, const uint8_t *input,
					 size_t input_length);

/**
 * @brief Update the software AES-CCM operation with new data.
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
psa_status_t cracen_sw_aes_ccm_update(cracen_aead_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length);

/**
 * @brief Finish the software AES-CCM encryption operation.
 *
 * @param[in,out] operation         Pointer to the AEAD operation structure.
 * @param[out]    ciphertext        Output buffer for remaining ciphertext (typically none for CCM).
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
psa_status_t cracen_sw_aes_ccm_finish(cracen_aead_operation_t *operation, uint8_t *ciphertext,
				      size_t ciphertext_size, size_t *ciphertext_length,
				      uint8_t *tag, size_t tag_size, size_t *tag_length);

/**
 * @brief Verify the tag and finish the software AES-CCM decryption operation.
 *
 * @param[in,out] operation        Pointer to the AEAD operation structure.
 * @param[out]    plaintext        Output buffer for remaining plaintext (typically none for CCM).
 * @param[in]     plaintext_size   Size of the plaintext buffer.
 * @param[out]    plaintext_length Pointer to store the actual plaintext length.
 * @param[in]     tag              Authentication tag to verify.
 * @param[in]     tag_length       Length of the authentication tag.
 *
 * @retval PSA_SUCCESS Authentication tag verified successfully.
 * @retval PSA_ERROR_INVALID_SIGNATURE Authentication tag verification failed.
 * @retval PSA_ERROR_BAD_STATE Operation not initialized.
 */
psa_status_t cracen_sw_aes_ccm_verify(cracen_aead_operation_t *operation, uint8_t *plaintext,
				      size_t plaintext_size, size_t *plaintext_length,
				      const uint8_t *tag, size_t tag_length);

/**
 * @brief Abort the software AES-CCM operation.
 *
 * Clears the operation context and any sensitive data.
 *
 * @param[in,out] operation Pointer to the AEAD operation structure.
 *
 * @retval PSA_SUCCESS Operation aborted successfully.
 */
psa_status_t cracen_sw_aes_ccm_abort(cracen_aead_operation_t *operation);

/**
 * @brief Perform a single-part software AES-CCM encryption.
 *
 * @param[in]  attributes              Key attributes.
 * @param[in]  key_buffer              Key material.
 * @param[in]  key_buffer_size         Size of the key material.
 * @param[in]  alg                     Algorithm to use.
 * @param[in]  nonce                   Nonce value.
 * @param[in]  nonce_length            Length of the nonce.
 * @param[in]  additional_data         Additional authenticated data.
 * @param[in]  additional_data_length  Length of the additional data.
 * @param[in]  plaintext               Input plaintext to encrypt.
 * @param[in]  plaintext_length        Length of the plaintext.
 * @param[out] ciphertext              Output buffer for ciphertext and tag.
 * @param[in]  ciphertext_size         Size of the ciphertext buffer.
 * @param[out] ciphertext_length       Pointer to store the actual ciphertext and tag length.
 *
 * @retval PSA_SUCCESS Encryption completed successfully.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL Output buffer too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid nonce length or key size.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm or key type.
 */
psa_status_t cracen_sw_aes_ccm_encrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *nonce,
				       size_t nonce_length, const uint8_t *additional_data,
				       size_t additional_data_length, const uint8_t *plaintext,
				       size_t plaintext_length, uint8_t *ciphertext,
				       size_t ciphertext_size, size_t *ciphertext_length);

/**
 * @brief Perform a single-part software AES-CCM decryption.
 *
 * @param[in]  attributes              Key attributes.
 * @param[in]  key_buffer              Key material.
 * @param[in]  key_buffer_size         Size of the key material.
 * @param[in]  alg                     Algorithm to use.
 * @param[in]  nonce                   Nonce value.
 * @param[in]  nonce_length            Length of the nonce.
 * @param[in]  additional_data         Additional authenticated data.
 * @param[in]  additional_data_length  Length of the additional data.
 * @param[in]  ciphertext              Input ciphertext and tag to decrypt.
 * @param[in]  ciphertext_length       Length of the ciphertext and tag.
 * @param[out] plaintext               Output buffer for plaintext.
 * @param[in]  plaintext_size          Size of the plaintext buffer.
 * @param[out] plaintext_length        Pointer to store the actual plaintext length.
 *
 * @retval PSA_SUCCESS Decryption and verification completed successfully.
 * @retval PSA_ERROR_INVALID_SIGNATURE Authentication tag verification failed.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL Output buffer too small.
 * @retval PSA_ERROR_INVALID_ARGUMENT Invalid nonce length, invalid key size, or ciphertext too
 * short.
 * @retval PSA_ERROR_NOT_SUPPORTED Unsupported algorithm or key type.
 */
psa_status_t cracen_sw_aes_ccm_decrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *nonce,
				       size_t nonce_length, const uint8_t *additional_data,
				       size_t additional_data_length, const uint8_t *ciphertext,
				       size_t ciphertext_length, uint8_t *plaintext,
				       size_t plaintext_size, size_t *plaintext_length);

/**
 * @}
 */

#endif /* CRACEN_SW_AES_CCM_H */
