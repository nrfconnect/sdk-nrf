/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_SW_AES_CTR_H
#define CRACEN_SW_AES_CTR_H

#include <psa/crypto.h>
#include <stdbool.h>
#include <stdint.h>
#include <sxsymcrypt/keyref.h>
#include <cracen_psa_primitives.h>

/**
 * @brief Setup software AES-CTR operation
 *
 * @param operation Pointer to the cipher operation structure.
 * @param attributes Key attributes.
 * @param key_buffer Key material.
 * @param key_buffer_size Size of the key material.
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise
 */
psa_status_t cracen_sw_aes_ctr_setup(cracen_cipher_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size);

/**
 * @brief Set IV (Counter) for the software AES-CTR operation
 *
 * @param operation Pointer to the cipher operation structure.
 * @param iv IV (Counter) value (16 bytes).
 * @param iv_length Length of IV (must be 16).
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise
 */
psa_status_t cracen_sw_aes_ctr_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				      size_t iv_length);

/**
 * @brief Update the software AES-CTR operation with new data
 *
 * @param operation Pointer to cipher operation structure.
 * @param input Input data.
 * @param input_length Length of the input data.
 * @param output Output buffer.
 * @param output_size Size of the output buffer.
 * @param output_length Pointer where to store the actual output length.
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise
 */
psa_status_t cracen_sw_aes_ctr_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length);

/**
 * @brief Finish the software AES-CTR operation
 *
 * @param operation Pointer to the cipher operation structure.
 * @param output_length Pointer to the store actual output length (always 0 for CTR).
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise
 */
psa_status_t cracen_sw_aes_ctr_finish(cracen_cipher_operation_t *operation, size_t *output_length);

/**
 * @brief Perform a single-shot software AES-CTR encryption or decryption.
 *
 * @param attributes Key attributes.
 * @param key_buffer Key material.
 * @param key_buffer_size Size of the key material.
 * @param iv IV (Counter) value (16 bytes).
 * @param iv_length Length of IV (must be 16).
 * @param input Input data to encrypt or decrypt.
 * @param input_length Length of input data.
 * @param output Output buffer.
 * @param output_size Size of output buffer.
 * @param output_length Pointer to store the actual output length.
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise.
 */
psa_status_t cracen_sw_aes_ctr_crypt(const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     const uint8_t *iv, size_t iv_length, const uint8_t *input,
				     size_t input_length, uint8_t *output, size_t output_size,
				     size_t *output_length);

#endif /* CRACEN_SW_AES_CTR_H */
