/** Cryptographic message hashing SHA-3.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SXSYMCRYPT_SHA3_HEADER_FILE
#define SXSYMCRYPT_SHA3_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "internal.h"

struct sxhash;

#define SX_HASH_DIGESTSZ_SHA3_224 28
#define SX_HASH_DIGESTSZ_SHA3_256 32
#define SX_HASH_DIGESTSZ_SHA3_384 48
#define SX_HASH_DIGESTSZ_SHA3_512 64
#define SX_HASH_BLOCKSZ_SHA3_224 144
#define SX_HASH_BLOCKSZ_SHA3_256 136
#define SX_HASH_BLOCKSZ_SHA3_384 104
#define SX_HASH_BLOCKSZ_SHA3_512 72

/** Hash algorithm SHA-3 224 */
extern const struct sxhashalg sxhashalg_sha3_224;

/** Hash algorithm SHA-3 256 */
extern const struct sxhashalg sxhashalg_sha3_256;

/** Hash algorithm SHA-3 384 */
extern const struct sxhashalg sxhashalg_sha3_384;

/** Hash algorithm SHA-3 512*/
extern const struct sxhashalg sxhashalg_sha3_512;

/** Hash algorithm SHAKE256, with output size fixed to 114 bytes (for Ed448). */
extern const struct sxhashalg sxhashalg_shake256_114;

#ifdef __cplusplus
}
#endif

#endif
