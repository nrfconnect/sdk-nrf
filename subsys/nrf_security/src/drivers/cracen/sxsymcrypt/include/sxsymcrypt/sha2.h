/** Cryptographic message hashing SHA-2.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SXSYMCRYPT_SHA2_HEADER_FILE
#define SXSYMCRYPT_SHA2_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "internal.h"

struct sxhash;

#define SX_HASH_DIGESTSZ_SHA2_224 28
#define SX_HASH_DIGESTSZ_SHA2_256 32
#define SX_HASH_DIGESTSZ_SHA2_384 48
#define SX_HASH_DIGESTSZ_SHA2_512 64
#define SX_HASH_BLOCKSZ_SHA2_224 64
#define SX_HASH_BLOCKSZ_SHA2_256 64
#define SX_HASH_BLOCKSZ_SHA2_384 128
#define SX_HASH_BLOCKSZ_SHA2_512 128

/** Hash algorithm SHA-2 224
 *
 * Has only 32 bit capacity against length extension attacks.
 */
extern const struct sxhashalg sxhashalg_sha2_224;

/** Hash algorithm SHA-2 256
 *
 * Has no resistance against length extension attacks.
 */
extern const struct sxhashalg sxhashalg_sha2_256;

/** Hash algorithm SHA-2 384
 *
 * Has 128 bit capacity against length extension attacks.
 */
extern const struct sxhashalg sxhashalg_sha2_384;

/** Hash algorithm SHA-2 512
 *
 * Has no resistance against length extension attacks.
 */
extern const struct sxhashalg sxhashalg_sha2_512;

#ifdef __cplusplus
}
#endif

#endif
