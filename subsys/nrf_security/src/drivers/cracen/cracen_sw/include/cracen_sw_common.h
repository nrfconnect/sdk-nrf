/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef CRACEN_SW_COMMON_H
#define CRACEN_SW_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <psa/crypto.h>
#include <sxsymcrypt/keyref.h>

/**
 * @brief Encrypt a single AES block using ECB mode.
 *
 * This function is designed for CRACEN software workarounds that need
 * AES-ECB encryption functionality.
 *
 * @param key The AES key reference.
 * @param input Pointer to a 16-byte input block.
 * @param output Pointer to a 16-byte output buffer.
 *
 * @retval psa_status_t PSA_SUCCESS on success, error code otherwise.
 */
psa_status_t cracen_aes_ecb_encrypt(const struct sxkeyref *key, const uint8_t *input,
				    uint8_t *output);

#endif /* CRACEN_SW_COMMON_H */
