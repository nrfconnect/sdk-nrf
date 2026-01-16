/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_rsa_keygen
 * @{
 * @brief RSA key generation for CRACEN PSA driver (internal use only).
 *
 * @warning These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_PSA_RSA_KEYGEN_H
#define CRACEN_PSA_RSA_KEYGEN_H

#include <cracen_psa_primitives.h>
#include <stdint.h>

/** @brief Generate an RSA private key.
 *
 * Generates an RSA private key based on FIPS 186-4.
 *
 * @param[in] pubexp   Public exponent.
 * @param[in] pubexpsz Size of the public exponent in bytes.
 * @param[in] keysz    Key size in bits.
 * @param[out] privkey Generated private key structure.
 *
 * @retval 0 (::SX_OK) The operation completed successfully.
 * @retval ::SX_ERR_INSUFFICIENT_ENTROPY Insufficient entropy for prime generation.
 * @retval ::SX_ERR_TOO_MANY_ATTEMPTS Too many attempts to generate valid primes.
 * @retval ::SX_ERR_PK_RETRY Resources not available; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_generate_privkey(uint8_t *pubexp, size_t pubexpsz, size_t keysz,
				struct cracen_rsa_key *privkey);

/** @} */

#endif /* CRACEN_PSA_RSA_KEYGEN_H */
