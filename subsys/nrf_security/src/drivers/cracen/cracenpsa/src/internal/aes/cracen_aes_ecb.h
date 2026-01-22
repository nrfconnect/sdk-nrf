/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_AES_ECB_H
#define CRACEN_AES_ECB_H

#include <psa/crypto.h>
#include <stdint.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/blkcipher.h>

psa_status_t cracen_aes_ecb_encrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				    const uint8_t *input, size_t input_length, uint8_t *output,
				    size_t output_size, size_t *output_length);

psa_status_t cracen_aes_ecb_decrypt(struct sxblkcipher *blkciph, const struct sxkeyref *key,
				    const uint8_t *input, size_t input_length, uint8_t *output,
				    size_t output_size, size_t *output_length);

#endif /* CRACEN_AES_ECB_H */
