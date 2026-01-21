/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file cracen_sw_aes_cbc.h
 * @defgroup cracen_sw_aes_cbc CRACEN Software AES-CBC
 * @brief Software implementation of AES-CBC for CRACEN driver
 *
 * This module provides a software implementation of AES-CBC (Cipher Block Chaining)
 * for the CRACEN_SW driver.
 *
 * @{
 */

#ifndef CRACEN_SW_AES_CBC_H
#define CRACEN_SW_AES_CBC_H

#include <psa/crypto.h>
#include <stdbool.h>
#include <sxsymcrypt/keyref.h>
#include <cracen_psa_primitives.h>

/**
 * @brief Setup software AES-CBC operation.
 *
 * @param[in,out] operation        Pointer to the cipher operation structure.
 * @param[in]     attributes       Key attributes.
 * @param[in]     key_buffer       Key material.
 * @param[in]     key_buffer_size  Size of the key material.
 * @param[in]     alg              Algorithm.
 * @param[in]     dir              Direction (CRACEN_ENCRYPT or CRACEN_DECRYPT).
 *
 * @retval PSA_SUCCESS on success.
 * @retval PSA_ERROR_INVALID_ARGUMENT if any parameter is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED if the key type is not supported.
 * @retval PSA_ERROR_INSUFFICIENT_MEMORY if memory allocation fails.
 */
psa_status_t cracen_sw_aes_cbc_setup(cracen_cipher_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg, enum cipher_operation dir);

/**
 * @brief Set IV for the software AES-CBC operation.
 *
 * @param[in,out] operation   Pointer to the cipher operation structure.
 * @param[in]     iv          IV value (16 bytes).
 * @param[in]     iv_length   Length of IV (must be 16).
 *
 * @retval PSA_SUCCESS on success.
 * @retval PSA_ERROR_INVALID_ARGUMENT if iv_length is not 16.
 */
psa_status_t cracen_sw_aes_cbc_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				      size_t iv_length);

/**
 * @brief Update the software AES-CBC operation with new data.
 *
 * @param[in,out] operation       Pointer to the cipher operation structure.
 * @param[in]     input           Input data.
 * @param[in]     input_length    Length of the input data.
 * @param[out]    output          Output buffer.
 * @param[in]     output_size     Size of the output buffer.
 * @param[out]    output_length   Pointer to store the actual output length.
 *
 * @retval PSA_SUCCESS on success.
 * @retval PSA_ERROR_INVALID_ARGUMENT if any parameter is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL if output buffer is too small.
 */
psa_status_t cracen_sw_aes_cbc_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length);

/**
 * @brief Finish the software AES-CBC operation.
 *
 * @param[in,out] operation       Pointer to the cipher operation structure.
 * @param[out]    output          Output buffer for final block.
 * @param[in]     output_size     Size of the output buffer.
 * @param[out]    output_length   Pointer to store the actual output length.
 *
 * @retval PSA_SUCCESS on success.
 * @retval PSA_ERROR_INVALID_ARGUMENT if any parameter is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL if output buffer is too small.
 * @retval PSA_ERROR_INVALID_PADDING if PKCS7 padding is invalid.
 */
psa_status_t cracen_sw_aes_cbc_finish(cracen_cipher_operation_t *operation, uint8_t *output,
				      size_t output_size, size_t *output_length);

/**
 * @brief Perform a single-shot software AES-CBC encryption.
 *
 * @param[in]  attributes       Key attributes.
 * @param[in]  key_buffer       Key material.
 * @param[in]  key_buffer_size  Size of the key material.
 * @param[in]  alg              Algorithm (PSA_ALG_CBC_NO_PADDING or PSA_ALG_CBC_PKCS7).
 * @param[in]  iv               IV value (16 bytes).
 * @param[in]  iv_length        Length of IV (must be 16).
 * @param[in]  input            Input data to encrypt.
 * @param[in]  input_length     Length of the input data.
 * @param[out] output           Output buffer.
 * @param[in]  output_size      Size of the output buffer.
 * @param[out] output_length    Pointer to store the actual output length.
 *
 * @retval PSA_SUCCESS on success.
 * @retval PSA_ERROR_INVALID_ARGUMENT if any parameter is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL if output buffer is too small.
 * @retval PSA_ERROR_NOT_SUPPORTED if the algorithm is not supported.
 */
psa_status_t cracen_sw_aes_cbc_encrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *iv, size_t iv_length,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length);

/**
 * @brief Perform a single-shot software AES-CBC decryption.
 *
 * @param[in]  attributes       Key attributes.
 * @param[in]  key_buffer       Key material.
 * @param[in]  key_buffer_size  Size of the key material.
 * @param[in]  alg              Algorithm (PSA_ALG_CBC_NO_PADDING or PSA_ALG_CBC_PKCS7).
 * @param[in]  iv               IV value (16 bytes).
 * @param[in]  iv_length        Length of IV (must be 16).
 * @param[in]  input            Input data to decrypt.
 * @param[in]  input_length     Length of the input data.
 * @param[out] output           Output buffer.
 * @param[in]  output_size      Size of the output buffer.
 * @param[out] output_length    Pointer to store the actual output length.
 *
 * @retval PSA_SUCCESS on success.
 * @retval PSA_ERROR_INVALID_ARGUMENT if any parameter is invalid.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL if output buffer is too small.
 * @retval PSA_ERROR_INVALID_PADDING if PKCS7 padding is invalid.
 * @retval PSA_ERROR_NOT_SUPPORTED if the algorithm is not supported.
 */
psa_status_t cracen_sw_aes_cbc_decrypt(const psa_key_attributes_t *attributes,
				       const uint8_t *key_buffer, size_t key_buffer_size,
				       psa_algorithm_t alg, const uint8_t *iv, size_t iv_length,
				       const uint8_t *input, size_t input_length, uint8_t *output,
				       size_t output_size, size_t *output_length);

/**
 * @}
 */

#endif /* CRACEN_SW_AES_CBC_H */
