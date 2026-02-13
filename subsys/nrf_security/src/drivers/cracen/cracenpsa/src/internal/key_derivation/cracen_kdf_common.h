/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_KDF_COMMON_H
#define CRACEN_KDF_COMMON_H

#include <psa/crypto.h>
#include <stdint.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/blkcipher.h>

#define uint32_to_be(i)                                                                            \
	((((i)&0xFF) << 24) | ((((i) >> 8) & 0xFF) << 16) | ((((i) >> 16) & 0xFF) << 8) |          \
	 (((i) >> 24) & 0xFF))

typedef psa_status_t (*cracen_kdf_block_generator_t)(cracen_key_derivation_operation_t *);

/**
 * \brief Initialize and set up the MAC operation that will be used to generate pseudo-random
 *        bytes.
 *
 * \param[in, out] operation        Cracen key derivation operation object.
 * \param[in]      key_buffer       Key buffer or HKDF salt.
 * \param[in]      key_buffer_size  Size of key buffer in bytes.
 *
 * \return psa_status_t PSA status code.
 */
psa_status_t cracen_kdf_start_mac_operation(cracen_key_derivation_operation_t *operation,
					    const uint8_t *key_buffer, size_t key_buffer_size);

psa_status_t cracen_kdf_generate_output_bytes(cracen_key_derivation_operation_t *operation,
					      cracen_kdf_block_generator_t block_gen,
					      uint8_t *output, size_t output_length);

#endif /* CRACEN_KDF_COMMON_H */
