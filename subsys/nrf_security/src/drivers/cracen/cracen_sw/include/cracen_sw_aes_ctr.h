/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_SW_AES_CTR_H
#define CRACEN_SW_AES_CTR_H

#include <psa/crypto.h>
#include <stdbool.h>
#include <sxsymcrypt/keyref.h>
#include "cracen_psa_primitives.h"

/**
 * @brief Setup software AES-CTR operation
 *
 * @param operation Pointer to cipher operation structure
 * @param attributes Key attributes
 * @param key_buffer Key material
 * @param key_buffer_size Size of key material
 * @return psa_status_t PSA_SUCCESS on success, error code otherwise
 */
psa_status_t cracen_sw_aes_ctr_setup(cracen_cipher_operation_t *operation,
				     const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size);

/**
 * @brief Set IV/counter for software AES-CTR operation
 *
 * @param operation Pointer to cipher operation structure
 * @param iv IV/counter value (16 bytes)
 * @param iv_length Length of IV (must be 16)
 * @return psa_status_t PSA_SUCCESS on success, error code otherwise
 */
psa_status_t cracen_sw_aes_ctr_set_iv(cracen_cipher_operation_t *operation, const uint8_t *iv,
				      size_t iv_length);

/**
 * @brief Update software AES-CTR operation with new data
 *
 * @param operation Pointer to cipher operation structure
 * @param input Input data
 * @param input_length Length of input data
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @param output_length Pointer to store actual output length
 * @return psa_status_t PSA_SUCCESS on success, error code otherwise
 */
psa_status_t cracen_sw_aes_ctr_update(cracen_cipher_operation_t *operation, const uint8_t *input,
				      size_t input_length, uint8_t *output, size_t output_size,
				      size_t *output_length);

/**
 * @brief Finish software AES-CTR operation
 *
 * @param operation Pointer to cipher operation structure
 * @param output_length Pointer to store actual output length (always 0 for CTR)
 * @return psa_status_t PSA_SUCCESS on success, error code otherwise
 */
psa_status_t cracen_sw_aes_ctr_finish(cracen_cipher_operation_t *operation, size_t *output_length);

/**
 * @brief Single-shot software AES-CTR crypt (encryption/decryption)
 *
 * @param attributes Key attributes
 * @param key_buffer Key material
 * @param key_buffer_size Size of key material
 * @param iv IV/counter value (16 bytes)
 * @param iv_length Length of IV (must be 16)
 * @param input Input data to crypt
 * @param input_length Length of input data
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @param output_length Pointer to store actual output length
 * @return psa_status_t PSA_SUCCESS on success, error code otherwise
 */
psa_status_t cracen_sw_aes_ctr_crypt(const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     const uint8_t *iv, size_t iv_length, const uint8_t *input,
				     size_t input_length, uint8_t *output, size_t output_size,
				     size_t *output_length);

#endif /* CRACEN_SW_AES_CTR_H */
