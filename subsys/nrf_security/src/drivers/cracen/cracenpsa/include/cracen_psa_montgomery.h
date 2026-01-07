/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_montgomery
 * @{
 * @brief Montgomery curve operations for CRACEN PSA driver (internal use only).
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_PSA_MONTGOMERY_H
#define CRACEN_PSA_MONTGOMERY_H

#include <stdint.h>

/** @brief Generate an X448 public key from a private key.
 *
 * @param[in] priv_key Private key.
 * @param[out] pub_key  Buffer to store the public key.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_x448_genpubkey(const uint8_t *priv_key, uint8_t *pub_key);

/** @brief Generate an X25519 public key from a private key.
 *
 * @param[in] priv_key Private key.
 * @param[out] pub_key  Buffer to store the public key.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_x25519_genpubkey(const uint8_t *priv_key, uint8_t *pub_key);

/** @} */

#endif /* CRACEN_PSA_MONTGOMERY_H */
