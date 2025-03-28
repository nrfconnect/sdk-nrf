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

#define SX_HASH_DIGESTSZ_SHA1 20
#define SX_HASH_BLOCKSZ_SHA1 64

/** Hash algorithm SHA-1 (Secure Hash Algorithm 1)
 *
 * Deprecated algorithm. NIST formally deprecated use of SHA-1 in 2011
 * and disallowed its use for digital signatures in 2013. SHA-3 or SHA-2
 * are recommended instead.
 */
extern const struct sxhashalg sxhashalg_sha1;

#ifdef __cplusplus
}
#endif

#endif
