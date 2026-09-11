/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_AES_MMO_H
#define CRACEN_AES_MMO_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>

#include "cracen_psa_primitives.h"

/** @brief Set up an AES-MMO (Zigbee) hash operation.
 *
 * @param[out] operation AES-MMO operation context.
 *
 * @retval PSA_SUCCESS The operation completed successfully.
 */
psa_status_t cracen_aes_mmo_setup(cracen_aes_mmo_operation_t *operation);

/** @brief Add a message fragment to an AES-MMO (Zigbee) hash operation.
 *
 * @param[in,out] operation    AES-MMO operation context.
 * @param[in] input            Message fragment to hash.
 * @param[in] input_length     Length of the message fragment in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT The message is longer than the algorithm allows.
 */
psa_status_t cracen_aes_mmo_update(cracen_aes_mmo_operation_t *operation, const uint8_t *input,
				   size_t input_length);

/** @brief Finish an AES-MMO (Zigbee) hash operation.
 *
 * @param[in,out] operation   AES-MMO operation context.
 * @param[out] hash           Buffer to store the digest.
 * @param[in] hash_size       Size of the digest buffer in bytes.
 * @param[out] hash_length    Length of the generated digest in bytes.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_BUFFER_TOO_SMALL The digest buffer is too small.
 */
psa_status_t cracen_aes_mmo_finish(cracen_aes_mmo_operation_t *operation, uint8_t *hash,
				   size_t hash_size, size_t *hash_length);

#endif /* CRACEN_AES_MMO_H */
