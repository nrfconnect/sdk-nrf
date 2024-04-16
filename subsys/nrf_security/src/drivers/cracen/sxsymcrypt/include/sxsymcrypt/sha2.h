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

/** Prepares a SHA256 hash operation context
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
 * @remark - SHA256 digest size is 32 bytes
 */
int sx_hash_create_sha256(struct sxhash *c, size_t csz);

/** Prepares a SHA384 hash operation context
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
 * @remark - SHA384 digest size is 48 bytes
 */
int sx_hash_create_sha384(struct sxhash *c, size_t csz);

/** Prepares a SHA512 hash operation context
 *
 * This function initializes the user allocated object \p c with a new hash
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing
 * functions.
 *
 * @param[out] c hash operation context
 * @param[in] csz size of the hash operation context
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @remark - SHA512 digest size is 64 bytes
 */
int sx_hash_create_sha512(struct sxhash *c, size_t csz);

/** Prepares a SHA224 hash operation context
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
 * @remark - SHA224 digest size is 28 bytes
 */
int sx_hash_create_sha224(struct sxhash *c, size_t csz);

#ifdef __cplusplus
}
#endif

#endif
