/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BOOTLOADER_CRYPTO_INTERNAL_H__
#define BOOTLOADER_CRYPTO_INTERNAL_H__

#include <stddef.h>
#include <zephyr/types.h>
#include <stdbool.h>


/**
 * @brief Get hash of data
 *
 * @param[out] hash     Buffer to store hash in
 * @param[in]  data     Data to produce hash over
 * @param[in]  data_len Length of data to hash
 *
 * @return True if success, false otherwise.
 */
bool get_hash(u8_t *hash, const u8_t *data, u32_t data_len);


/**
 * @brief Verify hash of provided data against expected hash.
 *
 * @param[in]  data     Data to produce hash over
 * @param[in]  data_len Length of data to hash
 * @param[in]  expected Expected hash
 */
bool verify_hash(const u8_t *data, u32_t data_len, const u8_t *expected);

/**
 * @brief Verify truncated hash of data.
 *
 * @param[in]  data     Data to produce hash over
 * @param[in]  data_len Length of data to hash
 * @param[in]  expected Expected hash
 * @param[in]  hash_len Length of hash
 */
bool verify_truncated_hash(const u8_t *data, u32_t data_len,
			   const u8_t *expected, u32_t hash_len);

/**
 * @brief Verify signature of data.
 *
 * @param[in] data       Data to produce hash over
 * @param[in] data_len   Length of data to hash
 * @param[in] signature  Expected signature
 * @param[in] public_key Public Key
 */
bool verify_signature(const u8_t *data, u32_t data_len,
		const u8_t *signature, const u8_t *public_key);

#endif
