/** Cryptographic message hashing SM3.
 *
 * @file
 *
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SXSYMCRYPT_SM3_HEADER_FILE
#define SXSYMCRYPT_SM3_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include "internal.h"

struct sxhash;

#define SX_HASH_DIGESTSZ_SM3 32
#define SX_HASH_BLOCKSZ_SM3 64

/** GM/T 0004-2012: SM3 cryptographic hash algorithm */
extern const struct sxhashalg sxhashalg_sm3;

#ifdef __cplusplus
}
#endif

#endif
