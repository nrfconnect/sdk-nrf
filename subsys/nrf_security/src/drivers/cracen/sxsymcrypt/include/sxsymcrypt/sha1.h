/** Cryptographic message hashing SHA-1.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SXSYMCRYPT_SHA1_HEADER_FILE
#define SXSYMCRYPT_SHA1_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "internal.h"

struct sxhash;

/** Hash algorithm SHA-1 (Secure Hash Algorithm 1)
 *
 * Deprecated algorithm. NIST formally deprecated use of SHA-1 in 2011
 * and disallowed its use for digital signatures in 2013. SHA-3 or SHA-2
 * are recommended instead.
 */
extern const struct sxhashalg sxhashalg_sha1;

/** Prepares a SHA1 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA1 digest size is 20 bytes
 */
int sx_hash_create_sha1(struct sxhash *c, size_t csz);

#ifdef __cplusplus
}
#endif

#endif
