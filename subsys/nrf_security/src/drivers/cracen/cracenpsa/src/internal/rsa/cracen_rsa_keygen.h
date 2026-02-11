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
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * @warning RSA key generation requires at least 1 KB of memory.
 *		The generation can be time-consuming.
 */

#ifndef CRACEN_RSA_KEYGEN_H
#define CRACEN_RSA_KEYGEN_H

#include <cracen_psa_primitives.h>
#include <stdint.h>

/** @brief Generate an RSA private key.
 *
 * This function generates RSA primes p and q and computes the private key components
 * following FIPS 186-4 Appendix B.3.3. The generated primes undergo Miller-Rabin
 * primality testing with the number of rounds specified in FIPS 186-4 Table C.2.
 *
 * The output key format (standard or CRT) is determined by the @p privkey
 * structure's slotmask field, which must be initialized by the caller before
 * calling this function.
 *
 * @param[in] pubexp   Public exponent in big-endian format. Must not be NULL.
 *                     Must be an odd integer strictly greater than 2^16 and
 *                     strictly less than 2^256. The most significant byte must
 *                     not be zero.
 * @param[in] pubexpsz Size of the public exponent in bytes. Must be in the range
 *                     [3, 32]. A value of 3 with @p pubexp equal to 0x010000 is
 *                     rejected (would be exactly 2^16).
 * @param[in] keysz    Key size in bytes. Must be at least 256 (2048 bits) per
 *                     FIPS 186-4 requirements and an even number (key bit
 *                     length must be a multiple of 16). Maximum supported size
 *                     depends on PSA_MAX_RSA_KEY_BITS configuration.
 * @param[out] privkey Pointer to RSA key structure to receive the generated
 *                     private key. Must not be NULL. The structure must be
 *                     pre-initialized with:
 *                     -  slotmask set to indicate standard or CRT key format
 *                     -  elements[] pointing to pre-allocated buffers of
 *                        appropriate sizes for the key components
 *                     For standard keys: n (keysz bytes), d (keysz bytes).
 *                     For CRT keys: p, q (keysz/2 bytes each), dp, dq, qinv
 *                      (keysz/2 bytes each).
 *
 * @retval 0 (::SX_OK)                       Operation completed successfully.
 * @retval ::SX_ERR_INSUFFICIENT_ENTROPY	 Insufficient entropy for prime generation.
 * @retval ::SX_ERR_INPUT_BUFFER_TOO_SMALL   @p pubexpsz or @p keysz is zero, or public
 *                                           exponent is not greater than 2^16.
 * @retval ::SX_ERR_TOO_BIG                  Public exponent exceeds 32 bytes (2^256).
 * @retval ::SX_ERR_INCOMPATIBLE_HW          Key size less than 256 bytes (2048 bits).
 * @retval ::SX_ERR_TOO_MANY_ATTEMPTS	     Too many attempts to generate valid primes.
 * @retval ::SX_ERR_PK_RETRY		         Hardware resources unavailable; retry later.
 * @retval Other SX status codes from @ref cracen_status_codes on internal errors.
 */
int cracen_rsa_generate_privkey(sx_pk_req *req, uint8_t *pubexp, size_t pubexpsz,
				size_t keysz, struct cracen_rsa_key *privkey);

/** @} */

#endif /* CRACEN_RSA_KEYGEN_H */
