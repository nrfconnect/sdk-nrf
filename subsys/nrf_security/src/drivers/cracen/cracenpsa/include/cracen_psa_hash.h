/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_HASH_H
#define CRACEN_PSA_HASH_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"

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

#endif /* CRACEN_PSA_HASH_H */
