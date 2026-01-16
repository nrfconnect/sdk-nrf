/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include "common.h"
#include "cracen_psa_primitives.h"

/**
 * @brief Set up an HMAC operation.
 *
 * This function initializes an HMAC operation with the provided key and algorithm.
 *
 * @param[in,out] operation       Pointer to the HMAC operation context to be initialized.
 * @param[in]     attributes      Pointer to the key attributes structure.
 * @param[in]     key_buffer      Pointer to the buffer containing the key material.
 * @param[in]     key_buffer_size Size of the key buffer in bytes.
 * @param[in]     alg             The HMAC algorithm to be used.
 *
 * @return PSA_SUCCESS on success or a valid PSA status code.
 */
psa_status_t cracen_hmac_setup(cracen_mac_operation_t *operation,
			       const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			       size_t key_buffer_size, psa_algorithm_t alg);

/**
 * @brief Update an HMAC operation with input data.
 *
 * This function processes a chunk of input data as part of an ongoing HMAC operation.
 *
 * @param[in,out] operation    Pointer to the HMAC operation context.
 * @param[in]     input        Pointer to the input data buffer.
 * @param[in]     input_length Length of the input data in bytes.
 *
 * @return PSA_SUCCESS on success or a valid PSA status code.
 */
psa_status_t cracen_hmac_update(cracen_mac_operation_t *operation, const uint8_t *input,
				size_t input_length);

/**
 * @brief Finalize an HMAC operation.
 *
 * This function completes the HMAC operation and releases any associated resources.
 *
 * @param[in,out] operation Pointer to the HMAC operation context.
 *
 * @return PSA_SUCCESS on success or a valid PSA status code.
 */
psa_status_t cracen_hmac_finish(cracen_mac_operation_t *operation);
