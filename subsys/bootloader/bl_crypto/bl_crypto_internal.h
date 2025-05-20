/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
 * @param[in]  external Whether this function is called through an EXT_API, in
 *                      which case it can only use stack memory.
 *
 * @return 0 if success, error code otherwise.
 */
int get_hash(uint8_t *hash, const uint8_t *data, uint32_t data_len, bool external);

#endif
