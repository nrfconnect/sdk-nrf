/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_MAC_CMAC_H
#define CRACEN_MAC_CMAC_H

#include <psa/crypto.h>
#include <cracen/common.h>
#include "cracen_psa_primitives.h"

/**
 * @brief Set up a CMAC operation.
 *
 * This function initializes a CMAC operation with the provided key attributes
 * and key buffer. It prepares the operation structure for subsequent CMAC
 * processing.
 *
 * @param[in,out] operation        Pointer to the CMAC operation structure to be initialized.
 * @param[in]     attributes       Pointer to the key attributes structure.
 * @param[in]     key_buffer       Pointer to the buffer containing the key material.
 * @param[in]     key_buffer_size  Size of the key buffer in bytes.
 *
 * @return PSA_SUCCESS on success or a valid PSA status code.
 */
psa_status_t cracen_cmac_setup(cracen_mac_operation_t *operation,
			       const psa_key_attributes_t *attributes, const uint8_t *key_buffer,
			       size_t key_buffer_size);

/**
 * @brief Update a CMAC operation with input data.
 *
 * This function processes a chunk of input data as part of a CMAC operation.
 * It can be called multiple times to process data in chunks.
 *
 * @param[in,out] operation    Pointer to the CMAC operation structure.
 * @param[in]     input        Pointer to the input data buffer.
 * @param[in]     input_length Length of the input data in bytes.
 *
 * @return PSA_SUCCESS on success or a valid PSA status code.
 */
psa_status_t cracen_cmac_update(cracen_mac_operation_t *operation, const uint8_t *input,
				size_t input_length);

/**
 * @brief Finalize a CMAC operation.
 *
 * This function completes the CMAC operation and releases any resources
 * associated with it. It should be called after all input data has been
 * processed.
 *
 * @param[in,out] operation Pointer to the CMAC operation structure.
 *
 * @return PSA_SUCCESS on success or a valid PSA status code.
 */
psa_status_t cracen_cmac_finish(cracen_mac_operation_t *operation);

#endif /* CRACEN_MAC_CMAC_H */
