/** Cryptographic HMAC(Keyed-Hash Message Authentication Code).
 *
 * The "create operation" functions are specific to HMAC. For the rest,
 * the HMAC computation is done by using the following MAC API functions:
 * sx_mac_feed(), sx_mac_generate(), sx_mac_status() and sx_mac_wait().
 * The current implementation of HMAC does not support context-saving.
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Examples:
 * The following example shows typical sequence of function calls for computing
 * the HMAC of a message.
   @code
       sx_mac_create_hmac_sha2_256(ctx, key)
       sx_mac_feed(ctx, 'chunk 1')
       sx_mac_feed(ctx, 'chunk 2')
       sx_mac_generate(ctx)
       sx_mac_wait(ctx)
   @endcode
 */

#ifndef HMAC_HEADER_FILE
#define HMAC_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "internal.h"

struct sxmac;

/** Prepares a HMAC SHA256 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] keyref HMAC key reference @sa keyref.h
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_RETRY
 */
int sx_mac_create_hmac_sha2_256(struct sxmac *c, struct sxkeyref *keyref);

/** Prepares a HMAC SHA384 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] keyref HMAC key reference @sa keyref.h
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_RETRY
 *
 * @remark - \p the key reference contents should not be changed until the
 *              operation is completed.
 */
int sx_mac_create_hmac_sha2_384(struct sxmac *c, struct sxkeyref *keyref);

/** Prepares a HMAC SHA512 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] keyref HMAC key reference @sa keyref.h
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_RETRY
 *
 * @remark - \p the key reference contents should not be changed until the
 *              operation is completed.
 */
int sx_mac_create_hmac_sha2_512(struct sxmac *c, struct sxkeyref *keyref);

/** Prepares a HMAC SHA1 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] keyref HMAC key reference @sa keyref.h
 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_RETRY
 *
 * @remark - \p the key reference contents should not be changed until the
 *              operation is completed.
 */
int sx_mac_create_hmac_sha1(struct sxmac *c, struct sxkeyref *keyref);

/** Prepares a HMAC SHA224 MAC operation
 *
 * This function initializes the user allocated object \p c with a new MAC
 * operation context and reserves the HW resource.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the hashing functions (except the ones that are specific to
 * context-saving).
 *
 * @param[out] c MAC operation context
 * @param[in] keyref HMAC key reference @sa keyref.h

 * @return ::SX_OK
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_RETRY
 *
 * @remark - \p the key reference contents should not be changed until the
 *              operation is completed.
 */
int sx_mac_create_hmac_sha2_224(struct sxmac *c, struct sxkeyref *keyref);

#ifdef __cplusplus
}
#endif

#endif
