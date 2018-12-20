/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BOOTLOADER_CRYPTO_H__
#define BOOTLOADER_CRYPTO_H__

#include <zephyr/types.h>

/* Placeholder defines. Values should be updated, if no existing errors can be
 * used instead. */
#define EPKHASHINV 101
#define ESIGINV    102


/**
 * @brief Verify a signature using configured signature and SHA-256
 *
 * Verifies the public key against the public key hash, then verifies the hash
 * of the signed data against the signature using the public key.
 *
 * @param[in]  public_key       Public key.
 * @param[in]  public_key_hash  Expected hash of the public key. This is the
 *                              root of trust.
 * @param[in]  signature        Firmware signature.
 * @param[in]  firmware         Firmware.
 * @param[in]  firmware_len     Length of firmware.
 *
 * @retval 0            On success.
 * @retval -EPKHASHINV  If public_key_hash didn't match public_key.
 * @retval -ESIGINV     If signature validation failed.
 *
 * @remark No parameter can be NULL.
 */
int crypto_root_of_trust(const u8_t *public_key,
		const u8_t *public_key_hash,
		const u8_t *signature,
		const u8_t *firmware,
		const u32_t firmware_len);

#endif

