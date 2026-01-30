/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_KEY_WRAP_COMMON_H
#define CRACEN_KEY_WRAP_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <cracen/common.h>
#include <silexpk/core.h>

/* RFC3394 and RFC5649 */
#define CRACEN_KEY_WRAP_BLOCK_SIZE			8u
#define CRACEN_KEY_WRAP_INTEGRITY_CHECK_REG_SIZE	CRACEN_KEY_WRAP_BLOCK_SIZE
#define CRACEN_KEY_WRAP_ITERATIONS_COUNT		5u

/** @brief Execute key wrap operation on multiple blocks of fixed size and
 *         calculate integrity check register value.
 *
 * @param[in] keyref                  AES key reference.
 * @param[in, out] block_array        Array with key material in blocks.
 * @param[in] plaintext_blocks_count  Number of blocks in @p block_array.
 * @param[out] integrity_check_reg    Integrity check register.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT
 * @retval PSA_ERROR_NOT_SUPPORTED
 * @retval PSA_ERROR_BUFFER_TOO_SMALL
 */
psa_status_t cracen_key_wrap(const struct sxkeyref *keyref, uint8_t *block_array,
			     size_t plaintext_blocks_count, uint8_t *integrity_check_reg);

/** @brief Execute key unwrap operation on multiple blocks of fixed size.
 *
 * @param[in] keyref                  AES key reference.
 * @param[in, out] cipher_input_block Block containing the integrity check register.
 * @param[in, out] block_array        Array with key material in blocks.
 * @param[in] ciphertext_blocks_count Number of blocks in @p cipher_input_block.
 *
 * @retval PSA_SUCCESS                The operation completed successfully.
 * @retval PSA_ERROR_INVALID_ARGUMENT
 * @retval PSA_ERROR_NOT_SUPPORTED
 * @retval PSA_ERROR_BUFFER_TOO_SMALL
 */
psa_status_t cracen_key_unwrap(const struct sxkeyref *keyref, uint8_t *cipher_input_block,
			       uint8_t *block_array, size_t ciphertext_blocks_count);

#endif /* CRACEN_KEY_WRAP_COMMON_H */
