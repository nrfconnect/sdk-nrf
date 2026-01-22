/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_AES_CBC_H
#define CRACEN_AES_CBC_H

#include <psa/crypto.h>
#include <stdint.h>
#include <sxsymcrypt/aes.h>
#include <sxsymcrypt/blkcipher.h>

psa_status_t cracen_aes_cbc_encrypt(const struct sxkeyref *key, const uint8_t *input,
				    size_t input_length, uint8_t *output, size_t output_size,
				    size_t *output_length, const uint8_t *iv);

psa_status_t cracen_aes_cbc_decrypt(const struct sxkeyref *key, const uint8_t *input,
				    size_t input_length, uint8_t *output, size_t output_size,
				    size_t *output_length, const uint8_t *iv);

#endif /* CRACEN_AES_CBC_H */
