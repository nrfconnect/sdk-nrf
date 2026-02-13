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

psa_status_t cracen_key_wrap(const struct sxkeyref *keyref, uint8_t *block_array,
			     size_t plaintext_blocks_count, uint8_t *integrity_check_reg);

psa_status_t cracen_key_unwrap(const struct sxkeyref *keyref, uint8_t *cipher_input_block,
			       uint8_t *block_array, size_t ciphertext_blocks_count);

#endif /* CRACEN_KEY_WRAP_COMMON_H */
