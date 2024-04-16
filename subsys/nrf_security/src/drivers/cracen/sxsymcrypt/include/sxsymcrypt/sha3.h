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

/** Hash algorithm SHA-3 224 */
extern const struct sxhashalg sxhashalg_sha3_224;

/** Hash algorithm SHA-3 256 */
extern const struct sxhashalg sxhashalg_sha3_256;

/** Hash algorithm SHA-3 384 */
extern const struct sxhashalg sxhashalg_sha3_384;

/** Hash algorithm SHA-3 512*/
extern const struct sxhashalg sxhashalg_sha3_512;

/** Hash algorithm SHAKE256, with output size fixed to 114 bytes (for ED448). */
extern const struct sxhashalg sxhashalg_shake256_114;

/** Creates a SHA3-224 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource. After successful execution
 * of this function, the context \p c can be passed to any of the hashing
 * functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA3-224 digest size is 28 bytes
 */
int sx_hash_create_sha3_224(struct sxhash *c, size_t csz);

/** Creates a SHA3-256 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource. After successful execution
 * of this function, the context \p c can be passed to any of the hashing
 * functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA3-256 digest size is 32 bytes
 */
int sx_hash_create_sha3_256(struct sxhash *c, size_t csz);

/** Creates a SHA3-384 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource. After successful execution
 * of this function, the context \p c can be passed to any of the hashing
 * functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA3-384 digest size is 48 bytes
 */
int sx_hash_create_sha3_384(struct sxhash *c, size_t csz);

/** Creates a SHA3-512 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource. After successful execution
 * of this function, the context \p c can be passed to any of the hashing
 * functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA3-512 digest size is 64 bytes
 */
int sx_hash_create_sha3_512(struct sxhash *c, size_t csz);

#ifdef __cplusplus
}
#endif

#endif
