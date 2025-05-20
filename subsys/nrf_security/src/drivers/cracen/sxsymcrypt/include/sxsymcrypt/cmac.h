/** Message Authentication Code AES CMAC.
 *
 * @file
 * @copyright Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Examples:
 * The following examples show typical sequences of function calls for
 * generating a mac.
   @code
   1. One-shot operation MAC generation
	  sx_mac_create_aescmac(ctx, ...)
	  sx_mac_feed(ctx, ...)
	  sx_mac_generate(ctx)
	  sx_mac_wait(ctx)
   @endcode
 */

#ifndef CMAC_HEADER_FILE
#define CMAC_HEADER_FILE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "internal.h"
#include "mac.h"

/** Prepares an AES CMAC generation.
 *
 * This function initializes the user allocated object \p c with a new AES CMAC
 * operation context needed to run the CMAC generation.
 *
 * After successful execution of this function, the context \p c can be passed
 * to any of the CMAC functions.
 *
 * @param[out] c CMAC operation context
 * @param[in] key key used for the CMAC generation operation,
 *                expected size 16, 24 or 32 bytes
 * @return ::SX_OK
 * @return ::SX_ERR_INVALID_KEYREF
 * @return ::SX_ERR_INVALID_KEY_SZ
 * @return ::SX_ERR_INCOMPATIBLE_HW
 * @return ::SX_ERR_RETRY
 *
 * @pre - key reference provided by \p key must be initialized using
 *        sx_keyref_load_material() or sx_keyref_load_by_id()
 */
int sx_mac_create_aescmac(struct sxmac *c, const struct sxkeyref *key);

#ifdef __cplusplus
}
#endif

#endif
