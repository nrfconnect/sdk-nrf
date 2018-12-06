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
 * @param[in]  pk            Public key.
 * @param[in]  pk_hash       Expected hash of the public key. This is the root
 *                           of trust.
 * @param[in]  sig           Signature
 * @param[in]  fw            Firmware
 * @param[in]  fw_len        Length of firmware.
 *
 * @retval 0            On success.
 * @retval -EPKHASHINV  If pk_hash didn't match pk.
 * @retval -ESIGINV     If signature validation failed.
 *
 * @remark No parameter can be NULL.
 */
int crypto_root_of_trust(const u8_t *pk,
		const u8_t *pk_hash,
		const u8_t *sig,
		const u8_t *fw,
		const u32_t fw_len);

#endif

